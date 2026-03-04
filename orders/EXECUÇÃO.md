# Como Executar - HTTP Real ✨

Agora o Orders **faz COMPRAS REAIS** na tua Alpaca via HTTP!

## 📋 Pré-requisitos

### 1. Instalar libcurl (Obrigatório!)

**Ubuntu/Debian:**
```bash
sudo apt-get install libcurl4-openssl-dev
```

**macOS:**
```bash
brew install curl
```

**Windows (MSVC com vcpkg):**
```bash
vcpkg install curl:x64-windows
```

### 2. Instalar GTest (Opcional, para testes)

**Ubuntu/Debian:**
```bash
sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
```

**macOS:**
```bash
brew install googletest
```

## 🚀 Setup Alpaca API

### 1. Get API Key

1. Vai a https://app.alpaca.markets/
2. Login com tua conta
3. Settings → Copia **API Key**
4. Copia também **Secret Key** (se usares na estratégia)

### 2. Configure .env

```bash
# Copy template
cp .env.example .env

# Edit com teu API key
# Windows: notepad .env
# Linux:   nano .env
```

Resultado:
```bash
ALPACA_API_KEY="PKxxxxx..."
ALPACA_SECRET_KEY="xxxxxxxxxxxxxxxx"
ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

⚠️ NEVER commit `.env`!

## 🔨 Build

### Linux/Mac

```bash
# Build
./build.sh

# Output:
# ✓ example_fire_and_forget (executable)
# ✓ orders_test (if GTest installed)
```

### Windows PowerShell

```powershell
# Build
.\build.ps1

# Output:
# ✓ build/Release/example_fire_and_forget.exe
# ✓ build/Release/orders_test.exe (if GTest installed)
```

## ▶️ Executar

### Exemplo 1: Fire-and-Forget (Compas Reais!)

```bash
# Linux/Mac
./build/example_fire_and_forget

# Windows
.\build\Release\example_fire_and_forget.exe
```

**Output esperado:**
```
[AlpacaOrderExecutor] Initialized with base_url: https://paper-api.alpaca.markets
[AlpacaOrderExecutor] Started (fire-and-forget mode with real HTTP)
[AlpacaOrderExecutor] Order enqueued: AAPL BUY 100 @ $150.00
[AlpacaOrderExecutor] Processing batch of 1 orders
[AlpacaOrderExecutor] Order accepted: AAPL
✓ Order submitted to Alpaca
```

### Exemplo 2: Testes

```bash
# Linux/Mac (se GTest instalado)
./build/orders_test

# Windows
.\build\Release\orders_test.exe
```

## 🔍 Verificar Ordens

Depois de executar, verifica na dashboard Alpaca:

1. https://app.alpaca.markets/
2. **Dashboard → Orders**
3. Vê tua ordem em Pending ou Filled ✓

## 📊 O que está a fazer

```
Código C++                  Thread Background           Alpaca API
│                          │                           │
├─ sendOrder(buy)          │                           │
│  Enqueue                 │                           │
│  Retorna imediatamente   │                           │
│  (1-2 microsounds)       │                           │
│                          ├─ Batch (a cada 100ms)    │
│                          ├─ HTTP POST /v2/orders    │
│                          │                           ├─ Recebe
│                          │                           ├─ Cria ordem
│                          │                           └─ Responde JSON
│                          ├─ Parse response          │
│                          ├─ Log resultado           │
│                          └─ Continue...             │
└─ Segue a estratégia      └─ Sem bloquear!          └─ Orden criada!
```

## 🎯 Fluxo Completo

### Passo 1: Preparar
```bash
cp .env.example .env
# Edit .env com API key
```

### Passo 2: Build
```bash
./build.sh      # Linux/Mac
.\build.ps1     # Windows
```

### Passo 3: Executar
```bash
./build/example_fire_and_forget
```

### Passo 4: Verificar
```
✓ Check Alpaca dashboard
✓ Order should appear in ~50-200ms
✓ Status: Pending or Filled
```

## 🐛 Troubleshooting

### "ALPACA_API_KEY not set"
- Confirma que `.env` foi copiado
- Confirma que `.env` tem a chave
- Build scripts carregam `.env` automaticamente

### "Failed to init CURL"
- Instala libcurl:
  - Ubuntu: `sudo apt-get install libcurl4-openssl-dev`
  - macOS: `brew install curl`

### "Alpaca rejected: ..."
- API key pode ser inválida
- Formato JSON pode estar errado
- Alpaca API pode estar down
- Check logs para detalhes

### Build fails
- Confirma que tens libcurl instalado
- Run `cmake` fresh: `rm -rf build && ./build.sh`

## 📝 Example Code

Ver [examples/example_fire_and_forget.cpp](examples/example_fire_and_forget.cpp):

```cpp
// 1. Criar executor
auto executor = std::make_unique<AlpacaOrderExecutor>(
    std::getenv("ALPACA_API_KEY"),
  std::getenv("ALPACA_SECRET_KEY"),
    std::getenv("ALPACA_BASE_URL"),
    10  // batch size
);

// 2. Iniciar background thread
executor->initialize();

// 3. Enviar ordem (REAL!)
Order order{
    .portfolio_id = 1,
    .event_id = 100,
    .delivery_id = 1,
    .asset_id = "AAPL",
    .quantity = 100.0,
    .price = 150.0,
    .side = "BUY"
};

executor->sendOrder(order);  // ← Retorna imediatamente!
                             // ← HTTP vai ao background

// 4. Shutdown
executor->shutdown();
```

## 🚀 Próximos Passos

1. ✅ Compilar e testar
2. ✅ Verificar ordens na Alpaca
3. → Integrar com tua estratégia
4. → Testar em Paper Trading
5. → Deploy em Live (com cuidado!)

## ⚠️ Avisos

- **Paper Trading**: Sem dinheiro real, sempre ligado
- **Live Trading**: CUIDADO! Dinheiro real!
- **API Key**: NUNCA commit `.env`
- **Latência**: Espera ~50-200ms por ordem
- **Erros**: Check logs para debugging

## 📞 Suporte

Ver:
- [docs/API.md](docs/API.md) - API Reference
- [CONFIG_TEMPLATE.md](CONFIG_TEMPLATE.md) - Configuração
- [README.md](README.md) - Overview

Sucesso! 🎯
