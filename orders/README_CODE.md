# README_CODE

Guia de arquitetura e funcionamento do projeto `orders`.

## 1) O que este projeto faz

Este módulo implementa um executor de ordens com modelo **fire-and-forget**:

- recebe ordens (`Order`) por chamada de método;
- enfileira rapidamente (sem bloquear rede);
- processa em background;
- envia para Alpaca via HTTP (`libcurl`);
- mantém um registo interno básico das ordens enviadas.

Não existe endpoint HTTP de entrada neste repositório. A entrada é por objetos C++ (`Order`, `std::vector<Order>`).

## 2) Estrutura principal

- Interface base: [include/execution/IExecution.hpp](include/execution/IExecution.hpp)
- Implementação Alpaca: [include/execution/AlpacaOrderExecutor.hpp](include/execution/AlpacaOrderExecutor.hpp)
- Lógica principal: [src/execution/AlpacaOrderExecutor.cpp](src/execution/AlpacaOrderExecutor.cpp)
- Exemplo de uso: [examples/example_fire_and_forget.cpp](examples/example_fire_and_forget.cpp)
- Testes: [tests/orders_test.cpp](tests/orders_test.cpp)
- Build config: [CMakeLists.txt](CMakeLists.txt)

## 3) Modelo de dados

A ordem é definida em `IExecution` com campos como:

- `portfolio_id`, `event_id`, `delivery_id`
- `asset_id`
- `quantity`, `price`
- `side` (`BUY`/`SELL`)

O executor guarda ordens submetidas num `std::map<uint64_t, Order>` indexado por `event_id`.

## 4) Ciclo de vida do executor

Métodos relevantes em `AlpacaOrderExecutor`:

- `initialize()`
  - valida estado;
  - arranca `executor_thread_`;
  - ativa loop de processamento.

- `shutdown()`
  - sinaliza `running_ = false`;
  - faz `join()` da thread.

## 5) Pipeline de execução

### Entrada

- `sendOrder(order)`
- `sendOrders(vector<Order>)`

Ambos colocam ordens na fila `pending_orders_` com mutex e retornam rápido.

### Background

A thread chama `executor_loop()`:

1. chama `process_batch()`;
2. dorme ~100ms;
3. repete enquanto `running_`.

`process_batch()` tira até `max_batch_size` ordens da fila e chama `submit_to_alpaca(order)` para cada uma.

## 6) Integração Alpaca (HTTP)

No `submit_to_alpaca`:

1. valida `api_key_` e `secret_key_`;
2. faz `GET /v2/account` para teste simples de conta;
3. faz `POST /v2/orders` com payload market/day;
4. considera sucesso quando resposta contém `"id"`.

Headers usados:

- `APCA-API-KEY-ID`
- `APCA-API-SECRET-KEY`
- `accept: application/json`
- `content-type: application/json` (POST)

Nota: `side` é normalizado para minúsculas antes do envio (`buy`/`sell`).

## 7) Concorrência e thread-safety

- Fila de pendentes protegida por `pending_orders_mutex_`.
- Mapa de enviadas protegido por `sent_orders_mutex_`.
- Flag de execução com `std::atomic<bool> running_`.

## 8) Como as credenciais entram no sistema

O `AlpacaOrderExecutor` recebe credenciais pelo construtor (`api_key`, `secret_key`, `base_url`).

Em runtime, a forma recomendada é carregar `.env` no shell e passar as variáveis ao criar o executor (como no exemplo).

Script utilitário para carregar `.env` no WSL:

- [env.sh](env.sh)

## 9) Build e targets

Definidos no CMake:

- biblioteca estática `hft-orders`
- executável `example_fire_and_forget`
- executável `orders_test` (se GTest existir)

Dependência obrigatória: `CURL`.

## 10) Limitações atuais (importante)

- Não há persistência durável (estado em memória).
- Não há retry/backoff para falhas transitórias.
- Critério de sucesso por substring (`"id"`) é simples.
- Testes são “integration-like” (podem tocar rede) quando env existe.

## 11) Próximos passos recomendados

- adicionar status code HTTP explícito em logs;
- separar testes unitários de integração real;
- adicionar retries para `429/5xx`;
- guardar estado com mais detalhe (`attempts`, `last_error`).
