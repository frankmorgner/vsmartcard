$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"
Set-PSDebug -Trace 1

$TestCertFile = Join-Path (Get-Location) "BixVReader.cer"
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=UMDF Test Certificate' -KeyExportPolicy Exportable -CertStoreLocation Cert:\CurrentUser\My
Export-Certificate -Cert $cert -FilePath "$TestCertFile"

$VS_PATH = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
& "$VS_PATH\Common7\Tools\Launch-VsDevShell.ps1"
Enter-VsDevShell -VsInstallPath "$VS_PATH" -Arch $env:VCVARS_PLATFORM -HostArch $env:PROCESSOR_ARCHITECTURE -SkipAutomaticLocation

& cl.exe /MT /Ivirtualsmartcard\src\vpcd virtualsmartcard\src\vpcd-config\vpcd-config.c virtualsmartcard\src\vpcd-config\local-ip.c ws2_32.lib

& msbuild "virtualsmartcard\win32\BixVReader.sln" "/p:Configuration=Release;Platform=$env:MSBUILD_PLATFORM;TestCertFile=$TestCertFile"

& python --version
& python -m pip install -q --upgrade pip
& python -m pip install -q virtualenv
& python -m pip install -q -U setuptools
& python -m pip install -q pycryptodomex
& python -m pip install -q pbkdf2
& python -m pip install -q Pillow
& python -m pip install -q pyreadline3
& python -m pip install -q pyscard
& python -m pip install -q pyinstaller

& python -m PyInstaller --onefile virtualsmartcard\src\vpicc\vicc.in -i doc\_static\chip.ico
