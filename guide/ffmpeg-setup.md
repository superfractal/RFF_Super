<!-- Modified by GPT-6 on 2026-09-25. -->
# FFmpeg setup for Windows

RFF_Super uses FFmpeg for video and audio export. Install it separately; it is not needed for still-image rendering.

## 1. Download and extract

Open the [official FFmpeg download page](https://ffmpeg.org/download.html) and choose a provider under **Windows EXE Files**. Download a Windows x64 build with the encoders listed below, then extract the archive. A static build is convenient for placing beside the application; shared builds also need their accompanying DLLs.

## 2. Make FFmpeg available

Choose either method:

- **Beside the application:** put `ffmpeg.exe` in the same folder as `RFF_Super.exe` (normally the application's `bin/` folder). Include any DLLs required by your FFmpeg build.
- **On PATH:** keep the extracted folder, for example `C:\Tools\ffmpeg`, and add the folder containing `ffmpeg.exe` (usually `C:\Tools\ffmpeg\bin`) to your user `Path` through Windows **Environment Variables → User variables → Path → Edit → New**. Reopen PowerShell and restart RFF_Super after saving.

The video exporter checks beside `RFF_Super.exe` first, then falls back to PATH. An older copy beside the application therefore takes priority over one on PATH.

## 3. Check the installation

For a PATH installation, run these commands in a new PowerShell window:

```powershell
ffmpeg -version
ffmpeg -hide_banner -encoders | Select-String 'libx264|libx264rgb|libx265'
```

For an installation beside the application, open PowerShell in the folder containing `RFF_Super.exe` and use `.\ffmpeg.exe` in place of `ffmpeg` in the command examples on this page.

| Video export mode | Required encoder |
| --- | --- |
| Standard SDR | `libx264` |
| Lossless SDR | `libx264rgb` |
| HDR (PQ or HLG) | `libx265` with 10-bit output support |

Only the encoder for your chosen mode is required. For HDR, run `ffmpeg -hide_banner -h encoder=libx265` and check that its supported pixel formats include `yuv420p10le`.

Restart RFF_Super and try a short export using the [animation and export guide](animation-and-export.md).

## If export fails

- **Command not found:** check that PATH points to the folder containing `ffmpeg.exe`, then reopen PowerShell and restart the application.
- **Missing DLL:** restore the complete FFmpeg package. Copying only the executable from an MSYS2 or shared build is insufficient.
- **Missing encoder:** choose a build that includes the encoder for your export mode. Check the copy beside RFF_Super as well as the one on PATH.

For the MSYS2 installation method, see [Build instructions: Supply FFmpeg](../documentation/BUILDING.md#5-supply-ffmpeg).
