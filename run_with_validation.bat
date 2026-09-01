@rem Created by a project contributor on 2026-08-10.
@rem Modified by GPT-5 on 2026-08-21.
@echo off
rem Launches RFF_Super with Vulkan diagnostic layers forced on by the loader, to
rem investigate the VK_ERROR_DEVICE_LOST abort described in README.md.
rem
rem The application itself is untouched: the loader injects the layers and the
rem layers write the logs. Requires the Vulkan SDK to be installed.
rem
rem Default: crash diagnostic only (device-lost / hang analysis).
rem Set RFF_VALIDATION=1 beforehand to also enable the validation layer.

rem vk_layer_settings.txt uses relative paths, so run from this script's folder.
rem Nothing here is tied to one machine.
cd /d "%~dp0"

set "LAYERS=VK_LAYER_LUNARG_crash_diagnostic"
if "%RFF_VALIDATION%"=="1" set "LAYERS=VK_LAYER_KHRONOS_validation,VK_LAYER_LUNARG_crash_diagnostic"

set "VK_LOADER_LAYERS_ENABLE=%LAYERS%"
set "VK_INSTANCE_LAYERS=%LAYERS%"
set "VK_LAYER_SETTINGS_PATH=%~dp0vk_layer_settings.txt"

rem Start clean so the logs only cover this run.
if exist validation.log del validation.log
if exist crash_diagnostic.log del crash_diagnostic.log

echo Layers: %LAYERS%
echo Logs:   %~dp0crash_diagnostic.log
echo.
echo Run the video export until it aborts, then keep the log files, any dump
echo folder created next to this script, and the "terminate called ..." line
echo printed below -- it names the VkResult.
echo.

"%~dp0bin\RFF_Super.exe"

echo.
echo RFF_Super exited with code %ERRORLEVEL%.
pause
