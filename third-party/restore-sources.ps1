# Modified by GPT-6 on 2026-09-24
param(
    [string]$OutputDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'debug/restored-third-party-sources')
)

$ErrorActionPreference = 'Stop'
$sourceDirectory = Join-Path $PSScriptRoot 'sources'
$manifest = Get-Content -LiteralPath (Join-Path $sourceDirectory 'archive-parts.json') -Raw | ConvertFrom-Json
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$null = New-Item -ItemType Directory -Force -Path $outputRoot

foreach ($archive in $manifest.archives) {
    if ([IO.Path]::GetFileName($archive.file) -ne $archive.file) {
        throw 'Invalid archive filename'
    }
    $target = Join-Path $outputRoot $archive.file
    if (Test-Path -LiteralPath $target) {
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $archive.sha256) {
            throw "Existing output differs: $target"
        }
        Write-Output "Already verified: $($archive.file)"
        continue
    }

    $temporary = $target + '.' + [guid]::NewGuid().ToString('N') + '.partial'
    $outputStream = [IO.File]::Open($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
    try {
        $pieces = if ($archive.parts.Count -gt 0) {
            $archive.parts
        } else {
            @([pscustomobject]@{ file = $archive.file; sha256 = $archive.sha256 })
        }
        foreach ($piece in $pieces) {
            if ([IO.Path]::GetFileName($piece.file) -ne $piece.file) {
                throw 'Invalid part filename'
            }
            $inputPath = Join-Path $sourceDirectory $piece.file
            if ((Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash -ne $piece.sha256) {
                throw "Source checksum mismatch: $($piece.file)"
            }
            $inputStream = [IO.File]::OpenRead($inputPath)
            try {
                $inputStream.CopyTo($outputStream)
            } finally {
                $inputStream.Dispose()
            }
        }
    } finally {
        $outputStream.Dispose()
    }
    if ((Get-Item -LiteralPath $temporary).Length -ne $archive.size -or
        (Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash -ne $archive.sha256) {
        throw "Reconstructed archive checksum mismatch: $temporary"
    }
    Move-Item -LiteralPath $temporary -Destination $target
    Write-Output "Restored and verified: $($archive.file)"
}

Copy-Item -LiteralPath (Join-Path $sourceDirectory 'sources.json') -Destination $outputRoot
Write-Output "Source archives are ready in $outputRoot"
