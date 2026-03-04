# Windows Build Script
# Run from PowerShell: .\build.ps1

param(
    [string]$BuildType = "Release",
    [switch]$NoTests = $false,
    [switch]$Help = $false
)

# Load environment variables from .env
if (Test-Path ".env") {
    Write-Host "Loading environment variables from .env" -ForegroundColor Yellow
    Get-Content .env | ForEach-Object {
        $line = $_.Trim()
        if ($line -eq "" -or $line -match "^\s*#") { return }
        if ($line -notmatch "=") { return }

        $name, $value = $line -split '=', 2
        $name = $name.Trim()
        $value = $value.Trim()

        if ([string]::IsNullOrWhiteSpace($name)) { return }

        $value = $value -replace "`r", ""
        $value = $value -replace '^\s*"', '' -replace '"\s*$', ''
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
} else {
    Write-Host "⚠️  No .env file found. Copy .env.example to .env" -ForegroundColor Yellow
    Write-Host "   cp .env.example .env" -ForegroundColor Yellow
    Write-Host "   Edit .env with your API credentials" -ForegroundColor Yellow
    Write-Host ""
}

if ($Help) {
    Write-Host "Usage: .\build.ps1 [OPTIONS]"
    Write-Host "Options:"
    Write-Host "  -BuildType Release|Debug  Build type (default: Release)"
    Write-Host "  -NoTests                  Skip test build"
    Write-Host "  -Help                     Show this help"
    exit 0
}

Write-Host "=== Building HFT Orders System ===" -ForegroundColor Yellow
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow
Write-Host "Build Tests: $(if ($NoTests) { 'OFF' } else { 'ON' })" -ForegroundColor Yellow
Write-Host ""

$BuildDir = "build"
$BuildTests = if ($NoTests) { "OFF" } else { "ON" }

# Clean old build
if (Test-Path $BuildDir) {
    Write-Host "Cleaning old build..." -ForegroundColor Yellow
    Remove-Item $BuildDir -Recurse -Force
}

# Create build directory
Write-Host "Creating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# Configure
Write-Host "Running CMake configure..." -ForegroundColor Yellow
Push-Location $BuildDir
cmake .. `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DBUILD_TESTS=$BuildTests

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build . --config $BuildType --parallel 4

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host ""

# Run tests if enabled
if ($BuildTests -eq "ON") {
    Write-Host "Running tests..." -ForegroundColor Yellow
    ctest --verbose
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "Some tests failed!" -ForegroundColor Red
        Pop-Location
        exit 1
    }
}

Pop-Location

Write-Host "=== Build Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Run example: .\build\Release\example_fire_and_forget.exe"
Write-Host "  2. Review documentation: docs/QUICKSTART.md"
Write-Host ""
