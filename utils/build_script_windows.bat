SET QT_PREFIX_PATH=C:\Qt\Qt5.5.0\5.5\msvc2013_64
SET INNOSETUP_PATH=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
SET QT_BINARIES_PATH=C:\home\projects\binaries\qt-daemon
SET ACHIVE_NAME_PREFIX=Boolberry-win-x64-
SET BUILDS_PATH=C:\home\deploy\boolberry
SET SOURCES_PATH=C:\home\deploy\boolberry\sources\boolberry
set BOOST_ROOT=C:\local\boost_1_56_0
set BOOST_LIBRARYDIR=C:\local\boost_1_56_0\lib64-msvc-12.0
set EXTRA_FILES_PATH=C:\home\deploy\boolberry\extra_files
set CERT_FILEPATH=C:\home\cert\bbr\boolberry.pfx

cd %SOURCES_PATH%

git pull

IF %ERRORLEVEL% NEQ 0 (
  goto error
)
@echo on

@echo "---------------- BUILDING CONSOLE APPLICATIONS ----------------"
@echo "---------------------------------------------------------------"

setLocal 

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

IF "%1"=="skip_build" GOTO skip_build

rmdir build /s /q
mkdir build
cd build
cmake -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%" -D BUILD_GUI=TRUE -D STATIC=FALSE -G "Visual Studio 12 Win64" ..
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


cd bunch

zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"


cd %SOURCES_PATH%\src\gui\qt-daemon\
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo "Other stuff"

cd %ETC_BINARIES_PATH%
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip *.*
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


unzip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip -d installer_src
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


"%INNOSETUP_PATH%"  /dBinariesPath=../build/installer_src /DMyAppVersion=%version% /o%BUILDS_PATH%\builds\ /f%ACHIVE_NAME_PREFIX%%version%-installer ..\utils\setup.iss 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo on
@echo "Signing installer..."
signtool sign /f %CERT_FILEPATH% /p %BBR_CERT_PASS% %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%-installer.exe



@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"



goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

pause



