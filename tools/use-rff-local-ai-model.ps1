# Modified by GPT-6 on 2026-09-27
# Modified by Opus 5.5 on 2026-10-04
param([Parameter(Mandatory = $true)][ValidateSet('Bonsai', 'Qwen')][string]$Model)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
# Settings live in config/; move any copy an older build left at the root.
$configDirectory = Join-Path $root 'config'
$null = New-Item -ItemType Directory -Force -Path $configDirectory
foreach ($name in @('local-ai.json', 'local-ai-server.json')) {
    $legacy = Join-Path $root $name
    if ((Test-Path -LiteralPath $legacy) -and -not (Test-Path -LiteralPath (Join-Path $configDirectory $name))) {
        Move-Item -LiteralPath $legacy -Destination $configDirectory
    }
}
$profile = Join-Path $root ('debug/local_ai/profiles/' + $Model.ToLowerInvariant())
$server = Get-Content -LiteralPath (Join-Path $profile 'local-ai-server.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$settings = Get-Content -LiteralPath (Join-Path $profile 'local-ai.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$connectionPath = Join-Path $configDirectory 'local-ai.json'
$connection = Get-Content -LiteralPath $connectionPath -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($file in @($server.executable, $server.model, $server.mmproj)) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $file) -PathType Leaf)) { throw "Missing model/runtime file: $file" }
}
$probe = [Net.Sockets.TcpClient]::new()
$running = $false
try { $probe.Connect('127.0.0.1', [int]$server.port); $running = $true } catch {} finally { $probe.Dispose() }
if ($running) { throw 'Close RFF_Super and its local AI server before changing models.' }
$backup = Join-Path $root ('debug/local_ai/model-switch-' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-ffff'))
$null = New-Item -ItemType Directory -Path $backup
Copy-Item -LiteralPath $connectionPath -Destination $backup
Copy-Item -LiteralPath (Join-Path $configDirectory 'local-ai-server.json') -Destination $backup
$connection.model = $settings.model
$connection.request = $settings.request
$connection.endpoint = 'http://127.0.0.1:' + $server.port + '/v1/chat/completions'
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $configDirectory 'local-ai-server.json'), ($server | ConvertTo-Json -Depth 20), $utf8)
[IO.File]::WriteAllText($connectionPath, ($connection | ConvertTo-Json -Depth 30), $utf8)
Write-Output "Selected $Model. Start RFF_Super with tools/start-rff-local-ai.ps1."
