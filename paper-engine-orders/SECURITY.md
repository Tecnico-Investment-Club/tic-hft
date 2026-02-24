# Guia de Segurança - Variáveis de Ambiente

## 🔒 Visão Geral

As variáveis de ambiente contêm **credenciais sensíveis** (API keys, tokens, passwords). Este guia descreve as **melhores práticas** para gerenciar-las com segurança.

---

## 📋 Ficheiros de Ambiente

### `.env` (PRIVADO - Nunca commitar!)
```bash
# Este ficheiro contém as tuas credenciais REAIS
# ⚠️  NUNCA deve ser committed ao git
# ⚠️  Guardar com permissões restritivas: chmod 600 .env

ALPACA_API_KEY=pk_abc123xyz...
DB_SOURCE_CONNECTION=postgresql://user:password@localhost:5432/db
```

**Status**: Ignorado por `.gitignore`

### `.env.example` (PÚBLICO - Seguro para commitar)
```bash
# Este ficheiro é um TEMPLATE com valores placeholder
# ✓ PODE ser committed ao git (sem valores reais)
# ✓ Serve como documentação para outros developers

ALPACA_API_KEY=your_alpaca_api_key_here
DB_SOURCE_CONNECTION=postgresql://hft_user:hft_password@localhost:5432/hft_db
```

**Status**: Deve ser committed ao git

---

## 🚀 Como Usar

### Opção 1: Ficheiro `.env` (Recomendado para desenvolvimento local)

**1. Copiar o template:**
```bash
cd tic-hft/paper-engine-orders
cp .env.example .env
```

**2. Editar `.env` com as tuas credenciais reais:**
```bash
# .env
ALPACA_API_KEY=pk_live_abc123xyz...
ALPACA_BASE_URL=https://paper-api.alpaca.markets
DB_SOURCE_CONNECTION=postgresql://user:pass@localhost:5432/db
```

**3. Carregar as variáveis no bash/shell:**

**Linux/macOS:**
```bash
# Opção A: Usar export (para o shell atual)
export $(cat .env | xargs)

# Opção B: Usar source (similar)
source .env

# Opção C: Usar com direnv (recomendado)
# Instalar: brew install direnv
echo "export \$(cat .env | xargs)" > .envrc
direnv allow
```

**Windows PowerShell:**
```powershell
# Opção A: Carregar manualmente
$env:ALPACA_API_KEY = "pk_live_abc123xyz..."
$env:DB_SOURCE_CONNECTION = "postgresql://..."

# Opção B: Script helper
Get-Content .env | ForEach-Object {
    if ($_ -and -not $_.StartsWith('#')) {
        $name, $value = $_ -split '=', 2
        [Environment]::SetEnvironmentVariable($name, $value)
    }
}
```

**4. Executar a aplicação:**
```bash
./paper-engine-orders --dry-run
# As variáveis estão agora disponíveis como $env:ALPACA_API_KEY, etc
```

### Opção 2: Docker Compose (Recomendado para produção)

**1. Criar `.env`:**
```bash
cp .env.example .env
# Editar com as tuas credenciais
```

**2. Docker Compose lê automaticamente `.env`:**
```bash
docker-compose up --build
# Docker carrega .env automaticamente
```

**docker-compose.yml** (já configurado):
```yaml
environment:
  ALPACA_API_KEY: ${ALPACA_API_KEY}
  ALPACA_BASE_URL: ${ALPACA_BASE_URL}
  DB_SOURCE_CONNECTION: ${DB_SOURCE_CONNECTION}
```

### Opção 3: Variáveis de Sistema (Linux/macOS)

```bash
# Definir permanentemente no ~/.bashrc ou ~/.zshrc
export ALPACA_API_KEY="pk_live_abc123xyz..."
export ALPACA_BASE_URL="https://paper-api.alpaca.markets"
export DB_SOURCE_CONNECTION="postgresql://..."

# Recarregar:
source ~/.bashrc  # ou ~/.zshrc
```

### Opção 4: Variáveis de Sistema (Windows)

```powershell
# PowerShell (permanente para o utilizador)
[Environment]::SetEnvironmentVariable("ALPACA_API_KEY", "pk_live_abc123xyz...", "User")
[Environment]::SetEnvironmentVariable("DB_SOURCE_CONNECTION", "postgresql://...", "User")

# Reiniciar PowerShell ou rebootar
```

**Sistema → Definições avançadas → Variáveis de ambiente** (GUI)

---

## 🔐 Segurança: Melhores Práticas

### ✅ FAZER:

1. **Guardar `.env` localmente**
   ```bash
   # Nunca commitar
   echo ".env" >> .gitignore
   ```

2. **Usar permissões restritivas (Linux/macOS)**
   ```bash
   chmod 600 .env  # Apenas tu podes ler/escrever
   ```

3. **Rotação de credentials**
   - Trocar Alpaca API keys regularmente
   - Atualizar database passwords periodicamente

4. **Usar ficheiros separados para dev/prod**
   ```
   .env.development    (dev local)
   .env.production    (servidor)
   .env.staging       (testes)
   ```

5. **Usar secrets manager em produção**
   - AWS Secrets Manager
   - Azure Key Vault
   - HashiCorp Vault
   - 1Password CLI

6. **Auditar acesso**
   - Logs de quem acedeu credenciais
   - Monitorar uso de API keys

### ❌ NÃO FAZER:

1. **❌ Commitar `.env` ao git**
   ```bash
   # NUNCA!
   git add .env
   git commit -m "Add credentials"
   ```

2. **❌ Hardcode secrets no código**
   ```cpp
   // NUNCA!
   const char* api_key = "pk_live_abc123xyz...";
   ```

3. **❌ Compartilhar credenciais por email/chat**
   ```
   ❌ slack: "hey, use this api key: pk_live_abc123xyz..."
   ✓ Use: 1Password shared vault, LastPass, etc
   ```

4. **❌ Usar mesmas credentials dev/prod**
   - Dev: API key de teste
   - Prod: API key de produção

5. **❌ Guardar passwords em plain text**
   ```bash
   # ❌ NUNCA:
   password=MyPassword123
   
   # ✓ Usar hash/encryption
   ```

6. **❌ Log de credenciais**
   ```cpp
   // ❌ NUNCA:
   std::cout << "API Key: " << api_key << std::endl;
   
   // ✓ Log apenas de ações:
   std::cout << "API call successful" << std::endl;
   ```

---

## 🔑 Exemplo: Alpaca API Key

### Como obter:

1. Ir para https://app.alpaca.markets
2. Login ou Sign up
3. Account → Settings → API Keys
4. Generate new key
5. Copiar e guardar em `.env`:

```bash
# .env
ALPACA_API_KEY=pk_live_abc123xyz...
```

### Tipos de keys:

- **Paper Trading**: `pk_...` (seguro para testes)
- **Live Trading**: `pk_live_...` (REAL MONEY - USE COM CUIDADO!)

---

## 🐳 Docker: Segurança Extra

### Usar secrets do Docker (Swarm/Kubernetes):

**Docker Swarm:**
```bash
# Criar secret
echo "pk_live_abc123xyz..." | docker secret create alpaca_api_key -

# docker-compose.yml
version: '3.8'
services:
  paper-engine-orders:
    secrets:
      - alpaca_api_key
    environment:
      ALPACA_API_KEY_FILE: /run/secrets/alpaca_api_key
```

**Kubernetes:**
```bash
# Criar secret
kubectl create secret generic hft-secrets \
  --from-literal=alpaca_api_key=pk_live_abc123xyz...

# deployment.yaml
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: paper-engine-orders
    env:
    - name: ALPACA_API_KEY
      valueFrom:
        secretKeyRef:
          name: hft-secrets
          key: alpaca_api_key
```

---

## 📝 Checklist de Segurança

- [ ] `.env` está em `.gitignore`
- [ ] `.env.example` foi committed (sem valores reais)
- [ ] Nunca commit `.env` por acidente
- [ ] Permissões restritas: `chmod 600 .env`
- [ ] Credenciais diferentes para dev/prod
- [ ] API keys nunca aparecem em logs
- [ ] Revisar git history se api key foi exposta:
  ```bash
  git log -p --all -- ".env"  # Ver se .env foi commitado
  ```

---

## 🚨 Se Credenciais Forem Expostas

1. **IMEDIATO**: Revogar a API key
   - Alpaca: Account → API Keys → Revoke

2. **Verificar git history**:
   ```bash
   # Ver commits com .env
   git log --name-status | grep ".env"
   
   # Ver se foi pushed
   git log origin/main --name-status | grep ".env"
   ```

3. **Limpar history (se necessário)**:
   ```bash
   # ⚠️ Cuidado: isto reescreve a história!
   git filter-branch --tree-filter 'rm -f .env' HEAD
   git push origin main --force-with-lease
   ```

4. **Notificar a equipa**

---

## 💡 Tools Recomendadas

### Local Development:
- **direnv**: https://direnv.net/
- **dotenv**: npm/python packages

### Secrets Management:
- **1Password**: Teams e automação
- **LastPass**: Teams
- **AWS Secrets Manager**: AWS
- **HashiCorp Vault**: On-premise

### Security Scanning:
- **git-secrets**: Prevent commits with secrets
  ```bash
  brew install git-secrets
  git secrets --install
  git secrets --register-aws
  ```

- **gitguardian**: Scan repositories
- **TruffleHog**: Find secrets in git history

---

## 📚 Referências

- [12 Factor App - Config](https://12factor.net/config)
- [OWASP - Secrets Management](https://cheatsheetseries.owasp.org/cheatsheets/Secrets_Management_Cheat_Sheet.html)
- [Alpaca API Docs](https://docs.alpaca.markets/)
- [Git Secrets](https://github.com/awslabs/git-secrets)
