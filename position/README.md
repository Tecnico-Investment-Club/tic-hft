# Position Module

Módulo responsável por manter o estado de posições após cada fill de execução.

Este módulo ainda é um MVP: foca em quantidade, preço médio e realized PnL por símbolo.

## Objetivo

- Receber fills vindos da camada de Execution.
- Atualizar o estado de posição por símbolo.
- Expor leitura do estado atual (snapshot simples).
- Fornecer base para futura integração com Risk e Strategy.

## Estrutura Básica

- `include/PositionTracker.hpp`: tipos públicos e API principal.
- `src/PositionTracker.cpp`: implementação da lógica de atualização.
- `tests/position_test.cpp`: teste mínimo do fluxo buy/sell.
- `CMakeLists.txt`: build da biblioteca e teste opcional.

## Tipos Principais

### Side

Enum com a direção do fill:

- `Side::Buy`
- `Side::Sell`

### Fill

Representa um fill executado:

- `event_id`: id do evento/fill.
- `symbol`: ativo (ex.: `BTCUSDT`).
- `side`: buy/sell.
- `quantity`: quantidade executada (> 0).
- `price`: preço executado (> 0).

### Position

Estado acumulado por símbolo:

- `symbol`
- `quantity`: quantidade líquida (long > 0, short < 0).
- `avg_price`: preço médio da posição aberta.
- `realized_pnl`: PnL realizado acumulado.

## Funções Essenciais

### `on_fill(const Fill& fill)`

Atualiza a posição com um novo fill.

Comportamento atual:

- Ignora fills inválidos (`symbol` vazio, `quantity <= 0`, `price <= 0`).
- Se aumenta posição no mesmo lado, recalcula `avg_price` por notional médio.
- Se fecha parcial/totalmente, calcula `realized_pnl` no trecho fechado.
- Se vira de lado (long para short ou short para long), define novo `avg_price` no preço do fill que virou.

### `get_position(const std::string& symbol) const`

Retorna `std::optional<Position>`:

- `has_value() == false` se símbolo não existe.
- Caso exista, devolve snapshot atual da posição.

### `get_total_realized_pnl() const`

Soma `realized_pnl` de todos os símbolos guardados.

## Exemplo de Uso

```cpp
#include "PositionTracker.hpp"
#include <iostream>

using namespace hft::position;

int main() {
    PositionTracker tracker;

    tracker.on_fill(Fill{.event_id = 1, .symbol = "BTCUSDT", .side = Side::Buy, .quantity = 1.0, .price = 100.0});
    tracker.on_fill(Fill{.event_id = 2, .symbol = "BTCUSDT", .side = Side::Sell, .quantity = 0.4, .price = 120.0});

    auto pos = tracker.get_position("BTCUSDT");
    if (pos) {
        std::cout << "Qty: " << pos->quantity << "\n";
        std::cout << "Avg: " << pos->avg_price << "\n";
        std::cout << "Realized PnL: " << pos->realized_pnl << "\n";
    }

    return 0;
}
```

## Como fazer build

A partir da raiz do repo:

```bash
mkdir -p position/build
cmake -S position -B position/build -DBUILD_POSITION_TESTS=ON
cmake --build position/build -j4
```

## Como Testar

### Opção A: via CTest

```bash
ctest --test-dir position/build --output-on-failure
```

### Opção B: executando binário direto

```bash
./position/build/position_test
```

No Windows PowerShell, o binário normalmente fica como:

```powershell
.\\position\\build\\Debug\\position_test.exe
```

(ou `Release`, dependendo do gerador/configuração)

## Integração Recomendada no Pipeline

Fluxo alvo no sistema completo:

1. Strategy gera intenção de ordem.
2. Risk aprova/rejeita.
3. Execution envia e recebe fills.
4. PositionTracker recebe os fills via `on_fill(...)`.
5. Risk e Strategy leem `get_position(...)` para decisões seguintes.

## Limites Atuais do MVP

- Não há persistência em disco.
- Não há sincronização para multithread (single-thread expected neste estado).
- Não calcula unrealized PnL (mark-to-market) ainda.
- Não agrega exposição por portfolio.

## Próximos Passos Sugeridos

1. Adicionar thread-safety (mutex ou lock-free por design).
2. Adicionar `unrealized_pnl(mark_price)` e exposure por símbolo.
3. Adicionar snapshots por portfolio e equity consolidada.
4. Ligar eventos reais de fills vindos do módulo orders.
