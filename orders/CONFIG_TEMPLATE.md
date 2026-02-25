# Configuration

## Setup (2 steps)

### 1. Copy template to .env
```bash
cp .env.example .env
```

### 2. Edit .env with your values
```bash
# Windows PowerShell
notepad .env

# Linux/Mac
nano .env
```

## Environment Variables

### ALPACA_API_KEY (Required)
Your API key from https://app.alpaca.markets/

```bash
ALPACA_API_KEY="PKxxxxx..."
```

### ALPACA_BASE_URL (Required)
Which Alpaca environment to use.

**Paper Trading** (test, always available):
```bash
ALPACA_BASE_URL="https://paper-api.alpaca.markets"
```

**Live Trading** (real money, market hours only):
```bash
ALPACA_BASE_URL="https://api.alpaca.markets"
```

## Using in Code

Automatically loaded from `.env`:

```cpp
auto executor = std::make_unique<AlpacaOrderExecutor>(
    std::getenv("ALPACA_API_KEY"),
    std::getenv("ALPACA_BASE_URL"),
    10  // batch size
);
```

## Security

⚠️ **NEVER commit .env!**

- `.env` = your secrets (IGNORED by git)
- `.env.example` = template only (in git)

See `.gitignore` for what's ignored.
