# Quick Setup Guide - tic-hft/paper-engine-orders

## 🚀 Quick Start (5 minutos)

### 1️⃣ Setup Variáveis de Ambiente

**Linux/macOS:**
```bash
cd tic-hft/paper-engine-orders
cp .env.example .env
nano .env  # ou vim, code, etc
source ./load-env.sh
```

**Windows (PowerShell):**
```powershell
cd tic-hft\paper-engine-orders
Copy-Item .env.example .env
notepad .env  # Editar com o teu editor favorito
. .\load-env.ps1
```

### 2️⃣ Preencher `.env`

```bash
# Editar e adicionar as tuas credenciais:
ALPACA_API_KEY=pk_live_abc123...
ALPACA_BASE_URL=https://paper-api.alpaca.markets
DB_SOURCE_CONNECTION=postgresql://user:pass@localhost:5432/db
DB_TARGET_CONNECTION=postgresql://user:pass@localhost:5432/db
```

### 3️⃣ Build & Run

**Com Docker (Recomendado):**
```bash
docker-compose up --build
```

**Manual:**
```bash
mkdir build && cd build
cmake ..
make
./paper-engine-orders --dry-run
```

---

## 📖 Documentação Completa

- **SECURITY.md**: Guia de segurança completo
- **README.md**: Documentação do projeto
- **Dockerfile**: Container configuration
- **CMakeLists.txt**: Build configuration

---

## ⚠️ Importante

- **NUNCA** commitar `.env` ❌
- **SEMPRE** usar `chmod 600 .env` no Linux/macOS 🔒
- **SEMPRE** usar `.env.example` como template 📋
- **SEMPRE** usar paper trading para testes 🧪
- **NUNCA** compartilhar API keys ⛔

---

## ✅ Checklist

- [ ] Criar `.env` a partir de `.env.example`
- [ ] Preencher credenciais reais
- [ ] Carregar variáveis: `source ./load-env.sh` (ou `load-env.ps1`)
- [ ] Testar conexão DB: `psql $DB_SOURCE_CONNECTION -c "SELECT 1"`
- [ ] Verificar permissões: `ls -la .env` (deve ser `-rw-------`)
- [ ] Build: `mkdir build && cd build && cmake .. && make`
- [ ] Run: `./paper-engine-orders --dry-run`

---

## 🆘 Troubleshooting

**"Error: .env file not found"**
```bash
cp .env.example .env
```

**"Error: ALPACA_API_KEY not set"**
```bash
# Verificar se as variáveis estão carregadas
echo $ALPACA_API_KEY
# Se vazio, fazer load novamente:
source ./load-env.sh
```

**"PostgreSQL connection failed"**
```bash
# Verificar se DB está running
psql -h localhost -U hft_user -d hft_db -c "SELECT 1"
# Se docker:
docker-compose ps
```

---

## 📞 Suporte

Ver SECURITY.md para mais informações sobre segurança e secrets management.
