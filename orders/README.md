# HFT Orders System

Envia ordens para a bolsa (Alpaca) de forma rápida. Fire-and-forget = retorna logo!

## 🚀 Rápido Start (5 min)

### 1. Setup Alpaca API Key

```bash
# Copy template
cp .env.example .env

# Edit .env with your key
# Windows: notepad .env
# Linux:   nano .env
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
