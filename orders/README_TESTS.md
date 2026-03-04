# README_TESTS

Guia prático para testar cada pedaço do projeto e comandos essenciais.

## 1) Pré-requisitos

### WSL/Linux

- `cmake`
- compilador C/C++ (`gcc/g++`)
- `libcurl` dev
- opcional: `gtest`

Exemplo Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libgtest-dev
```

### Windows PowerShell

- CMake no PATH
- toolchain C++
- libcurl disponível para o CMake encontrar

## 2) Preparar variáveis (.env)

Copiar template:

```bash
cp .env.example .env
```

Preencher:

- `ALPACA_API_KEY`
- `ALPACA_SECRET_KEY`
- `ALPACA_BASE_URL` (paper recomendado)

## 3) Comandos essenciais (WSL)

### Carregar `.env`

```bash
source ./env.sh
```

### Build + testes (script completo)

```bash
./build.sh
```

### Só build sem testes

```bash
./build.sh --no-tests
```

### Build manual CMake

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

### Correr suite de testes

```bash
cd build
ctest --output-on-failure
```

### Correr binário de testes diretamente

```bash
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

## 4) Comandos essenciais (PowerShell)

### Build + testes

```powershell
.\build.ps1
```

### Build sem testes

```powershell
.\build.ps1 -NoTests
```

### Debug build

```powershell
.\build.ps1 -BuildType Debug
```

## 5) Como testar cada parte

## Parte A — Parsing de ambiente

Objetivo: confirmar que variáveis existem antes de testar rede.

```bash
source ./env.sh
env | grep '^ALPACA_'
```

Se alguma faltar, os testes de integração podem ser `SKIP` ou falhar auth.

## Parte B — Compilação

Objetivo: validar toolchain + dependências CMake.

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j4
```

## Parte C — Testes GTest

Objetivo: validar API interna (`sendOrder`, `sendOrders`, lifecycle, etc.).

```bash
cd build
./orders_test
```

Obs: testes no ficheiro [tests/orders_test.cpp](tests/orders_test.cpp) fazem `skip` quando `ALPACA_API_KEY/SECRET` não existem.

## Parte D — Integração Alpaca (conectividade)

Objetivo: confirmar auth real antes de culpar o código.

```bash
source ./env.sh
curl -s -o /tmp/alpaca_account.json -w "%{http_code}\n" \
  -H "accept: application/json" \
  -H "APCA-API-KEY-ID: $ALPACA_API_KEY" \
  -H "APCA-API-SECRET-KEY: $ALPACA_SECRET_KEY" \
  "${ALPACA_BASE_URL}/v2/account"
cat /tmp/alpaca_account.json
```

Leitura rápida:

- `200` => conectado/autenticado
- `401` => key/secret inválida ou não carregada

## Parte E — Exemplo end-to-end

Objetivo: validar fluxo fire-and-forget a correr mesmo binário da app.

```bash
source ./env.sh
./build/example_fire_and_forget
```

## 6) Erros comuns e correção rápida

### `unauthorized`

- confirmar se estás no paper URL correto;
- confirmar variáveis no mesmo shell (`source ./env.sh`);
- validar conta com `GET /v2/account` via curl.

### `No .env found`

- cria `.env` a partir de `.env.example`.

### `cannot open source file curl/curl.h` (editor)

- dependência do ambiente de dev/includePath (não necessariamente erro de runtime).

### `Clock skew detected` no WSL

- aviso de timestamps entre Windows/WSL; normalmente não bloqueia build/test.

## 7) Comandos que deves decorar

WSL:

```bash
source ./env.sh
./build.sh
cd build && ctest --output-on-failure
cd build && ./orders_test --gtest_list_tests
```

PowerShell:

```powershell
.\build.ps1
.\build.ps1 -NoTests
```
