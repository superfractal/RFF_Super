# Modified by GPT-6 on 2026-09-20, 2026-09-21, 2026-09-24
param(
    [string]$ServerConfig = (Join-Path (Split-Path $PSScriptRoot -Parent) 'local-ai-server.json'),
    [string]$ApplicationPath = (Join-Path (Split-Path $PSScriptRoot -Parent) 'bin/RFF_Super.exe')
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$serverProcess = $null

try {
    $config = Get-Content -LiteralPath $ServerConfig -Raw | ConvertFrom-Json
    $serverPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $config.executable))
    $modelPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $config.model))
    foreach ($file in @($serverPath, $modelPath, $ApplicationPath)) {
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
            throw "File not found: $file"
        }
    }
    $connection = Get-Content -LiteralPath (Join-Path $projectRoot 'local-ai.json') -Raw | ConvertFrom-Json
    $endpoint = [Uri]$connection.endpoint
    if ($endpoint.Host -ne '127.0.0.1' -or $endpoint.Port -ne $config.port) {
        throw 'local-ai.json must point to 127.0.0.1 and the port in local-ai-server.json.'
    }

    $portProbe = [Net.Sockets.TcpClient]::new()
    $portInUse = $false
    try {
        $portProbe.Connect('127.0.0.1', [int]$config.port)
        $portInUse = $true
    } catch {
    } finally {
        $portProbe.Dispose()
    }
    if ($portInUse) {
        throw "Port $($config.port) is already in use. Stop the existing server before using this launcher."
    }

    $logDirectory = Join-Path $projectRoot 'debug/local_ai'
    $null = New-Item -ItemType Directory -Force -Path $logDirectory
    $serverArguments = @(
        '-m', ('"' + $modelPath + '"'),
        '--host', '127.0.0.1',
        '--port', [string]$config.port,
        '--alias', [string]$connection.model
    ) + @($config.arguments)
    if ($connection.vision) {
        if (-not $config.mmproj) {
            throw 'Vision requires mmproj in local-ai-server.json. See docs/local-ai.md.'
        }
        $projectorPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $config.mmproj))
        if (-not (Test-Path -LiteralPath $projectorPath -PathType Leaf)) {
            throw "Vision projector not found: $projectorPath. See docs/local-ai.md."
        }
        $serverArguments += @('--mmproj', ('"' + $projectorPath + '"'))
    }
    $serverProcess = Start-Process -FilePath $serverPath `
        -ArgumentList $serverArguments `
        -WorkingDirectory $projectRoot `
        -WindowStyle Hidden `
        -PassThru `
        -RedirectStandardOutput (Join-Path $logDirectory 'managed-server.log') `
        -RedirectStandardError (Join-Path $logDirectory 'managed-server-error.log')

    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    $ready = $false
    while ([DateTime]::UtcNow -lt $deadline) {
        if ($serverProcess.HasExited) {
            throw 'The local AI server exited during startup. See debug/local_ai/managed-server-error.log.'
        }
        try {
            $health = Invoke-RestMethod -Uri "http://127.0.0.1:$($config.port)/health" -TimeoutSec 2
            if ($health.status -eq 'ok') {
                $ready = $true
                break
            }
        } catch {
        }
        Start-Sleep -Milliseconds 250
    }
    if (-not $ready) {
        throw 'The local AI server did not become ready within 120 seconds.'
    }

    $applicationProcess = Start-Process -FilePath $ApplicationPath `
        -WorkingDirectory $projectRoot `
        -WindowStyle Normal `
        -PassThru
    Write-Output "RFF_Super PID=$($applicationProcess.Id); owned API server PID=$($serverProcess.Id)"
    $applicationProcess.WaitForExit()
} finally {
    if ($null -ne $serverProcess) {
        if (-not $serverProcess.HasExited) {
            $serverProcess.Kill()
            $serverProcess.WaitForExit()
        }
        $serverProcess.Dispose()
    }
}
