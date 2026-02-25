# Como Usar Orders

## 1️⃣ Estrutura do Order

Toda ordem tem 7 campos:

```cpp
struct Order {
    uint64_t portfolio_id;    // Teu portfolio (ex: 1)
    uint64_t event_id;        // ID único da ordem (ex: 100, 101, ...)
    uint64_t delivery_id;     // Lote (ex: 1, 2, 3)
    std::string asset_id;     // Símbolo (ex: "AAPL", "MSFT")
    double quantity;          // Quantas ações (ex: 100.0)
    double price;             // Preço por ação (ex: 150.0)
    std::string side;         // "BUY" ou "SELL"
};
```

## 2️⃣ API Key Setup

**Opção 1: Usar ficheiro .env (Recomendado)**

```bash
# Copy template
cp .env.example .env

# Edit .env with your credentials
# Windows: notepad .env
# Linux:   nano .env
```

Ver [../CONFIG_TEMPLATE.md](../CONFIG_TEMPLATE.md) para detalhes.

**Opção 2: Variável de ambiente (Manual)**

```bash
# Linux / Mac
export ALPACA_API_KEY="sk_paper123..."
export ALPACA_BASE_URL="https://paper-api.alpaca.markets"

# Windows PowerShell
$env:ALPACA_API_KEY="sk_paper123..."
$env:ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

## 3️⃣ Criar e Usar o Executor

```cpp
#include "include/execution/AlpacaOrderExecutor.hpp"

using namespace hft::orders::execution;

int main() {
    // PASSO 1: Criar o executor
    auto executor = std::make_unique<AlpacaOrderExecutor>(
        std::getenv("ALPACA_API_KEY"),
        "https://paper-api.alpaca.markets",  // paper = teste
        10  // agrupa 10 ordens antes de enviar
    );
    
    // PASSO 2: Iniciar background thread
    executor->initialize();
    
    // PASSO 3: Criar uma ordem
    Order order{
        .portfolio_id = 1,
        .event_id = 100,
        .delivery_id = 1,
        .asset_id = "AAPL",
        .quantity = 100.0,
        .price = 150.0,
        .side = "BUY"
    };
    
    // PASSO 4: Enviar (fire-and-forget = retorna logo!)
    if (executor->sendOrder(order)) {
        std::cout << "✓ Ordem enviada para fila\n";
    } else {
        std::cout << "✗ Ordem rejeitada\n";
    }
    
    // Background thread envia para Alpaca de forma assíncrona
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // PASSO 5: Parar quando terminas
    executor->shutdown();
    
    return 0;
}
```

## 4️⃣ Funções Principais

### sendOrder() - Enviar uma ordem
```cpp
bool result = executor->sendOrder(order);
```
- Retorna **imediatamente** (1-2 microsegundos)
- Alpaca (a bolsa) recebe em 50-200 ms
- Não bloqueia = thread-safe

### sendOrders() - Enviar vários de uma vez
```cpp
std::vector<Order> orders = {order1, order2, order3};
int count = executor->sendOrders(orders);
```

### getOrderStatus() - Ver estado de uma ordem
```cpp
OrderStatus status = executor->getOrderStatus(order_event_id);
if (status.is_valid) {
    std::cout << status.reason << "\n";
}
```

### cancelOrder() - Cancelar ordem
```cpp
bool cancelled = executor->cancelOrder(order_event_id);
```

## 5️⃣ Exemplo Prático: Comprar e Vender

```cpp
// Comprar 100 ações de AAPL a 150
Order buy{1, 1, 1, "AAPL", 100.0, 150.0, "BUY"};
executor->sendOrder(buy);

std::this_thread::sleep_for(std::chrono::milliseconds(100));

// Vender 50 ações a 152
Order sell{1, 2, 1, "AAPL", 50.0, 152.0, "SELL"};
executor->sendOrder(sell);
```

## 6️⃣ Dois Ambientes Alpaca

| Ambiente | URL | Usar Para |
|----------|-----|-----------|
| **Paper** (teste) | `https://paper-api.alpaca.markets` | Aprender, testar, demos |
| **Live** (real) | `https://api.alpaca.markets` | Dinheiro real |

## 7️⃣ Importante

- ✓ `sendOrder()` **não bloqueia** = rápido
- ✓ Thread background envia para Alpaca 
- ✗ Não há validação automática no orders = **tu validasdados antes de enviar**
- ✗ Precisa da API key no ambiente
- ✗ Paper trading = sem dinheiro real

## 🔗 Mais Info

- Exemplos: [examples/](../examples/)
- Testes: [tests/](../tests/)
- README: [../README.md](../README.md)
