@echo off
setlocal

cd "%~dp0"

:: Set paths (adjust these according to your system)
set SOLUTION_NAME="SimuSama"
set OUTPUT_PYD_NAME="simusama"
set PYTHON_VERSION="Python311"
set VS_VERSION="2022"

set VSDEV="C:\Program Files (x86)\Microsoft Visual Studio\%VS_VERSION%\Community\Common7\Tools\VsDevCmd.bat"
set PROJECT_DIR="."
set PYTHON_PATH="%LOCALAPPDATA%\Programs\Python\%PYTHON_VERSION%"
set PYBIND11_INCLUDE="%PROJECT_DIR%\extern\pybind11\include"
set EIGEN_INCLUDE="%PROJECT_DIR%\extern\eigen"
set OUTPUT_PYD_PATH="%PROJECT_DIR%\python\%OUTPUT_PYD_NAME%.pyd"
:: "%PYTHON_PATH%\Lib\site-packages\%OUTPUT_PYD_NAME%.pyd"

:: Initialize VS Developer Command Prompt
call %VSDEV%

:: Build the project
msbuild %PROJECT_DIR%\%SOLUTION_NAME%.sln /p:Configuration=Release /p:Platform=x64 ^
    /p:AdditionalIncludeDirectories="%PYTHON_PATH%\include;%EIGEN_INCLUDE%;%PYBIND11_INCLUDE%" ^
    /p:AdditionalLibraryDirectories="%PYTHON_PATH%\libs"

:: Copy the DLL to the Python directory (adjust the Python path as needed)
copy %PROJECT_DIR%\x64\Release\%SOLUTION_NAME%.dll %OUTPUT_PYD_PATH%

echo Build complete.