# load-env.ps1 - Helper script to load .env variables (Windows PowerShell)
# Usage: . .\load-env.ps1

if (-not (Test-Path ".env")) {
    Write-Host "❌ Error: .env file not found!" -ForegroundColor Red
    Write-Host "📋 Create it from .env.example:" -ForegroundColor Yellow
    Write-Host "   Copy-Item .env.example .env" -ForegroundColor Green
    return
}

# Load .env variables
Get-Content .env | ForEach-Object {
    if ($_ -and -not $_.StartsWith('#')) {
        $name, $value = $_ -split '=', 2
        if ($name -and $value) {
            [System.Environment]::SetEnvironmentVariable($name, $value)
            $env:$name = $value
        }
    }
}

Write-Host "✅ Environment variables loaded from .env" -ForegroundColor Green
Write-Host "🔑 Loaded variables:" -ForegroundColor Cyan
Get-Content .env | Where-Object { $_ -and -not $_.StartsWith('#') } | ForEach-Object {
    $name = ($_ -split '=')[0]
    Write-Host "   - $name" -ForegroundColor Gray
}
