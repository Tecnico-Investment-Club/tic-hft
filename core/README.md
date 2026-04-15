# Core Module

O módulo **Core** define os tipos fundamentais e a orquestração de todo o sistema HFT (High-Frequency Trading). É o coração do projeto que coordena os diferentes componentes: **Position Tracker**, **Risk Engine** e **Execution**.

---

## Índice

1. [Visão Geral](#visão-geral)
2. [CommonTypes.hpp - Tipos Principais](#commontypeshpp---tipos-principais)
3. [Arquitetura de Fluxo de Dados](#arquitetura-de-fluxo-de-dados)
4. [Como Funciona o Orchestrator](#como-funciona-o-orchestrator)
5. [Como Fazer Build](#como-fazer-build)
6. [Como Testar](#como-testar)
7. [Exemplo Completo](#exemplo-completo)

---

## Visão Geral

O Core module funciona como um "tradutor" entre os diferentes módulos do sistema. Cada módulo (Position, Risk, Orders) tem os seus próprios tipos, mas o Core define tipos unificados para comunicação entre layers.

### Arquitetura de Camadas

```
┌─────────────────────────────────────────────────────┐
│           STRATEGY (gera sinais de trade)            │
│       (retorna core::OrderIntent)                   │
└──────────────────┬──────────────────────────────────┘
                   │
                   ↓ core::OrderIntent
┌─────────────────────────────────────────────────────┐
│              RISK ENGINE (valida risco)              │
│   (converte para risk::OrderIntent, valida)         │
└──────────────────┬──────────────────────────────────┘
                   │
                   ↓ RiskDecision (aprovado/rejeitado)
┌─────────────────────────────────────────────────────┐
│            EXECUTION (envia ordens)                  │
│      (cria position::Fill após execução)            │
└──────────────────┬──────────────────────────────────┘
                   │
                   ↓ position::Fill
┌─────────────────────────────────────────────────────┐
│          POSITION TRACKER (mantém estado)            │
│     (atualiza posições e PnL realizado)            │
└─────────────────────────────────────────────────────┘
```

---

## CommonTypes.hpp - Tipos Principais

Todos os tipos do sistema estão definidos em `include/CommonTypes.hpp`.

### 1. **Side** (Enum)

Direção de uma operação:

```cpp
enum class Side {
    Buy,    // Compra
    Sell    // Venda
};
```

### 2. **Fill** (Struct)

Representa uma execução que aconteceu. Vem do exchange e é enviada para o Position Tracker.

```cpp
struct Fill {
    uint64_t event_id = 0;      // ID único do evento
    std::string symbol;         // Ex: "BTCUSDT", "ETHUSDT"
    Side side = Side::Buy;      // Buy ou Sell
    double quantity = 0.0;      // Quantidade executada (sempre > 0)
    double price = 0.0;         // Preço de execução
};
```

**Fluxo:** Execution → Position Tracker

### 3. **Position** (Struct)

Estado atual de uma posição num ativo. Mantido pelo Position Tracker.

```cpp
struct Position {
    std::string symbol;         // Ex: "BTCUSDT"
    double quantity = 0.0;      // Qty líquida (long > 0, short < 0)
    double avg_price = 0.0;     // Preço médio de entrada
    double realized_pnl = 0.0;  // PnL realizado acumulado
};
```

### 4. **PositionSnapshot** (Struct)

Uma "foto" da posição. Usada por Strategy e Risk Engine para consultar o estado.

```cpp
struct PositionSnapshot {
    double quantity = 0.0;      // Qty líquida atual
    double avg_price = 0.0;     // Preço médio
};
```

**Fluxo:** Position Tracker → Strategy/Risk Engine

### 5. **OrderIntent** (Struct)

A intenção de ordem criada pela Strategy. O Risk Engine valida isto antes de executar.

```cpp
struct OrderIntent {
    uint64_t event_id = 0;      // ID único da intenção
    std::string symbol;         // Ex: "BTCUSDT"
    Side side = Side::Buy;      // Buy ou Sell
    double quantity = 0.0;      // Quantidade intendida (> 0)
    double price = 0.0;         // Preço intendido (> 0)
};
```

**Fluxo:** Strategy → Risk Engine → Execution

### 6. **RiskDecision** (Struct)

Resultado da validação de risco. Vem do Risk Engine.

```cpp
struct RiskDecision {
    bool approved = false;              // Ordem foi aprovada?
    std::string reason;                 // Ex: "Order approved" ou "Max position exceeded"
    double adjusted_quantity = 0.0;     // Qty ajustada (futuro)
};
```

**Fluxo:** Risk Engine → Execution/Strategy

### 7. **Order** (Struct)

Ordem enviada para o exchange. Usada internamente na camada de Execution.

```cpp
struct Order {
    uint64_t portfolio_id = 0;  // ID do portfolio
    uint64_t event_id = 0;      // ID da ordem
    uint64_t delivery_id = 0;   // ID de entrega/confirmação
    std::string asset_id;       // Ex: "BTCUSDT"
    double quantity = 0.0;      // Quantidade
    double price = 0.0;         // Preço
    std::string side;           // "BUY" ou "SELL"
};
```

### 8. **PortfolioSnapshot** (Struct)

Estado consolidado de toda a carteira (para monitoring).

```cpp
struct PortfolioSnapshot {
    double total_capital = 0.0;
    double available_capital = 0.0;     // Capital disponível para novas ordens
    double daily_pnl = 0.0;             // PnL do dia
    double total_realized_pnl = 0.0;    // PnL realizado acumulado
    double total_unrealized_pnl = 0.0;  // PnL não realizado (mark-to-market)
    double total_exposure = 0.0;        // Exposição total da carteira
    std::vector<Position> positions;    // Todas as posições
};
```

---

## Arquitetura de Fluxo de Dados

### Ciclo Completo de Trading

1. **Strategy gera intenção** (`core::OrderIntent`)
   - Consulta posição atual via `PositionTracker::get_position()`
   - Cria um intent baseado em lógica de trading

2. **Risk Engine valida** 
   - Recebe o intent e posição atual
   - Valida contra limites configuráveis
   - Retorna `RiskDecision`

3. **Execution envia ordem**
   - Se aprovada, envia para o exchange
   - Simula/aguarda by_fill do exchange

4. **Position Tracker actualiza**
   - Recebe o fill realizado
   - Atualiza quantidade, preço médio e PnL realizado

### Conversão de Tipos (Type Bridge)

O Core fornece conversores para traduzir entre tipos. Exemplo:

```cpp
// core::Side → position::Side
inline hft::position::Side to_position_side(hft::core::Side s) {
    return (s == hft::core::Side::Buy) ? hft::position::Side::Buy 
                                       : hft::position::Side::Sell;
}

// core::OrderIntent → risk::OrderIntent
inline hft::risk::OrderIntent to_risk_order_intent(const hft::core::OrderIntent& intent) {
    return {
        .event_id = intent.event_id,
        .symbol = intent.symbol,
        .side = to_risk_side(intent.side),
        .quantity = intent.quantity,
        .price = intent.price
    };
}
```

---

## Como Funciona o Orchestrator

O `Orchestrator` (em `examples/orchestrator_example.cpp`) é o exemplo principal que mostra como tudo funciona integrado.

### Componentes do Orchestrator

```cpp
class Orchestrator {
private:
    hft::position::PositionTracker position_tracker_;
    hft::risk::RiskEngine risk_engine_;
    SimpleStrategy strategy_;
    MockExecution execution_;
    int cycle_count_;
};
```

### Ciclo de Trading (`run_trading_cycle`)

```cpp
void run_trading_cycle(const std::string& symbol) {
    // 1. Obter estado atual da posição
    auto pos_opt = position_tracker_.get_position(symbol);
    
    // 2. Strategy gera intenção de ordem
    auto intent = strategy_.generate_signal(symbol, strategy_pos);
    
    // 3. Risk Engine valida
    auto risk_decision = risk_engine_.validate(risk_intent, risk_pos);
    
    if (!risk_decision.approved) {
        std::cout << "[Orchestrator] Order blocked by risk gates.\n";
        return;
    }
    
    // 4. Execution envia ordem
    execution_.send_order(intent);
    
    // 5. Position Tracker recebe fill e atualiza
    auto fill = execution_.get_last_fill();
    position_tracker_.on_fill(fill);
    
    // 6. Risk Engine actualiza PnL
    risk_engine_.update_daily_pnl(simulated_pnl);
}
```

### Ejemplo: SimpleStrategy

A estratégia exemplo é muito simples:
- Se não há posição: Compra 1 BTC a $50,000
- Se há posição longa: Vende 50% a $51,000

```cpp
class SimpleStrategy {
public:
    hft::core::OrderIntent generate_signal(
        const std::string& symbol,
        const hft::core::PositionSnapshot& current_pos
    ) {
        if (current_pos.quantity < 0.1) {
            return {
                .symbol = symbol,
                .side = hft::core::Side::Buy,
                .quantity = 1.0,
                .price = 50000.0
            };
        } else {
            return {
                .symbol = symbol,
                .side = hft::core::Side::Sell,
                .quantity = 0.5,
                .price = 51000.0
            };
        }
    }
};
```

---

## Como Fazer Build

### Pré-requisitos

**Windows (com Strawberry Perl/GCC):**
- CMake 3.16+
- GCC 13+ (via Strawberry Perl)

**Linux/WSL/macOS:**
- CMake 3.16+
- g++ ou clang++

### Windows (PowerShell)

```powershell
cd c:\Users\pedro\Documents\GITHUB\Tecnico_Investment_Club\tic-hft

# Build tudo (position, risk, core/examples)
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build . -j4

# Depois, os binários estarão em:
# - .\core\examples\orchestrator_example.exe
# - position\libhft-position.a
# - risk\libhft-risk.a
```

> **Nota:** No Windows, use `cmake --build . -j4` em vez de `make`. O gerador "Unix Makefiles" funciona bem com Strawberry Perl/GCC.

### Linux/WSL/macOS

```bash
cd ~/Tecnico_Investment_Club/tic-hft

# Build tudo
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# Ou usar make como alternativa
cd build
make -j4
```

### Build apenas o orchestrator example

```bash
cmake --build build --target orchestrator_example
```

### Desativar o orchestrator example

```bash
cmake -B build -DBUILD_ORCHESTRATOR_EXAMPLE=OFF
cmake --build build
```

### Nota sobre o módulo Orders

O módulo `orders` requer a biblioteca **libcurl**, que não está instalada por padrão. Atualmente está comentado no `CMakeLists.txt`. Para habilitar:

1. **Windows:** Instalar CURL via package manager (e.g., vcpkg)
2. **Linux/WSL:** `sudo apt install libcurl4-openssl-dev`
3. **macOS:** `brew install curl`

Depois, descomentar a linha em [CMakeLists.txt](CMakeLists.txt#L11).

---

## Como Testar

### 1. Testar o Orchestrator Example

O orchestrator simula 3 ciclos de trading para o símbolo "BTCUSDT":

```bash
# Linux/WSL/macOS
cd build
./core/examples/orchestrator_example

# Windows PowerShell
cd build
.\core\examples\orchestrator_example.exe
```

**Saída esperada:**

```
======================================================================
Trading Cycle #1 for: BTCUSDT
======================================================================
[Position] Current: qty=0.00 avg_price=$0.00
[Strategy] Generated intent: BTCUSDT BUY 1.00 @ $50000.00
[Risk] Decision: APPROVED - Order approved
[Execution] Sending order: BTCUSDT BUY 1.00 @ $50000.00
[Position] Updated with fill. New qty: 1.00
[Orchestrator] Status: Portfolio Capital=$999950.00 | Daily PnL=$0.00 | Total Realized PnL=$0.00

======================================================================
Trading Cycle #2 for: BTCUSDT
======================================================================
[Position] Current: qty=1.00 avg_price=$50000.00
[Strategy] Generated intent: BTCUSDT SELL 0.50 @ $51000.00
[Risk] Decision: APPROVED - Order approved
[Execution] Sending order: BTCUSDT SELL 0.50 @ $51000.00
[Position] Updated with fill. New qty: 0.50
[Orchestrator] Status: Portfolio Capital=$999999.50 | Daily PnL=$500.00 | Total Realized PnL=$500.00

======================================================================
Trading Cycle #3 for: BTCUSDT
======================================================================
[Position] Current: qty=0.50 avg_price=$50000.00
[Strategy] Generated intent: BTCUSDT SELL 0.50 @ $51000.00
[Risk] Decision: APPROVED - Order approved
[Execution] Sending order: BTCUSDT SELL 0.50 @ $51000.00
[Position] Updated with fill. New qty: 0.00
[Orchestrator] Status: Portfolio Capital=$1000000.00 | Daily PnL=$500.00 | Total Realized PnL=$1000.00
```

### 2. Testar Position Module

```bash
cd build/position
./position_test
```

Testa:
- Buy único
- Buy + Sell parcial (realiza PnL)
- Buy + Sell total (fecha posição)

### 3. Testar Risk Module

```bash
cd build/risk
./risk_test
```

Testa:
- Validações de notional máximo
- Validações de posição máxima por símbolo
- Validações de exposição total
- Validações de drawdown diário

### 4. Testar Orders Module (com Alpaca)

```bash
cd orders

# Exportar credenciais Alpaca
source ./env.sh

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Executar exemplo fire-and-forget
cd build
./example_fire_and_forget
```

⚠️ **Importante:** Este teste usa a API real de Alpaca (paper trading). Certifique-se de que as credenciais estão corretas em `.env`.

### 5. Teste Completo (Integração)

Para um teste que simula o ciclo completo com todos os módulos (sem contacts reais com exchange):

```bash
# Após compilar tudo
cd build
./core/examples/orchestrator_example
```

---

## Exemplo Completo

### Código Mínimo: Criar um Orchestrator e Rodar 1 Ciclo

```cpp
#include "core/include/CommonTypes.hpp"
#include "position/include/PositionTracker.hpp"
#include "risk/include/RiskEngine.hpp"

using namespace hft::core;
using namespace hft::position;
using namespace hft::risk;

int main() {
    // Inicializar componentes
    PositionTracker position_tracker;
    RiskEngine risk_engine(1000000.0);  // $1M capital
    
    // Configurar limites de risco
    RiskLimits limits;
    limits.max_notional_per_order = 100000.0;
    limits.max_position_per_symbol = 10.0;
    risk_engine.set_limits(limits);
    
    // Simular uma ordem intendida
    hft::core::OrderIntent intent{
        .event_id = 1,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 1.0,
        .price = 50000.0
    };
    
    // Consultar posição atual
    auto pos_opt = position_tracker.get_position("BTCUSDT");
    double current_qty = pos_opt ? pos_opt->quantity : 0.0;
    
    // Validar risco
    hft::risk::OrderIntent risk_intent{
        .event_id = 1,
        .symbol = "BTCUSDT",
        .side = risk::Side::Buy,
        .quantity = 1.0,
        .price = 50000.0
    };
    
    hft::risk::PositionSnapshot risk_pos{
        .quantity = current_qty,
        .avg_price = 0.0
    };
    
    auto decision = risk_engine.validate(risk_intent, risk_pos);
    
    if (decision.approved) {
        std::cout << "Ordem aprovada! Proceder com execução.\n";
        
        // Simular execução e receber fill
        hft::position::Fill fill{
            .event_id = 1,
            .symbol = "BTCUSDT",
            .side = hft::position::Side::Buy,
            .quantity = 1.0,
            .price = 50000.0
        };
        
        // Atualizar position tracker
        position_tracker.on_fill(fill);
        
        // Consultar nova posição
        auto updated = position_tracker.get_position("BTCUSDT");
        if (updated) {
            std::cout << "Qty: " << updated->quantity << " @ $" << updated->avg_price << "\n";
        }
    } else {
        std::cout << "Ordem rejeitada: " << decision.reason << "\n";
    }
    
    return 0;
}
```

---

## Próximos Passos

- [ ] Integrar ETL (Event Transformation Layer) para data reals da Binance
- [ ] Estratégias mais avançadas (mean reversion, momentum, ML)
- [ ] Simulations backtesting
- [ ] Dashboard/monitoring em tempo real
- [ ] Melhor integração com Alpaca/Binance APIs
- [ ] Logging estruturado (spdlog)
- [ ] Métricas e telemetrias

---

**Desenvolvido para fins educacionais pelo [Tecnico Investment Club](https://www.instagram.com/tecnico_investment_club/).**
