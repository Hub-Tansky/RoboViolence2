@echo off
rem Starts the client from the package folder. Arguments are one console command, e.g. run-client.cmd connect 127.0.0.1
cd /d "%~dp0"
start "" bv2.exe %*
