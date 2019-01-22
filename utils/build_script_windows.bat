SET QT_PREFIX_PATH=C:\dev\_sdk\Qt5.11.2\5.11.2\msvc2017_64
SET INNOSETUP_PATH=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
SET ETC_BINARIES_PATH=C:\dev\deploy\etc-binaries
SET ACHIVE_NAME_PREFIX=boolberry-win-x64-webegine-
SET BUILDS_PATH=C:\dev\deploy\boolberry
set BOOST_ROOT=C:\dev\_sdk\boost_1_68_0
set BOOST_LIBRARYDIR=C:\dev\_sdk\boost_1_68_0\lib64-msvc-14.1
set CERT_FILEPATH=C:\dev\cert\boolberry\boolberry_cert.pfx
SET MY_PATH=%~dp0
SET SOURCES_PATH=%MY_PATH:~0,-7%

cd %SOURCES_PATH%
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo on

@echo "---------------- BUILDING CONSOLE APPLICATIONS ----------------"
@echo "---------------------------------------------------------------"

setLocal 

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat" x86_amd64

set skip_build_flag=
if "%1"=="skip_build"   set skip_build_flag=true
if "%1"=="--skip-build" set skip_build_flag=true
if defined skip_build_flag goto skip_build

rmdir build /s /q
mkdir build
cd build
cmake -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%" -D BUILD_GUI=TRUE -D STATIC=FALSE -G "Visual Studio 15 2017 Win64" -T host=x64 ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)



msbuild version.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/daemon.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simplewallet.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/Boolberry.vcxproj /p:SubSystem="WINDOWS,5.02" /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)

:skip_build

cd %SOURCES_PATH%\build\src\release

@echo "Version: "

set cmd=simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

@echo "Signing...."
@echo on
signtool sign /f %CERT_FILEPATH% /p %BBR_CERT_PASS% Boolberry.exe
signtool sign /f %CERT_FILEPATH% /p %BBR_CERT_PASS% boolbd.exe
signtool sign /f %CERT_FILEPATH% /p %BBR_CERT_PASS% simplewallet.exe

@echo "Copying...."
mkdir bunch
copy /Y Boolberry.exe bunch
copy /Y boolbd.exe bunch
copy /Y simplewallet.exe bunch

%QT_PREFIX_PATH%\bin\windeployqt.exe bunch\Boolberry.exe

set PACKAGE_ZIP_FILENAME=%ACHIVE_NAME_PREFIX%%version%.zip
set PACKAGE_EXE_FILENAME=%ACHIVE_NAME_PREFIX%%version%-installer.exe
set PACKAGE_EXE_FILESTEM=%ACHIVE_NAME_PREFIX%%version%-installer
set PACKAGE_ZIP_PATH=%BUILDS_PATH%\builds\%PACKAGE_ZIP_FILENAME%
set PACKAGE_EXE_PATH=%BUILDS_PATH%\builds\%PACKAGE_EXE_FILENAME%

cd bunch

zip -r %PACKAGE_ZIP_PATH% *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"


cd %SOURCES_PATH%\src\gui\qt-daemon\
zip -r %PACKAGE_ZIP_PATH% html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo "Other stuff"

cd %ETC_BINARIES_PATH%
zip -r %PACKAGE_ZIP_PATH% *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


cd %SOURCES_PATH%\build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------------------------------------------------------"
@echo "-------------------Building installer--------------------------"
@echo "---------------------------------------------------------------"

mkdir installer_src


unzip %PACKAGE_ZIP_PATH% -d installer_src
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


"%INNOSETUP_PATH%"  /dBinariesPath=../build/installer_src /DMyAppVersion=%version% /o%BUILDS_PATH%\builds\ /f%PACKAGE_EXE_FILESTEM% ..\utils\setup.iss 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo on
@echo "Signing installer..."
signtool sign /f %CERT_FILEPATH% /p %BBR_CERT_PASS% %PACKAGE_EXE_PATH%
IF %ERRORLEVEL% NEQ 0 (
  goto error
)



@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"

pscp -load boolberry_build_server %PACKAGE_ZIP_PATH% %BBR_BUILD_SERVER_ADDR%:/var/www/html/builds
IF %ERRORLEVEL% NEQ 0 (
  @echo "error on uploading zip package to the server"
  goto error
)

pscp -load boolberry_build_server %PACKAGE_EXE_PATH% %BBR_BUILD_SERVER_ADDR%:/var/www/html/builds
IF %ERRORLEVEL% NEQ 0 (
  @echo "error on uploading exe package to the server"
  goto error
)

set mail_msg="New build for win-x64 available at http://%BBR_BUILD_SERVER_ADDR%:8081/builds/%PACKAGE_EXE_FILENAME% <br><br>ZIP:  http://%BBR_BUILD_SERVER_ADDR%:8081/builds/%PACKAGE_ZIP_FILENAME%"

echo %mail_msg%

senditquiet.exe -t %emails% -subject "Boolberry win-x64 build %version%" -body %mail_msg%


goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

exit 0
