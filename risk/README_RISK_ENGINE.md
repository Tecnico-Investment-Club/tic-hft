# Risk Engine: Explicação Detalhada

Este documento explica, de forma prática e didática, cada parte do `RiskEngine`: como funciona, qual a lógica de cada validação e por que existe.

## 1. Problema que o Risk Engine resolve

A Strategy gera ordens que *poderiam* arruinar a carteira.

Exemplos de "ordens más":

- Comprar 1000 BTC por $1M quando o capital é $1M (fica sem saldo).
- Aumentar posição em BTCUSDT já quando já tem 10 (limite atingido).
- Explodir toda a exposição da carteira num único símbolo.
- Perde $100k num dia e continua a tradear agressivamente.

O Risk Engine **para estas ordens antes de enviarem para execution**.

## 2. O que ele recebe

Recebe três inputs para cada validação:

- **OrderIntent**: proposta da Strategy (símbolo, side, qty, price).
- **PositionSnapshot**: estado atual da carteira naquele símbolo (qty, avg_price).
- **unrealized_pnl**: PnL em aberto (opcional, ainda não usado no MVP).

## 3. O que ele guarda internamente

Estado principal:

- `initial_capital_`: capital inicial da carteira.
- `daily_pnl_`: PnL acumulado do dia (atualizado após cada fill real).
- `position_map_`: resumo de posições por símbolo (MVP simples).
- `limits_`: configuração de limites (notional, posição, exposição, drawdown).

## 4. Fluxo da função `validate(...)`

### 4.1 Validação 0: parâmetros básicos

```cpp
if (intent.symbol.empty() || intent.quantity <= 0.0 || intent.price <= 0.0) {
    return rejected("Invalid order parameters...");
}
```

Não executa nada se:
- símbolo está vazio
- quantidade é 0 ou negativa
- preço é 0 ou negativo

Isto evita NaNs e divisões por zero.

### 4.2 Validação 1: notional

```cpp
double notional = qty * price;
if (notional > max_notional_per_order) {
    return rejected("Order notional exceeds max...");
}
```

**Por quê?** Uma ordem muito grande pode drenar todo o capital de uma vez.

**Exemplo:**
- Capital: $1M
- Max notional: $100k
- Ordem: Buy 5 BTC @ $50k = $250k notional
- **Resultado**: rejeitado

### 4.3 Validação 2: posição por símbolo

```cpp
double new_qty = current_pos.quantity + fill_qty_signed;
if (abs(new_qty) > max_position_per_symbol) {
    return rejected("New position exceeds limit...");
}
```

**Por quê?** Quer manter diversificação; não quer ficar muito exposto a um só ativo.

**Exemplo:**
- Max posição BTCUSDT: 10
- Posição atual: 8
- Ordem: Buy 5 BTC
- Nova posição: 13 (ultrapassa 10)
- **Resultado**: rejeitado

### 4.4 Validação 3: exposição total

```cpp
double new_exposure = current_exposure + abs(new_qty);
if (new_exposure > max_total_exposure) {
    return rejected("Total exposure exceeds limit...");
}
```

**Por quê?** Risco sistémico: não quer toda a carteira em risco ao mesmo tempo.

**Exemplo:**
- Max exposição total: $800k
- Exposição atual (todas posições): $700k
- Ordem: Buy 150 BTC @ $50k = +$7.5M
- Nova exposição: $7.7M (ultrapassa $800k)
- **Resultado**: rejeitado

### 4.5 Validação 4: drawdown diário

```cpp
if (daily_pnl < max_daily_drawdown) {
    return rejected("Daily drawdown limit exceeded...");
}
```

**Por quê?** Stop-loss de segurança: se perdeu muito hoje, para.

**Exemplo:**
- Max drawdown: -$50k
- PnL hoje: -$60k (perda)
- Ordem: nova ordem
- **Resultado**: rejeitado, porque já perdemos mais que o permitido.

## 5. Como os limites funcionam

### RiskLimits

Estrutura com todos os limites:

```cpp
struct RiskLimits {
    double max_notional_per_order = 100000.0;      // $100k por ordem
    double max_position_per_symbol = 10.0;         // 10 BTC máximo
    double max_total_exposure = 800000.0;          // $800k total
    double max_daily_drawdown = -50000.0;          // -$50k de perda
    int max_orders_per_second = 100;               // (ainda não implementado)
};
```

### Setters

Podem-se atualizar limites em tempo real:

```cpp
engine.set_max_notional(150000.0);  // aumenta para $150k
engine.set_max_position(20.0);       // aumenta para 20 BTC
```

## 6. Gestão de PnL diário

### Update

Quando uma ordem é executada com lucro/perda, o Risk Engine atualiza:

```cpp
engine.update_daily_pnl(500.0);   // lucro de $500
engine.update_daily_pnl(-1000.0); // perda de $1k
```

### Reset

Tipicamente à meia-noite ou fim de trading:

```cpp
engine.reset_daily_pnl();  // volta a 0
```

### Query

Fazer read do estado:

```cpp
double available = engine.get_available_capital();  // initial + daily_pnl
double pnl = engine.get_daily_pnl();                // lucro/perda do dia
double exposure = engine.get_total_exposure();      // soma de |qtys|
```

## 7. Cenários de uso no pipeline

### Cenário A: Ordem aprovada

```cpp
OrderIntent order = strategy.generate_signal();
PositionSnapshot pos = position_tracker.get_position(order.symbol);

auto decision = risk_engine.validate(order, pos);

if (decision.approved) {
    execution.send_order(order);
} else {
    logger.warning("Order rejected: " + decision.reason);
}
```

### Cenário B: Drawdown atingido, paralyser a trading

```
// Durante o dia:
risk_engine.update_daily_pnl(-50000.0);  // perdemos $50k

// Strategy tenta enviar nova ordem:
auto decision = risk_engine.validate(order, pos);
// decision.approved = false
// decision.reason = "Daily drawdown limit exceeded"
// Order não é enviada.
```

### Cenário C: Aumentar limite se Performance é boa

```cpp
// Se PnL > $100k, aumenta agressividade:
if (risk_engine.get_daily_pnl() > 100000.0) {
    risk_engine.set_max_notional(200000.0);
}
```

## 8. Observações importantes

- **Ordem da validação**: falha na primeira validação e devolve logo (early return).
- **Quantidade assinada**: buy = +qty, sell = -qty (igual a Position).
- **Abs() de exposição**: exposição é sempre positiva (soma de |qtys|).
- **Drawdown é negativo**: -$50k significa perda, não -50000.0.
- **Capital disponível**: initial + daily_pnl (pode ser negativo se drawdown muito).

## 9. MVP vs. Futuro

**MVP atual (implementado):**
- Notional, posição, exposição, drawdown.
- Limites configuráveis.
- PnL diário com reset.

**Futuro:**
- Rate limiting por tempo (max ordens/segundo).
- VaR/stress testing das correlações.
- Ajuste automático de quantidade.
- Market depth para validar price impact.
- Limites por portfolio (se múltiplos portfolios).
