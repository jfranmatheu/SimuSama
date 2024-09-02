@echo off
setlocal

cd "%~dp0"

:: Set paths (adjust these according to your system)
set DLL_NAME=SimuSama
set PYD_NAME=mpm_simulation

:: Copy the DLL to the Python directory (adjust the Python path as needed)
copy ".\x64\Release\%DLL_NAME%.dll" ".\pysimusama\%PYD_NAME%.pyd"
copy ".\x64\Release\%DLL_NAME%.dll" ".\x64\Release\%PYD_NAME%.pyd"

echo DLL to PYD successful.