@echo off
rem Starts the dedicated server from the package folder. Usage: run-server.cmd [launch script, default CTF]
cd /d "%~dp0"
if "%~1"=="" (bv2dedicated.exe CTF) else (bv2dedicated.exe %*)
