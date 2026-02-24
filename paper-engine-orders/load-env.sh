#!/bin/bash
# load-env.sh - Helper script to load .env variables
# Usage: source ./load-env.sh

if [ ! -f .env ]; then
    echo "❌ Error: .env file not found!"
    echo "📋 Create it from .env.example:"
    echo "   cp .env.example .env"
    return 1
fi

# Load .env variables
export $(cat .env | grep -v '^#' | xargs)

echo "✅ Environment variables loaded from .env"
echo "🔑 Loaded variables:"
grep -v '^#' .env | grep -v '^$' | sed 's/=.*//' | sed 's/^/   - /'
