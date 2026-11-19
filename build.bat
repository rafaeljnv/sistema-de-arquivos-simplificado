@echo off
setlocal

set "MINGW_BIN=C:\Program Files\JetBrains\CLion 2026.1.3\bin\mingw\bin"
if exist "%MINGW_BIN%\gcc.exe" (
    set "PATH=%MINGW_BIN%;%PATH%"
)

echo Usando compilador:
gcc --version

echo.
echo Compilando MiniFS (minifs.exe)...
gcc -Wall -Wextra -std=c99 -Iinclude src/disk.c src/bitmap.c src/metadata.c src/fs_ops.c src/cli.c src/main.c -o minifs.exe
if %ERRORLEVEL% NEQ 0 (
    echo Erro ao compilar minifs.exe!
    exit /b 1
)
echo [OK] minifs.exe gerado com sucesso!

echo.
echo Compilando suite de testes (test_minifs.exe)...
gcc -Wall -Wextra -std=c99 -Iinclude src/disk.c src/bitmap.c src/metadata.c src/fs_ops.c tests/test_minifs.c -o test_minifs.exe
if %ERRORLEVEL% NEQ 0 (
    echo Erro ao compilar test_minifs.exe!
    exit /b 1
)
echo [OK] test_minifs.exe gerado com sucesso!

if "%1"=="test" (
    echo.
    echo Executando bateria de testes automatizados...
    echo.
    test_minifs.exe
)
