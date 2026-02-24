# HFT Paper Engine Orders (Alpaca)

## Overview

Paper Engine Orders é um sistema de trading de papel (paper trading) para o HFT (High-Frequency Trading) que replicaão arquitetura do `tic-strategy-app/alpaca/paper-engine-orders` em C++.

**Broker**: Alpaca (stocks e crypto)

## Arquitetura

### Componentes Principais

1. **DatabaseSource**: Conecta-se ao PostgreSQL para ler dados do ETL
   - Equivalente ao `source.Source` do Alpaca
   - Executa queries para recuperar dados de mercado

2. **DatabaseTarget**: Persiste ordens no PostgreSQL
   - Equivalente ao `target.Target` do Alpaca
   - Escreve estados de ordens e eventos

3. **PaperEngineOrders**: Orquestrador principal
   - Coordena leitura do ETL
   - Executa lógica de estratégia
   - Gerencia ordens através do Alpaca API

4. **Alpaca Client**: Interface com broker
   - Submete ordens no Alpaca
   - Recupera status de ordens
   - Alpaca paper trading API

### Fluxo de Dados

```
ETL (tic-data-etl)
    ↓
PostgreSQL (Source)
    ↓
PaperEngineOrders (Strategy Logic)
    ↓
Alpaca API (Broker)
    ↓
PostgreSQL (Target - Persistence)
```

## Como Conecta ao ETL

1. **Dados do ETL**: Armazenados em PostgreSQL populado por `tic-data-etl`
2. **Leitura**: `DatabaseSource` consulta tabelas do ETL (Klines, market data)
3. **Processamento**: `PaperEngineOrders` executa estratégia nos dados
4. **Escrita**: `DatabaseTarget` persiste ordens em tabelas de paper engine

## Database Schema

### Tabela: hft_paper_engine.orders

Espelha a estrutura de `paper_engine.orders` do Alpaca:

```sql
- portfolio_id: Identificador do portfólio
- side: BUY/SELL
- asset_id_type: CRYPTO_TICKER/STOCK_TICKER
- asset_id: Símbolo do ativo (e.g., BTCUSDT)
- order_ts: Timestamp da ordem
- quantity: Quantidade
- notional: Quantidade * Preço
- target_wgt/real_wgt: Pesos da alocação
- status: PENDING/NEW/FILLED/CANCELED/ERROR
- event_id: ID único da ordem
- delivery_id: ID de entrega
- hash: Hash para integridade
- binance_order_id: ID da ordem no Binance
```

## Estrutura do Projeto

```
tic-hft/paper-engine-orders/
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yml
├── README.md
├── include/
│   ├── PaperEngineOrders.hpp
│   ├── persistance/
│   │   ├── DatabaseSource.hpp
│   │   └── DatabaseTarget.hpp
│   ├── model/
│   │   └── Order.hpp
│   └── queries/
│       └── OrdersQueries.hpp
├── src/
│   ├── main.cpp
│   ├── PaperEngineOrders.cpp
│   ├── persistance/
│   │   ├── DatabaseSource.cpp
│   │   └── DatabaseTarget.cpp
│   └── queries/
│       └── OrdersQueries.cpp
└── db/
    └── schema.sql
```

## Build & Run

### Com Docker Compose

```bash
cd tic-hft/paper-engine-orders
docker-compose up --build
```

### Build Manual

```bash
mkdir build
cd build
cmake ..
make
./paper-engine-orders --dry-run
```

### Variáveis de Ambiente

```bash
export DB_SOURCE_CONNECTION="postgresql://user:pass@localhost:5432/hft_db"
export DB_TARGET_CONNECTION="postgresql://user:pass@localhost:5432/hft_db"
export ALPACA_API_KEY=\"your_alpaca_token\"
export ALPACA_BASE_URL=\"https://paper-api.alpaca.markets\"
export PORTFOLIO_ID=1
export STRATEGY_ID=1
export CASH_ALLOCATION=1000.0
export DRY_RUN=true
```

### Opções de Linha de Comando

```bash
paper-engine-orders --help
  --portfolio-id ID       Portfolio ID (default: 1)
  --strategy-id ID        Strategy ID (default: 1)
  --cash-allocation AMOUNT Cash allocation in USD
  --dry-run               Dry-run mode (no real orders)
  --help                  Show help
```

## Comparação com Alpaca Python

| Componente | Alpaca (Python) | HFT (C++) |
|-----------|-----------------|-----------|
| DB Source | `source.Source` | `DatabaseSource` |
| DB Target | `target.Target` | `DatabaseTarget` |
| Broker | `Alpaca` (API) | `Alpaca` (API) |
| Orquestrador | `Loader` | `PaperEngineOrders` |
| Persistência | psycopg2 | libpqxx |
| Entry Point | `__main__.py` | `main.cpp` |

## Próximos Passos

1. Implementar sincronização com RingBuffer do ETL
2. Adicionar lógica de rebalanceamento de portfólio
3. Implementar métricas e reporting
4. Adicionar suporte a múltiplas estratégias
5. Otimizar performance para latência ultra-baixa
