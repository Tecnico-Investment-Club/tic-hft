# Position Tracker: Explicação Detalhada

Este documento explica, de forma prática, cada parte do `PositionTracker`: o que recebe, como funciona e a lógica por trás de cada variável.

## 1. O que o `PositionTracker` recebe

A entrada principal é um `Fill`, que representa uma execução real da bolsa (trade executado).

Campos de `Fill`:

- `event_id`: identificador do evento.
- `symbol`: símbolo do ativo (ex.: `BTCUSDT`).
- `side`: direção (`Side::Buy` ou `Side::Sell`).
- `quantity`: quantidade executada (deve ser `> 0`).
- `price`: preço de execução (deve ser `> 0`).

Ideia principal: cada fill é um delta que altera a posição atual.

## 2. Estado interno que ele mantém

Internamente, o tracker guarda:

- `positions_`: `std::unordered_map<std::string, Position>`

Isso significa que existe uma posição independente para cada símbolo.

Cada `Position` contém:

- `symbol`: nome do ativo.
- `quantity`: posição líquida atual.
  - `> 0`: long
  - `< 0`: short
  - `== 0`: flat
- `avg_price`: preço médio da parte da posição que ainda está aberta.
- `realized_pnl`: lucro/prejuízo já realizado (parte fechada).

## 3. Função auxiliar: `signed_quantity(...)`

A função converte a quantidade para formato com sinal:

- `Buy` -> `+quantity`
- `Sell` -> `-quantity`

Isto simplifica o cálculo da nova posição:

- `new_qty = old_qty + fill_qty_signed`

## 4. Fluxo da função `on_fill(const Fill& fill)`

### 4.1 Validação de dados

O tracker ignora o fill se:

- `symbol` estiver vazio
- `quantity <= 0`
- `price <= 0`

Isto evita contaminar o estado com dados inválidos.

### 4.2 Obter ou criar posição do símbolo

Com:

- `auto& pos = positions_[fill.symbol];`

Se o símbolo não existir no mapa, é criado automaticamente com valores default.

### 4.3 Variáveis de trabalho

- `fill_qty_signed`: quantidade do fill com sinal.
- `old_qty`: quantidade antes do fill.
- `new_qty`: quantidade depois do fill.

Essas três variáveis são o núcleo da atualização.

## 5. Lógica por cenários

### 5.1 Cenário A: abrir posição nova ou aumentar no mesmo lado

Condição:

- posição anterior praticamente zero, ou
- já era long e entrou novo `Buy`, ou
- já era short e entrou novo `Sell`.

Neste caso, não há fecho de posição. Então:

1. Recalcula `avg_price` por média ponderada de notional.
2. Atualiza `quantity` para `new_qty`.
3. Não altera `realized_pnl`.

Cálculo usado:

- `old_notional = pos.avg_price * |old_qty|`
- `fill_notional = fill.price * |fill_qty_signed|`
- `total_qty = |old_qty| + |fill_qty_signed|`
- `avg_price = (old_notional + fill_notional) / total_qty`

### 5.2 Cenário B: fill no lado oposto (redução, fecho total ou reversão)

Quando o fill vem no lado contrário da posição aberta, o tracker calcula primeiro a parte que efetivamente fecha:

- `closing_qty = min(|fill_qty_signed|, |old_qty|)`

Depois atualiza o `realized_pnl`:

- Se a posição era long:
  - `realized_pnl += closing_qty * (fill.price - pos.avg_price)`
- Se a posição era short:
  - `realized_pnl += closing_qty * (pos.avg_price - fill.price)`

Em seguida:

- `pos.quantity = new_qty`

Se ficar flat (`new_qty` praticamente zero):

- força `pos.quantity = 0.0`
- limpa `pos.avg_price = 0.0`

Se houver reversão de lado (ex.: long para short):

- define `pos.avg_price = fill.price`

Isto garante que a posição invertida começa com o preço correto do fill que causou a inversão.

## 6. Leitura do estado

### `get_position(const std::string& symbol) const`

Retorna `std::optional<Position>`:

- `std::nullopt` se o símbolo não existir.
- `Position` se o símbolo existir.

### `get_total_realized_pnl() const`

Percorre todas as posições e soma `realized_pnl`.

## 7. Significado de cada variável da posição

- `quantity`: inventário líquido em aberto.
- `avg_price`: custo médio do inventário ainda aberto.
- `realized_pnl`: resultado já travado (não depende mais do preço atual).

## 8. Exemplo rápido (igual ao teste)

Sequência:

1. `Buy 1.0 @ 100`
2. `Sell 0.4 @ 120`

Resultado:

- `quantity = 0.6`
- `realized_pnl = 0.4 * (120 - 100) = 8.0`

Ou seja, fecha 40% da posição com lucro de 20 por unidade.

## 9. Observações importantes (MVP)

- `event_id` ainda não é usado para deduplicação.
- Não há persistência em disco.
- Não há proteção de concorrência para múltiplas threads.
- Não há cálculo de `unrealized_pnl` (mark-to-market) neste módulo.
