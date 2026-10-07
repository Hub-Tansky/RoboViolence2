@echo off
rem Starts the master server (TCP 10207) from the package folder; it reads master.db and web.db here.
cd /d "%~dp0"
bv2master.exe
