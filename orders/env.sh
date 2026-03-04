#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

if [[ ! -f ".env" ]]; then
  echo "No .env found in $ROOT_DIR"
  exit 1
fi

set -a
source <(sed 's/\r$//' .env | grep -v '^\s*#' | grep -v '^\s*$')
set +a

if [[ $# -gt 0 ]]; then
  exec "$@"
fi

echo "Environment loaded from .env"
