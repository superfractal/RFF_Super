# Modified by Opus 5.5 on 2026-10-18
# Builds a release from a clean checkout of a committed tag with the verified static GMP, then stages it with its sources.
param(
    [Parameter(Mandatory = $true)][string]$Ref,
    [switch]$Candidate,
    [string]$MsysRoot = "C:/msys64",
    [string]$GmpPrefix = "",
    [string]$GmpSha256 = "cea4c8ae34263d44e982ff5f7a3a6a38095d539045c0e50e9ee462c5d83784f5",
    [int]$Jobs = 6
)
$ErrorActionPreference = "Stop"

function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe $($Arguments -join ' ') failed with exit code $LASTEXITCODE" }
}

function Get-Checked([string]$Exe, [string[]]$Arguments) {
    $output = & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe $($Arguments -join ' ') failed with exit code $LASTEXITCODE" }
    return $output
}

function ConvertTo-CMakePath([string]$Path) { return $Path -replace '\\', '/' }

$project = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$mingw = Join-Path $MsysRoot "mingw64"
$python = Join-Path $mingw "bin/python.exe"
$env:Path = (Join-Path $mingw "bin") + ";" + $env:Path
# A fresh checkout has no build/release-deps, so the installed package is used; the digest check below still guards it.
if (!$GmpPrefix) {
    $GmpPrefix = Join-Path $project "build/release-deps/mingw64"
    if (!(Test-Path (Join-Path $GmpPrefix "lib/libgmp.a"))) { $GmpPrefix = $mingw }
}
$GmpPrefix = (Resolve-Path $GmpPrefix).Path
$gmpLibrary = Join-Path $GmpPrefix "lib/libgmp.a"

# The unidentified installed libgmp.a must never reach a release, so the pinned package digest is checked before anything is built.
$gmpActual = (Get-FileHash -Algorithm SHA256 $gmpLibrary).Hash.ToLowerInvariant()
if ($gmpActual -ne $GmpSha256) { throw "$gmpLibrary is not the reviewed GMP package (SHA-256 $gmpActual)" }

$commit = (Get-Checked git @("-C", $project, "rev-parse", "--verify", "$Ref^{commit}")).Trim()
$tags = @(Get-Checked git @("-C", $project, "tag", "--points-at", $commit))
$tag = if ($tags -contains $Ref) { $Ref } elseif ($tags.Count -gt 0) { $tags[0] } else { "" }
if (!$Candidate -and !$tag) { throw "$Ref ($commit) has no tag; tag the release commit, or pass -Candidate for a trial build" }

$short = $commit.Substring(0, 12)
$name = if ($Candidate) { "candidate-$short-" + (Get-Date -Format "yyyyMMdd-HHmmss") } else { $tag }
$root = Join-Path $project "build/release/$name"
if (Test-Path $root) { throw "Output already exists; choose another tag or remove it: $root" }
$source = Join-Path $root "source"
$build = Join-Path $source "build/distribution"
$stage = Join-Path $root "candidate"
New-Item -ItemType Directory -Force (Split-Path $root) | Out-Null

Invoke-Checked git @("-C", $project, "worktree", "add", "--detach", $source, $commit)

Invoke-Checked cmake @(
    "-S", (ConvertTo-CMakePath $source), "-B", (ConvertTo-CMakePath $build), "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_CXX_COMPILER=g++", "-DCMAKE_C_COMPILER=gcc",
    "-DRFF_DISTRIBUTION=ON", "-DRFF_ASAN=OFF",
    "-DRFF_DEPENDENCY_PREFIX=$(ConvertTo-CMakePath $mingw)",
    "-DRFF_GMP_PREFIX=$(ConvertTo-CMakePath $GmpPrefix)"
)
Invoke-Checked cmake @("--build", (ConvertTo-CMakePath $build), "--parallel", "$Jobs")

$dirty = Get-Checked git @("-C", $source, "status", "--porcelain")
if ($dirty) { throw "The build changed tracked or unignored files in the checkout:`n$($dirty -join "`n")" }

$exe = Join-Path $build "runtime/bin/RFF_Super.exe"
$shaders = Join-Path $build "runtime/shaders"
$inventory = Join-Path $root "release-inventory.json"
Invoke-Checked $python @((Join-Path $source "tools/release_inventory.py"), "--exe", $exe, "--msys-root", $MsysRoot, "--static-gmp", $gmpLibrary, "--output", $inventory)
Invoke-Checked $python @((Join-Path $source "tools/release_sources.py"), "--inventory", $inventory, "--msys-root", $MsysRoot, "--output", (Join-Path $project "build/release-sources"))
Invoke-Checked $python @((Join-Path $source "tools/release_stage.py"), "--inventory", $inventory, "--sources", (Join-Path $project "build/release-sources"), "--shaders", $shaders, "--output", $stage, "--msys-root", $MsysRoot)

$archive = Join-Path $stage "sources/RFF_Super-source.tar.gz"
Invoke-Checked git @("-C", $project, "archive", "--format=tar.gz", "--prefix=RFF_Super-source/", "--output=$archive", $commit)

$marker = Join-Path $stage "NOT-FOR-DISTRIBUTION.txt"
if ($Candidate) {
    [IO.File]::WriteAllText($marker, "Trial build of untagged commit $commit. Tag the release commit and rerun tools/release_build.ps1 before distributing.`n", (New-Object Text.UTF8Encoding($false)))
} else {
    Remove-Item $marker
}

# The stage manifest was written before the source archive and marker change, so it is rebuilt over the final folder.
Invoke-Checked $python @("-c", "import json,sys,hashlib,pathlib;s=pathlib.Path(sys.argv[1]);m={p.relative_to(s).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(s.rglob('*')) if p.is_file() and p.name!='SHA256SUMS.json'};(s/'SHA256SUMS.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8')", $stage)

$record = [ordered]@{
    schema = 1
    status = if ($Candidate) { "TRIAL_BUILD_NOT_FOR_DISTRIBUTION" } else { "TAGGED_CLEAN_BUILD" }
    tag = $tag
    commit = $commit
    built = (Get-Date -Format "o")
    checkout_clean_after_build = $true
    cmake_options = @("CMAKE_BUILD_TYPE=Release", "RFF_DISTRIBUTION=ON", "RFF_ASAN=OFF", "RFF_DEPENDENCY_PREFIX=$(ConvertTo-CMakePath $mingw)", "RFF_GMP_PREFIX=$(ConvertTo-CMakePath $GmpPrefix)")
    tools = [ordered]@{
        gcc = @(Get-Checked gcc @("--version"))[0]
        cmake = @(Get-Checked cmake @("--version"))[0]
        ninja = @(Get-Checked ninja @("--version"))[0]
        glslc = ((Get-Checked glslc @("--version")) -join " ")
    }
    static_gmp = [ordered]@{ path = (ConvertTo-CMakePath $gmpLibrary); sha256 = $gmpActual }
    executable_sha256 = (Get-FileHash -Algorithm SHA256 $exe).Hash.ToLowerInvariant()
    source_archive_sha256 = (Get-FileHash -Algorithm SHA256 $archive).Hash.ToLowerInvariant()
}
[IO.File]::WriteAllText((Join-Path $root "build-record.json"), ($record | ConvertTo-Json -Depth 4), (New-Object Text.UTF8Encoding($false)))

Write-Host "Release staged: $stage" -ForegroundColor Green
Write-Host "Build record: $(Join-Path $root 'build-record.json')"
Write-Host "Checkout kept for inspection; remove with: git worktree remove --force `"$source`""
