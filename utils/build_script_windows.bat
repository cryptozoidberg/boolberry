SET QT_PREFIX_PATH=C:\Qt\Qt5.3.0\5.3\msvc2013_64
SET INNOSETUP_PATH=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
SET QT_BINARIES_PATH=C:\home\deploy\qt-binaries
SET BUILDS_PATH=C:\home\deploy\lui
SET ACHIVE_NAME_PREFIX=lui-win-x64-
SET SOURCES_PATH=C:\home\deploy\lui\src


@echo on

cd %SOURCES_PATH%
git pull
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------- BUILDING CONSOLE APPLICATIONS ----------------"
@echo "---------------------------------------------------------------"

IF %ERRORLEVEL% NEQ 0 (
  goto error
)



rmdir build /s /q
mkdir build
cd build
cmake -G "Visual Studio 12 Win64" ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

setLocal 
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

msbuild version.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/daemon.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simplewallet.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simpleminer.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)
endlocal

@echo on

 
set cmd=src\Release\simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

cd src\release
zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip luid.exe simplewallet.exe simpleminer.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip simplewallet.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip luid.exe

IF %ERRORLEVEL% NEQ 0 (
  goto error
)


cd ..\..\..
`
@echo "---------------- BUILDING GUI ---------------------------------"
@echo "---------------------------------------------------------------"

rmdir build /s /q
mkdir build
cd build
cmake -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%" -D BUILD_GUI=TRUE  -D STATIC=FALSE -G "Visual Studio 12 Win64" ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

setLocal 

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

msbuild version.vcxproj  /p:Configuration=Release /t:Build

echo 'errorlevel=%ERRORLEVEL%'

IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/qt-lui.vcxproj  /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)

endlocal

@echo on
cd src\release


zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip qt-lui.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"

cd %SOURCES_PATH%\src\gui\qt-daemon\
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add qt stuff"

cd %QT_BINARIES_PATH%
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


@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"



goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

pause



