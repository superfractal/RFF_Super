# Modified by Opus 5 on 2026-08-19
# Modified by GPT-6 on 2026-09-20, 2026-09-23, 2026-09-24
param([string]$OutputDirectory = "")
$glslc = Get-Command glslc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source

if (!$glslc) {
    if ($env:VULKAN_SDK) {
        $glslc = Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
    }
}

if (!$glslc) {
    # Search common Vulkan SDK install roots across all fixed drives, not just C:\VulkanSDK.
    $searchRoots = @()
    foreach ($drive in (Get-PSDrive -PSProvider FileSystem -ErrorAction SilentlyContinue)) {
        $searchRoots += (Join-Path $drive.Root "VulkanSDK")
        $searchRoots += (Join-Path $drive.Root "Program Files\VulkanSDK")
        $searchRoots += (Join-Path $drive.Root "Program Files (x86)\VulkanSDK")
    }
    $searchRoots = $searchRoots | Where-Object { Test-Path $_ } | Select-Object -Unique

    foreach ($root in $searchRoots) {
        $possiblePaths = Get-ChildItem -Path $root -Filter "glslc.exe" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending
        if ($possiblePaths) {
            $glslc = $possiblePaths[0].FullName
            break
        }
    }
}

if (!$glslc) {
    Write-Error "glslc.exe not found! Please install Vulkan SDK or add its Bin folder to your PATH."
    exit 1
}

Write-Host "Using glslc: $glslc" -ForegroundColor Cyan
$workspaceRoot = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$srcDir = Join-Path $workspaceRoot "shdsrc"
$dstDir = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $workspaceRoot "shaders" }
if (!$dstDir.StartsWith($workspaceRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Shader output must be a directory within the project: $dstDir"
}
if ((Split-Path -Leaf $dstDir) -ne 'shaders') {
    throw "Shader output must use a dedicated shaders directory: $dstDir"
}
$buildDir = Split-Path -Parent $dstDir
$stageDir = Join-Path $buildDir ("shaders-stage-" + [guid]::NewGuid().ToString("N"))
$backupDir = Join-Path $buildDir ("shaders-backup-" + [guid]::NewGuid().ToString("N"))

foreach ($managedPath in @($dstDir, $stageDir, $backupDir)) {
    $resolvedManagedPath = [IO.Path]::GetFullPath($managedPath)
    if (!$resolvedManagedPath.StartsWith($workspaceRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unsafe shader staging path: $resolvedManagedPath"
    }
    if ((Test-Path -LiteralPath $managedPath) -and ((Get-Item -LiteralPath $managedPath).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Shader staging must not target a reparse point: $managedPath"
    }
}

New-Item -ItemType Directory -Force -Path $stageDir | Out-Null

$exts = @(".vert", ".frag", ".comp")
$files = Get-ChildItem -Path $srcDir -File -Recurse | Where-Object { $exts -contains $_.Extension }

Write-Host "Found $($files.Count) shader files in $srcDir" -ForegroundColor Cyan

foreach ($file in $files) {
    $inputFile = $file.FullName

    $relativePath = $file.DirectoryName.Substring($srcDir.Length)
    if ($relativePath.StartsWith("\") -or $relativePath.StartsWith("/")) {
        $relativePath = $relativePath.Substring(1)
    }
    
    $targetDir = Join-Path $stageDir $relativePath
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    }

    $outputFile = Join-Path $targetDir ($file.Name + ".spv")
    
    Write-Host "Compiling: " -NoNewline
    Write-Host $file.Name -NoNewline -ForegroundColor Yellow
    Write-Host " -> " -NoNewline
    Write-Host ($relativePath + "\" + $file.Name + ".spv") -ForegroundColor Green
    
    # Optimize the large surface graph at build time to reduce first-use driver compilation.
    $compileOptions = @()
    if ($file.Name -eq "vk_slope.frag") { $compileOptions += "-O" }
    & $glslc @compileOptions $inputFile -o $outputFile
    
    if ($LASTEXITCODE -ne 0) {
        Remove-Item -LiteralPath $stageDir -Recurse -Force
        Write-Error "Failed to compile $inputFile"
        exit 1
    }
}

# One source, two binaries: the video chain's merged image is 8-bit for an SDR export and half float
# for an HDR one, and a storage image's format is fixed in the compiled shader rather than chosen when
# it is bound. Each entry names the source, the define, and the .spv the extra build is written to.
$variants = @(
    @{ Source = "vk_2_map_iter_stripe.comp"; Define = "MERGED_IMAGE_HDR"; Output = "vk_2_map_iter_stripe_hdr.comp.spv" }
)

foreach ($variant in $variants) {
    $inputFile = Join-Path $srcDir $variant.Source
    $outputFile = Join-Path $stageDir $variant.Output

    Write-Host "Compiling: " -NoNewline
    Write-Host $variant.Source -NoNewline -ForegroundColor Yellow
    Write-Host " (-D$($variant.Define)) -> " -NoNewline
    Write-Host $variant.Output -ForegroundColor Green

    & $glslc "-D$($variant.Define)" $inputFile -o $outputFile

    if ($LASTEXITCODE -ne 0) {
        Remove-Item -LiteralPath $stageDir -Recurse -Force
        Write-Error "Failed to compile $inputFile with -D$($variant.Define)"
        exit 1
    }
}

try {
    if (Test-Path -LiteralPath $dstDir) {
        Move-Item -LiteralPath $dstDir -Destination $backupDir -ErrorAction Stop
    }
    Move-Item -LiteralPath $stageDir -Destination $dstDir -ErrorAction Stop
} catch {
    if (Test-Path -LiteralPath $backupDir) {
        Move-Item -LiteralPath $backupDir -Destination $dstDir -ErrorAction Stop
    }
    Write-Error "Could not deploy compiled shaders: $_"
    exit 1
} finally {
    if (Test-Path -LiteralPath $stageDir) {
        Remove-Item -LiteralPath $stageDir -Recurse -Force
    }
}
if (Test-Path -LiteralPath $backupDir) {
    Remove-Item -LiteralPath $backupDir -Recurse -Force
}
Write-Host "All shaders compiled successfully." -ForegroundColor Cyan
