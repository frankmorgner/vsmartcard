@echo off
:: -------------------------------
:: Dedicated variables for expansions
:: -------------------------------
set "SCRIPT_FULL=%~f0"            REM Full path to this script
set "SCRIPT_DIR=%~dp0"            REM Directory containing this script

set "CERT_FILE=%SCRIPT_DIR%BixVReader.cer"
set "MSI_FILE=%SCRIPT_DIR%BixVReaderInstaller.msi"

:: -------------------------------
:: Check for Admin Rights and self-elevate
:: -------------------------------
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrative privileges...
    powershell -Command "Start-Process '%SCRIPT_FULL%' -Verb RunAs"
    exit /b
)

:: -------------------------------
:: Check testsigning
:: -------------------------------
bcdedit /enum {current} | findstr /i "testsigning" | findstr /i "Yes" >nul
if %errorlevel% neq 0 (
    echo ERROR: "testsigning" is not enabled.
    echo Please enable testsigning by running: bcdedit /set testsigning on
    echo A reboot will be required afterwards.
    pause
    exit /b 1
)

echo Testsigning is enabled. Proceeding with installation.
echo.

:: -------------------------------
:: Validate files exist
:: -------------------------------
if not exist "%CERT_FILE%" (
    echo ERROR: Certificate file not found: %CERT_FILE%
    pause
    exit /b 1
)

if not exist "%MSI_FILE%" (
    echo ERROR: MSI file not found: %MSI_FILE%
    pause
    exit /b 1
)

:: -------------------------------
:: Import Developer Certificate
:: -------------------------------
echo Importing certificate into Trusted Root...
certutil -addstore -f "Root" "%CERT_FILE%"
if %errorlevel% neq 0 (
    echo ERROR: Failed to import certificate into Root store.
    pause
    exit /b 1
)

echo Importing certificate into Trusted Publisher...
certutil -addstore -f "TrustedPublisher" "%CERT_FILE%"
if %errorlevel% neq 0 (
    echo ERROR: Failed to import certificate into TrustedPublisher store.
    pause
    exit /b 1
)

echo Certificate imported successfully.
echo.

:: -------------------------------
:: Install Driver
:: -------------------------------
echo Installing driver via msiexec...
msiexec /i "%MSI_FILE%"  /L* "install.log" /qn /norestart
if %errorlevel% neq 0 (
    echo ERROR: Driver installation failed.
    pause
    exit /b 1
)

echo.
echo =============================================
echo Driver installation process completed.
echo =============================================
pause
