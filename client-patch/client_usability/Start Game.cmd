@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\client\start-client.ps1"
if errorlevel 1 pause
