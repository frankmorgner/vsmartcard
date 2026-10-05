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

echo Running with administrative privileges.
echo.

:: -------------------------------
:: Validate files exist
:: -------------------------------
if not exist "%MSI_FILE%" (
    echo ERROR: INF file not found: %MSI_FILE%
    pause
    exit /b 1
)


:: -------------------------------
:: Uninstall Driver
:: -------------------------------
echo Uninstalling driver via msiexec...
msiexec /x "%MSI_FILE%" /qn /norestart
if %errorlevel% neq 0 (
    echo ERROR: Driver uninstallation failed or driver not found.
) else (
    echo Driver successfully uninstalled.
)
echo.

:: -------------------------------
:: Optional: Remove Developer Certificate
:: -------------------------------
if exist "%CERT_FILE%" (
    echo Removing developer certificate from Trusted Root...
    certutil -delstore "Root" "%CERT_FILE%"

    echo Removing developer certificate from Trusted Publisher...
    certutil -delstore "TrustedPublisher" "%CERT_FILE%"

    echo Certificate removal completed.
) else (
    echo Certificate file not found: %CERT_FILE%
    echo Skipping certificate removal.
)

echo.
echo =============================================
echo Driver uninstall process completed.
echo =============================================
pause
