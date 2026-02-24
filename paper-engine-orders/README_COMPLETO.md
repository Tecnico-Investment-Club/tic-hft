# Paper Engine Orders - Guia Completo

## 📋 Índice
1. [Visão Geral](#visão-geral)
2. [Arquitetura](#arquitetura)
3. [Setup Inicial](#setup-inicial)
4. [Como Correr](#como-correr)
5. [Componentes Principais](#componentes-principais)
6. [Fluxo de Dados](#fluxo-de-dados)
7. [Testes & Debugging](#testes--debugging)
8. [Troubleshooting](#troubleshooting)

---

## Visão Geral

**Paper Engine Orders** é um sistema de **simulação de trading (paper trading)** que:
- Lê dados de mercado do PostgreSQL (populado pelo ETL)
- Executa estratégias de trading
- Submete ordens **simuladas** no Alpaca (sem dinheiro real)
- Persiste as ordens no PostgreSQL

### Equivalente Python
Este projeto é a versão em **C++** do `tic-strategy-app/alpaca/paper-engine-orders` (Python).

### Broker
- **Alpaca Markets** (stocks e crypto)
- **Paper Trading API** (simulação segura)
- URL: `https://paper-api.alpaca.markets`

---

## Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                       ETL (tic-data-etl)                    │
│              (Binance, Alpaca → PostgreSQL)                 │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                      PostgreSQL                             │
│  • Klines (OHLCV)  • Market Data  • Paper Trading Orders    │
└────────┬──────────────────────────────────┬─────────────────┘
         │                                  │
    (Leitura)                          (Escrita)
         │                                  │
         ↓                                  ↓
┌─────────────────────────────────────────────────────────────┐
│              Paper Engine Orders (C++)                       │
│                                                              │
│  1. DatabaseSource (Lê dados do ETL)                        │
│  2. PaperEngineOrders (Estratégia)                          │
│  3. AlpacaClient (Submete ordens)                           │
│  4. DatabaseTarget (Persiste ordens)                        │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                    Alpaca API                               │
│              (Paper Trading - Simulação)                    │
│         • Submete ordens  • Executa trades  • Reports       │
└─────────────────────────────────────────────────────────────┘
```

## Setup Inicial

### 1️⃣ Pré-requisitos

```powershell
# PostgreSQL 14+ (já deve estar instalado)
psql --version

# Docker & Docker Compose (para containerização)
docker --version
docker-compose --version
```

### 2️⃣ Configurar Variáveis de Ambiente

**Copiar template:**
```powershell
cd tic-hft\paper-engine-orders
Copy-Item .env.example .env
```

**Editar `.env` com os teus valores:**
```ini
# Database
DB_SOURCE_CONNECTION=postgresql://hft_user:tua_senha_aqui@localhost:5432/hft_db
DB_TARGET_CONNECTION=postgresql://hft_user:tua_senha_aqui@localhost:5432/hft_db

# Alpaca
ALPACA_API_KEY=pk_live_abc123...  # Obtém em https://app.alpaca.markets
ALPACA_BASE_URL=https://paper-api.alpaca.markets/v2

# Estratégia
PORTFOLIO_ID=1
STRATEGY_ID=1
CASH_ALLOCATION=1000.0
```

### 3️⃣ Setup do PostgreSQL

**Criar user e database:**
```powershell
# Como admin (postgres)
psql -U postgres -c "CREATE USER hft_user WITH PASSWORD 'tua_senha_aqui';"
psql -U postgres -c "CREATE DATABASE hft_db OWNER hft_user;"
```

**Carregar schema:**
```powershell
psql -U postgres -d hft_db -f "db/schema.sql"
```

**Dar permissões:**
```powershell
# Coloca a password do postgres quando pedido
psql -U postgres -d hft_db -c "GRANT ALL PRIVILEGES ON SCHEMA hft_paper_engine TO hft_user; GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA hft_paper_engine TO hft_user;"
```

**Verificar:**
```powershell
$env:PGPASSWORD='tua_senha_aqui'
psql -U hft_user -d hft_db -h 127.0.0.1 -c "SELECT COUNT(*) FROM hft_paper_engine.orders;"
```

---

## Como Correr

### Opção A: Docker Compose (Recomendado)

**Mais simples, tudo isolado:**

```powershell
cd tic-hft\paper-engine-orders

# Correr tudo (DB + App)
docker-compose up --build

# Em outro terminal, ver logs
docker logs -f paper_engine_orders
```

**O que acontece:**
- PostgreSQL inicia automaticamente
- Schema é carregado
- Paper Engine Orders compila e arranca
- Tudo conectado automaticamente

### Opção B: Manual (Sem Docker)

**Se preferires compilar localmente:**

```powershell
cd tic-hft\paper-engine-orders

# Build
mkdir build
cd build
cmake ..
make -j4

# Correr
./paper-engine-orders --dry-run
```

---

## Componentes Principais

### 1. **DatabaseSource** (`src/persistance/DatabaseSource.cpp`)
Conecta-se ao PostgreSQL e **lê dados do ETL**.

```cpp
DatabaseSource source("postgresql://hft_user:pass@localhost:5432/hft_db");
source.connect();

// Ler dados de Klines
auto results = source.query("SELECT * FROM binance_klines WHERE symbol='BTCUSDT'");
```

**Tabelas que lê:**
- `binance_klines` - OHLCV data (Binance)
- `alpaca_klines` - OHLCV data (Alpaca)
- Qualquer tabela do ETL

### 2. **PaperEngineOrders** (`src/PaperEngineOrders.cpp`)
**Orquestrador principal** - executa a estratégia.

```cpp
PaperEngineOrders engine(
    db_source_conn,
    db_target_conn,
    alpaca_api_key,
    portfolio_id
);

engine.connect();
engine.run();  // Loop principal
```

**O que faz:**
1. Lê dados de mercado (DatabaseSource)
2. Calcula sinais de compra/venda
3. Valida ordens
4. Submete ao Alpaca
5. Persiste em PostgreSQL

### 3. **AlpacaClient** (Integração)
Cliente HTTP para **Alpaca API**.

```cpp
AlpacaClient alpaca(api_key, base_url);

// Submeter ordem
alpaca.submit_order({
    "symbol": "AAPL",
    "qty": 10,
    "side": "buy",
    "type": "market"
});

// Obter status de ordens
auto orders = alpaca.get_orders();
```

### 4. **DatabaseTarget** (`src/persistance/DatabaseTarget.cpp`)
Persiste as ordens no PostgreSQL.

```cpp
DatabaseTarget target("postgresql://...");
target.connect();

// Inserir ordem
target.insert_order({
    "portfolio_id": 1,
    "symbol": "AAPL",
    "qty": 10,
    "status": "FILLED"
});
```

**Tabelas que escreve:**
- `hft_paper_engine.orders` - Orders executadas
- `hft_paper_engine.orders_config` - Configuração da estratégia
- `hft_paper_engine.orders_control` - Controlo de rebalanceamento
- `hft_paper_engine.orders_latest` - Última ordem por ativo

---

## Fluxo de Dados

### Ciclo Completo:

```
1. ETL popula PostgreSQL
   └─ Binance → tic-data-etl → PostgreSQL (binance_klines)
   
2. DatabaseSource lê dados
   └─ SELECT * FROM binance_klines
   
3. PaperEngineOrders processa
   └─ Calcula sinais (RSI, MACD, etc)
   └─ Valida ordens
   └─ Cria estruturas de ordem
   
4. AlpacaClient submete
   └─ POST /v2/orders
   └─ Alpaca executa (simulação)
   
5. DatabaseTarget persiste
   └─ INSERT INTO hft_paper_engine.orders
   └─ UPDATE orders_control
```

### Exemplo de Ordem:

```sql
-- Order submetida
INSERT INTO hft_paper_engine.orders (
    portfolio_id,
    side,
    asset_id_type,
    asset_id,
    order_ts,
    quantity,
    status,
    event_id,
    delivery_id
) VALUES (
    1,                    -- portfolio_id
    'BUY',                -- side
    'SYMBOL',             -- asset_id_type
    'AAPL',               -- asset_id
    '2026-02-24 10:30:00',-- order_ts
    10.0,                 -- quantity
    'FILLED',             -- status
    1001,                 -- event_id (Alpaca)
    2001                  -- delivery_id
);
```

---

## Testes & Debugging

### 1. Testar Conexão DB

```powershell
$env:PGPASSWORD='tua_senha_aqui'
psql -U hft_user -d hft_db -h 127.0.0.1 -c "SELECT * FROM hft_paper_engine.orders LIMIT 5;"
```

### 2. Ver Logs da Aplicação

**Com Docker:**
```powershell
docker logs -f paper_engine_orders
```

**Manual:**
```powershell
# Aplicação imprime logs em stdout
./paper-engine-orders --dry-run
```

### 3. Testar Alpaca API

**Verificar API Key:**
```powershell
# Adiciona à .env e testa
curl -H "APCA-API-KEY-ID: $env:ALPACA_API_KEY" ^
     https://paper-api.alpaca.markets/v2/account
```

### 4. Modo Dry-Run

```powershell
# Simula tudo SEM submeter ordens reais
./paper-engine-orders --dry-run

# Output:
# [INFO] Dry-run mode enabled
# [INFO] Connected to PostgreSQL
# [INFO] Connected to Alpaca
# [INFO] Fetching market data...
# [INFO] Simulating orders (not submitting)
```

### 5. Verifica Queries SQL

**Todas as queries estão em `src/queries/OrdersQueries.cpp`:**

```cpp
// Ler Klines
SELECT symbol, high, low, close FROM binance_klines 
WHERE symbol = ? AND timestamp > ?

// Inserir Ordem
INSERT INTO hft_paper_engine.orders (...) VALUES (...)

// Atualizar Status
UPDATE hft_paper_engine.orders SET status = ? WHERE event_id = ?
```

---

## Troubleshooting

### ❌ "Connection refused" (PostgreSQL)

**Problema:** PostgreSQL não está a rodar

**Solução:**
```powershell
# Verificar se postgres está ativo
Get-Service postgresql* | Select-Object Name, Status

# Se não estiver, iniciar
Start-Service postgresql-x64-18

# Ou usar Docker
docker run -d -p 5432:5432 -e POSTGRES_PASSWORD=postgres postgres:15
```

### ❌ "Password authentication failed"

**Problema:** Password incorreta ou user não existe

**Solução:**
```powershell
# Verificar user existe
psql -U postgres -c "\du"

# Reset password
psql -U postgres -c "ALTER USER hft_user WITH PASSWORD 'nova_senha';"

# Actualizar .env
PGPASSWORD='nova_senha'
```

### ❌ "Table does not exist"

**Problema:** Schema não foi carregado

**Solução:**
```powershell
# Verificar tabelas
psql -U hft_user -d hft_db -c "\dt hft_paper_engine.*"

# Se vazio, carregar schema
psql -U postgres -d hft_db -f db/schema.sql
```

### ❌ Docker build fails

**Problema:** Dependências C++ não instaladas

**Solução:**
```powershell
# Recompilar
docker-compose up --build --no-cache

# Ou verificar logs
docker-compose logs -f
```

### ❌ Alpaca API errors

**Problema:** API Key inválida ou expirada

**Solução:**
1. Vai a https://app.alpaca.markets
2. Account Settings → API Keys
3. Gera nova key
4. Actualiza `.env`
5. Reinicia container

### ❌ Ordens não aparecem em PostgreSQL

**Problema:** DatabaseTarget não está a persistir

**Solução:**
```powershell
# Verifica logs
docker logs paper_engine_orders | grep -i "insert\|error\|database"

# Verifica permissões
psql -U hft_user -d hft_db -c "INSERT INTO hft_paper_engine.orders (...)"

# Se error: GRANT permissions novamente
```

---

## Estrutura do Projeto

```
paper-engine-orders/
├── README.md                      # README original
├── README_COMPLETO.md             # Este ficheiro
├── docker-compose.yml              # Configuração Docker
├── Dockerfile                      # Build da imagem
├── CMakeLists.txt                  # Build C++
├── .env                            # Variáveis (PRIVADO)
├── .env.example                    # Template
├── db/
│   └── schema.sql                  # Tabelas PostgreSQL
├── include/
│   ├── PaperEngineOrders.hpp       # Orquestrador
│   ├── persistance/
│   │   ├── DatabaseSource.hpp      # Leitura DB
│   │   └── DatabaseTarget.hpp      # Escrita DB
│   ├── model/
│   │   └── Order.hpp               # Estrutura de Ordem
│   └── queries/
│       └── OrdersQueries.hpp       # Queries SQL
└── src/
    ├── main.cpp                    # Entry point
    ├── PaperEngineOrders.cpp       # Implementação
    ├── persistance/
    │   ├── DatabaseSource.cpp
    │   └── DatabaseTarget.cpp
    ├── queries/
    │   └── OrdersQueries.cpp
    └── model/
        └── Order.cpp
```

---

## Next Steps

### 1. Validar Setup
```powershell
# Verifica tudo a funcionar
docker ps
psql -U hft_user -d hft_db -c "SELECT 1"
curl -H "APCA-API-KEY-ID: $env:ALPACA_API_KEY" https://paper-api.alpaca.markets/v2/account
```

### 2. Correr ETL
```powershell
cd ../../tic-data-etl
# Popula PostgreSQL com dados reais
```

### 3. Correr Paper Engine
```powershell
cd ../tic-hft/paper-engine-orders
docker-compose up --build
```

### 4. Monitorar
```powershell
# Terminal 1: Logs
docker logs -f paper_engine_orders

# Terminal 2: DB
psql -U hft_user -d hft_db -c "SELECT COUNT(*) FROM hft_paper_engine.orders;"

# Terminal 3: Alpaca
curl -s -H "APCA-API-KEY-ID: $env:ALPACA_API_KEY" \
     https://paper-api.alpaca.markets/v2/orders | jq '.'
```

---

## Referências

- **Alpaca Docs**: https://docs.alpaca.markets/
- **PostgreSQL**: https://www.postgresql.org/docs/
- **pqxx (C++ PostgreSQL)**: http://pqxx.org/
- **C++17**: https://en.cppreference.com/

---

## Suporte

Se tiveres problemas:
1. Verifica os logs: `docker logs -f paper_engine_orders`
2. Verifica DB: `psql -U hft_user -d hft_db -c "SELECT * FROM hft_paper_engine.orders;"`
3. Verifica Alpaca: Testa key em https://app.alpaca.markets
4. Recria containers: `docker-compose down && docker-compose up --build`
