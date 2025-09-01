<#  dev_web_ui.ps1 — Build WASM engine, copy into UI, run Vite dev server
    Usage:
      pwsh -File tools/dev_web_ui.ps1
      # or right-click "Run with PowerShell"

    Notes:
      - Requires Emscripten SDK (emsdk) installed.
      - If emcmake isn’t on PATH but $env:EMSDK is set, this script will source emsdk_env.bat.
      - Expects your CMake target to be named "objviewer". Change $TargetName if needed.
#>

#region Config
$ErrorActionPreference = 'Stop'
$TargetName = 'objviewer'  # <-- change if your CMake target name differs
$BuildDir   = 'build-web'
$EngineOut  = 'ui/public/engine'
#endregion

function Write-Step($n, $msg) { Write-Host "[${n}] $msg" -ForegroundColor Cyan }
function Write-Warn($msg)     { Write-Host "(warning) $msg" -ForegroundColor Yellow }
function Write-Err($msg)      { Write-Host "ERROR: $msg" -ForegroundColor Red }

# Resolve repo root as the parent of this script's folder
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = Resolve-Path (Join-Path $ScriptDir '..')
Set-Location $RepoRoot

function Ensure-Emsdk {
  Write-Step 0 "Ensuring Emscripten tools are available..."
  $emcmake = (Get-Command emcmake -ErrorAction SilentlyContinue)
  if (-not $emcmake) {
    if ($env:EMSDK) {
      Write-Host "  emcmake not on PATH; sourcing `"$env:EMSDK\emsdk_env.bat`"..."
      & "$env:EMSDK\emsdk_env.bat" | Out-Null
      $emcmake = (Get-Command emcmake -ErrorAction SilentlyContinue)
      if (-not $emcmake) {
        Write-Warn "emcmake still not found on PATH. Make sure emsdk is installed correctly."
      }
    } else {
      Write-Warn "EMSDK env var not set and emcmake not found on PATH."
    }
  } else {
    Write-Host "  emcmake found at $($emcmake.Source)"
  }
}

function Ensure-Ninja {
  Write-Host "  Ensuring Ninja is available..."
  $global:NinjaPath = $null
  $n = (Get-Command ninja -ErrorAction SilentlyContinue)
  if ($n) { $global:NinjaPath = $n.Source }
  if (-not $global:NinjaPath -and $env:EMSDK) {
    $candidate = Join-Path $env:EMSDK 'ninja.exe'
    if (Test-Path $candidate) { $global:NinjaPath = $candidate }
  }
  if (-not $global:NinjaPath) {
    throw "Ninja not found. Install ninja.exe and/or add it to PATH. Tip: place ninja.exe in %EMSDK%."
  }
  Write-Host "  Using Ninja: $global:NinjaPath"
}

function Configure-WebBuild {
  Write-Step "1/5" "Configure Emscripten web build (if needed)..."
  if (-not (Test-Path (Join-Path $BuildDir 'CMakeCache.txt'))) {
    # Use emcmake so the toolchain is set automatically
    & emcmake cmake -S . -B $BuildDir -G Ninja -DCMAKE_MAKE_PROGRAM="$global:NinjaPath" -DCMAKE_BUILD_TYPE=Release
  } else {
    Write-Host "  $BuildDir already configured."
  }
}

function Build-Web {
  Write-Step "2/5" "Build engine (WASM)..."
  & cmake --build $BuildDir -j
}

function Copy-Engine {
  Write-Step "3/5" "Copy engine outputs to $EngineOut ..."
  if (-not (Test-Path $EngineOut)) { New-Item -ItemType Directory -Path $EngineOut | Out-Null }
  $files = @("$TargetName.js", "$TargetName.wasm", "$TargetName.data")
  $copiedAny = $false
  foreach ($f in $files) {
    $src = Join-Path $BuildDir $f
    if (Test-Path $src) {
      Copy-Item $src -Destination (Join-Path $EngineOut $f) -Force
      Write-Host "  copied $f"
      $copiedAny = $true
    } else {
      Write-Host "  (info) $src not found, skipping."
    }
  }
  # Optional: copy html for convenience
  $htmlSrc = Join-Path $BuildDir "$TargetName.html"
  if (Test-Path $htmlSrc) {
    Copy-Item $htmlSrc -Destination (Join-Path $EngineOut "$TargetName.html") -Force
    Write-Host "  copied $TargetName.html"
  }
  if (-not $copiedAny) {
    Write-Warn "No JS/WASM/DATA outputs copied. Ensure your CMake target is '$TargetName' or adjust the script."
  }
}

function Install-UI {
  Write-Step "4/5" "Install UI deps (if needed)..."
  Push-Location 'ui'
  if (-not (Test-Path 'package.json')) {
    Pop-Location
    throw "ui/package.json not found. Is the UI located at $RepoRoot\ui ?"
  }
  if (-not (Test-Path 'node_modules')) {
    & npm install
  } else {
    Write-Host "  node_modules already present."
  }
  Pop-Location
}

function Start-Dev {
  Write-Step "5/5" "Start dev server at http://localhost:5173 ..."
  Push-Location 'ui'
  # Use 'call' style via & so it inherits the current environment
  & npm run dev
  Pop-Location
}

# ---- Run all steps
try {
  Ensure-Emsdk
  Ensure-Ninja
  Configure-WebBuild
  Build-Web
  Copy-Engine
  Install-UI
  Start-Dev
}
catch {
  Write-Host ""
  Write-Err $_.Exception.Message
  exit 1
}
