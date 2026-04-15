# Risk Module

Módulo responsável por validar intenções de ordem contra regras de risco configuráveis.

Este módulo ainda é um MVP: foca em validações básicas (notional, posição, exposição, drawdown).

## Objetivo

- Receber intenção de ordem da camada de Strategy.
- Validar contra limites de risco.
- Aprovar ou rejeitar com motivo específico.
- Fornecer interface configurável de limites.

## Estrutura Básica

- `include/RiskEngine.hpp`: tipos públicos e interface principal.
- `src/RiskEngine.cpp`: implementação da validação.
- `tests/risk_test.cpp`: conjunto de testes das regras.
- `CMakeLists.txt`: build da biblioteca e testes opcionais.

## Tipos Principais

### Side

Enum com a direção da ordem:

- `Side::Buy`
- `Side::Sell`

### OrderIntent

Representa a intenção de ordem vinda da Strategy:

- `event_id`: id do evento.
- `symbol`: ativo (ex.: `BTCUSDT`).
- `side`: buy/sell.
- `quantity`: quantidade intendida (> 0).
- `price`: preço intendido (> 0).

### PositionSnapshot

Estado atual da posição no símbolo avaliado:

- `quantity`: posição líquida atual.
- `avg_price`: preço médio da posição aberta.

### RiskDecision

Resultado da validação:

- `approved`: true/false.
- `reason`: motivo aprovado ou motivo da rejeição.
- `adjusted_quantity`: quantidade ajustada (se necessário futuro).

### RiskLimits

Configuração de limites:

- `max_notional_per_order`: valor máximo por ordem (ex.: $100k).
- `max_position_per_symbol`: tamanho máximo por ativo aberto (ex.: 10 BTC).
- `max_total_exposure`: exposição total máxima da carteira (ex.: $800k).
- `max_daily_drawdown`: perda máxima tolerada no dia (ex.: -$50k).
- `max_orders_per_second`: taxa de ordens (futuro).

## Funções Essenciais

### `RiskEngine(double initial_capital)`

Construtor; define capital inicial da carteira.

### `validate(const OrderIntent& intent, const PositionSnapshot& current_pos, double unrealized_pnl = 0.0)`

Valida uma intenção de ordem e retorna `RiskDecision`.

Passos internos:

1. Valida parâmetros básicos (símbolo não vazio, qty > 0, price > 0).
2. Calcula notional (`qty * price`) e compara contra limite.
3. Calcula nova posição e valida contra limite por símbolo.
4. Calcula exposição total e valida contra limite.
5. Valida drawdown diário.

Se qualquer validação falhar, devolve `approved=false` com motivo.
Se passar todas, retorna `approved=true`.

### `set_limits(const RiskLimits& limits)`

Define todos os limites de uma vez.

### `set_max_notional(double value)`, `set_max_position(double value)`, etc.

Atualiza limite específico isoladamente.

### `update_daily_pnl(double pnl_delta)`

Acumula PnL do dia (quando há fills realizado).

### `reset_daily_pnl()`

Reseta PnL diário (tipicamente à meia-noite ou fim do trading).

### `get_available_capital() const`

Retorna `initial_capital + daily_pnl`.

### `get_daily_pnl() const`

Retorna PnL acumulado do dia (positivo = lucro, negativo = perda).

## Validações Implementadas

1. **Parâmetros válidos**: símbolo não vazio, quantity > 0, price > 0.
2. **Notional**: `qty * price <= max_notional_per_order`.
3. **Posição por símbolo**: nova posição não ultrapassa limite.
4. **Exposição total**: soma de todas as posições não ultrapassa limite.
5. **Drawdown**: PnL do dia não violou limite diário.

## Exemplo de Uso

```cpp
#include "RiskEngine.hpp"
#include <iostream>

using namespace hft::risk;

int main() {
    RiskEngine engine(1000000.0);
    
    RiskLimits limits;
    limits.max_notional_per_order = 100000.0;
    limits.max_position_per_symbol = 10.0;
    engine.set_limits(limits);
    
    PositionSnapshot pos{.quantity = 0.0, .avg_price = 0.0};
    
    OrderIntent order{
        .event_id = 1,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 1.0,
        .price = 50000.0
    };
    
    auto decision = engine.validate(order, pos);
    
    if (decision.approved) {
        std::cout << "Order approved: " << decision.reason << "\n";
    } else {
        std::cout << "Order rejected: " << decision.reason << "\n";
    }
    
    return 0;
}
```

## Como Buildar

A partir da raiz do repo:

```bash
mkdir -p risk/build
cmake -S risk -B risk/build -DBUILD_RISK_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build risk/build -j4
```

## Como Testar

### Opção A: via CTest

```bash
ctest --test-dir risk/build --output-on-failure
```

### Opção B: executando binário direto

```bash
./risk/build/risk_test
```

No Windows PowerShell (WSL):

```bash
wsl -e bash -lc "cd /mnt/c/Users/pedro/Documents/GITHUB/Tecnico_Investment_Club/tic-hft && cmake -S risk -B risk/build -DBUILD_RISK_TESTS=ON -DCMAKE_BUILD_TYPE=Release && cmake --build risk/build -j4 && ctest --test-dir risk/build --output-on-failure"
```

## Integração no Pipeline

Fluxo alvo:

1. Strategy gera `OrderIntent`.
2. Risk valida contra limites com `validate(intent, position)`.
3. Se `approved=true`, order segue para Execution.
4. Se `approved=false`, order é descartada com log do motivo.
5. Após execução, Risk atualiza PnL com `update_daily_pnl(...)`.

## Limites Atuais do MVP

- Sem rate limiting por tempo (max_orders_per_second não implementado).
- Sem ajuste automático de quantidade (adjusted_quantity ainda não usado).
- Sem persitência de configurações.
- Simples agregação de exposição (não diferencia ativo a ativo com correlação).

## Próximos Passos Sugeridos

1. Rate limiting de ordens por segundo.
2. Limites por portfolio (se múltiplos portfolios).
3. Ajuste automático de quantidade se ligeiramente over-limit.
4. Persistência de limites em arquivo config.
5. Integração com Market Depth para validar price impact.
