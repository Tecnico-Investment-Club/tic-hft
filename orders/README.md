# HFT Orders System

Executor de ordens para a bolsa Alpaca com modelo **fire-and-forget**: enfileira em ~1-2 µs e envia para a Alpaca via HTTP em background (~50-200 ms).

---

## Índice

1. [Pré-requisitos](#1-pré-requisitos)
2. [Credenciais Alpaca](#2-credenciais-alpaca)
3. [Build](#3-build)
4. [Executar](#4-executar)
5. [API — Como usar o executor](#5-api--como-usar-o-executor)
6. [Arquitetura interna](#6-arquitetura-interna)
7. [Testes](#7-testes)
8. [Troubleshooting](#8-troubleshooting)
9. [Estrutura do projeto](#9-estrutura-do-projeto)
10. [Limitações e próximos passos](#10-limitações-e-próximos-passos)

---

## 1. Pré-requisitos

### WSL / Linux

```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libgtest-dev
```

### macOS

```bash
brew install cmake curl googletest
```

### Windows (PowerShell + vcpkg)

```powershell
vcpkg install curl:x64-windows
# CMake no PATH e toolchain C++ instalado
```

---

## 2. Credenciais Alpaca

O binário lê as credenciais de variáveis de ambiente exportadas no shell. **Não lê o `.env` automaticamente** — é preciso carregá-lo antes de executar.

### Criar `.env`

```bash
cp .env.example .env
# Editar com a tua key:
# Linux:   nano .env
# Windows: notepad .env
```

Conteúdo esperado:

```bash
ALPACA_API_KEY="PKxxxxx..."
ALPACA_SECRET_KEY="xxxxxxxxxxxxxxxx"
ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

> ⚠️ Nunca commites o ficheiro `.env` — está no `.gitignore`.

### Exportar para o shell antes de executar

```bash
# WSL / Linux — carrega .env e exporta tudo
source ./env.sh

# Verificar que ficaram exportadas
printenv ALPACA_API_KEY
printenv ALPACA_SECRET_KEY
printenv ALPACA_BASE_URL
```

> O problema mais comum de `terminate called after throwing std::logic_error` é correr `./example_fire_and_forget` num shell onde as variáveis **não** estão exportadas. `source ./env.sh` resolve.

### Alternativa: exportar manualmente

```bash
# Linux / Mac
export ALPACA_API_KEY="PKxxxxx..."
export ALPACA_SECRET_KEY="xxxxxxxxxxxxxxxx"
export ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

```powershell
# Windows PowerShell
$env:ALPACA_API_KEY="PKxxxxx..."
$env:ALPACA_SECRET_KEY="xxxxxxxxxxxxxxxx"
$env:ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

### Ambientes Alpaca

| Ambiente | URL | Usar para |
|----------|-----|-----------|
| **Paper** (recomendado) | `https://paper-api.alpaca.markets` | Testes, demos |
| **Live** (dinheiro real) | `https://api.alpaca.markets` | Produção |

---

## 3. Build

### Linux / Mac (script)

```bash
./build.sh            # build Release + testes
./build.sh --no-tests # só build
./build.sh --debug    # build Debug
```

### Windows PowerShell (script)

```powershell
.\build.ps1                        # build Release + testes
.\build.ps1 -NoTests               # só build
.\build.ps1 -BuildType Debug       # build Debug
```

### Manual CMake

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

---

## 4. Executar

As variáveis têm de estar exportadas **no mesmo shell** (ver [secção 2](#2-credenciais-alpaca)).

```bash
# Opção A: carregar env e executar direto
source ./env.sh
./build/example_fire_and_forget

# Opção B: env.sh passa o binário como argumento (exec)
./env.sh build/example_fire_and_forget
```

**Output esperado:**

```
=== HFT Orders System - Fire-and-Forget Example ===
API Key loaded: ✓ OK
Secret Key loaded: ✓ OK
[AlpacaOrderExecutor] API Key: PK6D****4JRV (length: 26)
[AlpacaOrderExecutor] Started (fire-and-forget mode with real HTTP)
Order 1: BUY 100 AAPL @ $150
[AlpacaOrderExecutor] Order enqueued: AAPL BUY 100.00 @ $150.00
Dispatch latency: 11 μs
```

---

## 5. API — Como usar o executor

### Struct `Order`

```cpp
struct Order {
    uint64_t    portfolio_id;  // ex: 1
    uint64_t    event_id;      // ID único da ordem
    uint64_t    delivery_id;   // lote
    std::string asset_id;      // símbolo, ex: "AAPL"
    double      quantity;      // número de ações
    double      price;         // preço por ação
    std::string side;          // "BUY" ou "SELL"
};
```

### Criar e ligar o executor

```cpp
#include "include/execution/AlpacaOrderExecutor.hpp"
using namespace hft::orders::execution;

auto executor = std::make_unique<AlpacaOrderExecutor>(
    std::getenv("ALPACA_API_KEY"),
    std::getenv("ALPACA_SECRET_KEY"),
    "https://paper-api.alpaca.markets",
    10  // max_batch_size
);

executor->initialize();  // arranca background thread
```

> As variáveis têm de estar exportadas **antes** de executar o binário; `getenv` devolve `nullptr` se não estiverem, causando crash.

### Métodos principais

#### `sendOrder()` — envia uma ordem (fire-and-forget)

```cpp
bool ok = executor->sendOrder(order);
// Retorna em ~1-2 µs; Alpaca recebe em ~50-200 ms
```

#### `sendOrders()` — envia vários de uma vez

```cpp
std::vector<Order> orders = {order1, order2, order3};
int count = executor->sendOrders(orders);
```

#### `getOrderStatus()` — estado de uma ordem

```cpp
OrderStatus status = executor->getOrderStatus(order.event_id);
if (status.is_valid) std::cout << status.reason << "\n";
```

#### `cancelOrder()` — cancelar

```cpp
bool cancelled = executor->cancelOrder(order.event_id);
```

#### `shutdown()` — parar quando terminar

```cpp
executor->shutdown();
```

### Exemplo mínimo completo

```cpp
Order buy{
    .portfolio_id = 1,
    .event_id     = 100,
    .delivery_id  = 1,
    .asset_id     = "AAPL",
    .quantity     = 100.0,
    .price        = 150.0,
    .side         = "BUY"
};
executor->sendOrder(buy);

std::this_thread::sleep_for(std::chrono::seconds(1));
executor->shutdown();
```

---

## 6. Arquitetura interna

```
Código C++                 Thread Background          Alpaca API
│                         │                          │
├─ sendOrder(order)       │                          │
│  push → pending_orders_ │                          │
│  retorna ~1 µs          │                          │
│                         ├─ process_batch() ~100ms  │
│                         ├─ GET /v2/account         │
│                         ├─ POST /v2/orders ────────►
│                         │                          ├─ cria ordem
│                         ◄──────────── JSON resp ───┤
│                         ├─ log resultado           │
└─ continua estratégia    └─ sem bloquear!           └─ ordem criada
```

### Ficheiros-chave

| Ficheiro | Função |
|----------|--------|
| `include/execution/IExecution.hpp` | Interface + `Order` + `OrderStatus` |
| `include/execution/AlpacaOrderExecutor.hpp` | Declaração da classe |
| `src/execution/AlpacaOrderExecutor.cpp` | Implementação completa |
| `examples/example_fire_and_forget.cpp` | Exemplo de uso |
| `tests/orders_test.cpp` | Suite de testes |

### Ciclo de vida do executor

- `initialize()` — arranca `executor_thread_`, activa o loop
- `executor_loop()` — chama `process_batch()` a cada ~100 ms enquanto `running_`
- `process_batch()` — tira até `max_batch_size` ordens da fila e chama `submit_to_alpaca()` para cada uma
- `submit_to_alpaca()` — valida keys → `GET /v2/account` → `POST /v2/orders` → considera sucesso se resposta contém `"id"`
- `shutdown()` — `running_ = false` + `join()` da thread

### Concorrência e thread-safety

- Fila `pending_orders_` protegida por `pending_orders_mutex_`
- Mapa `sent_orders_` protegido por `sent_orders_mutex_`
- Flag `running_` com `std::atomic<bool>`

### Headers HTTP usados

- `APCA-API-KEY-ID`
- `APCA-API-SECRET-KEY`
- `accept: application/json`
- `content-type: application/json` (POST)

> `side` é normalizado para minúsculas (`buy`/`sell`) antes do envio.

---

## 7. Testes

Os testes fazem `SKIP` automático se `ALPACA_API_KEY`/`ALPACA_SECRET_KEY` não estiverem no ambiente.

### Correr todos os testes

```bash
source ./env.sh
cd build
ctest --output-on-failure
# ou directamente:
./orders_test
```

### Listar testes disponíveis

```bash
./orders_test --gtest_list_tests
```

### Executar um teste específico

```bash
./orders_test --gtest_filter=AlpacaOrderExecutorTest.SendSingleOrder
```

### Verificar conectividade Alpaca manualmente

```bash
source ./env.sh
curl -s \
  -H "accept: application/json" \
  -H "APCA-API-KEY-ID: $ALPACA_API_KEY" \
  -H "APCA-API-SECRET-KEY: $ALPACA_SECRET_KEY" \
  "${ALPACA_BASE_URL}/v2/account"
# 200 = autenticado; 401 = key inválida ou não exportada
```

---

## 8. Troubleshooting

### `terminate called … basic_string: construction from null is not valid`

`getenv()` devolveu `nullptr` porque as variáveis não estão exportadas no shell.

```bash
source ./env.sh       # exporta as variáveis
./build/example_fire_and_forget
```

### `No .env found`

```bash
cp .env.example .env
# editar com as tuas credenciais
```

### `unauthorized` (401 da Alpaca)

- Confirma que estás no URL paper correcto
- Confirma que `source ./env.sh` foi feito no **mesmo** shell onde corres o binário
- Testa com `curl` (ver secção [Verificar conectividade](#7-testes))

### `cannot open source file curl/curl.h`

Dependência em falta:
```bash
sudo apt-get install libcurl4-openssl-dev   # Ubuntu/Debian
brew install curl                            # macOS
```

### Build falha

```bash
rm -rf build
./build.sh
```

### `Clock skew detected` no WSL

Aviso de timestamps entre Windows/WSL; não bloqueia build nem testes.

---

## 9. Estrutura do projeto

```
orders/
├── .env.example                  # Template de credenciais (copiar para .env)
├── .gitignore                    # Ignora .env
├── env.sh                        # Carrega .env e exporta variáveis no shell
├── build.sh                      # Build script Linux/Mac
├── build.ps1                     # Build script Windows PowerShell
├── CMakeLists.txt                # Build config (libcurl obrigatório)
├── include/execution/
│   ├── IExecution.hpp            # Interface + struct Order + OrderStatus
│   └── AlpacaOrderExecutor.hpp   # Declaração do executor
├── src/execution/
│   └── AlpacaOrderExecutor.cpp   # Implementação com HTTP real (libcurl)
├── examples/
│   └── example_fire_and_forget.cpp
├── tests/
│   └── orders_test.cpp
└── build/                        # Artefactos compilados (gerado pelo CMake)
```

**CMake targets:**
- `hft-orders` — biblioteca estática
- `example_fire_and_forget` — executável do exemplo
- `orders_test` — suite GTest (só se GTest estiver instalado)

---

## 10. Limitações e próximos passos

**Limitações actuais:**
- Estado em memória; sem persistência durável
- Sem retry/backoff para falhas transitórias (429/5xx)
- Critério de sucesso por substring `"id"` na resposta
- Testes são integration-like (podem tocar rede quando env existe)

**Próximos passos recomendados:**
- Adicionar status code HTTP explícito nos logs
- Separar testes unitários de testes de integração real
- Adicionar retries com backoff exponencial
- Guardar estado com mais detalhe (`attempts`, `last_error`)

## 🚀 Rápido Start (5 min)

### 1. Setup Alpaca API Key

```bash
# Copy template
cp .env.example .env

# Edit .env with your key
# Windows: notepad .env
# Linux:   nano .env
source env.sh
```

See [CONFIG_TEMPLATE.md](CONFIG_TEMPLATE.md) for details.

### 2. Build
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 3. Run Example
```bash
./example_fire_and_forget
```

## 📖 Como Usar

Lê [docs/API.md](docs/API.md) - tem tudo explicado passo a passo:
- Como criar uma ordem
- Como enviar
- Funções disponíveis
- Exemplos práticos

## 🧪 Testes
```bash
# Com GTest instalado:
cmake -DBUILD_TESTS=ON ..
cmake --build . --config Release
./orders_test
```

## 📁 Estrutura
```
orders/
├── .env.example          # Template (copy to .env)
├── CONFIG_TEMPLATE.md    # Como setup variáveis
├── .gitignore           # Ignora .env
├── include/execution/    # IExecution + AlpacaOrderExecutor
├── src/execution/        # Implementação
├── examples/             # Exemplos de uso
├── tests/                # Unit tests
├── docs/API.md          # Documentação completa
└── build/               # Compilado aqui
```

## ⚡ O Essencial

**sendOrder()** = envia 1 ordem
```cpp
executor->sendOrder(order);  // Retorna logo! (1-2 μs)
```

**Background thread** = envia para Alpaca (50-200 ms)

**Pronto!** = já está!

## 🔗 Links
- [CONFIG_TEMPLATE.md](CONFIG_TEMPLATE.md) - Setup da API Key
- [API.md](docs/API.md) - Documentação completa
- [examples/](examples/) - Mais exemplos
- [tests/](tests/) - Como testar
