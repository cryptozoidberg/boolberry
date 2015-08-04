SET QT_PREFIX_PATH=C:\Qt\Qt5.3.0\5.3\msvc2013_64
SET QT32_PREFIX_PATH=C:\Qt\Qt5.5.0_32\5.5\msvc2013
SET INNOSETUP_PATH=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
SET QT_BINARIES_PATH=C:\home\deploy\qt-binaries
SET QT32_BINARIES_PATH=C:\home\deploy\qt-binaries-32
SET BUILDS_PATH=C:\home\deploy\lui
SET ACHIVE_NAME_PREFIX=lui-win
SET SOURCES_PATH=C:\home\deploy\lui\src
SET LOCAL_BOOST_PATH=C:\local\boost_1_56_0
SET LOCAL_BOOST_LIB_PATH=C:\local\boost_1_56_0\stage
SET LOCAL_BOOST_PATH_32=C:\local\boost_1_56_0_32
SET LOCAL_BOOST_LIB_PATH_32=C:\local\boost_1_56_0_32\lib32-msvc-12.0



@echo on

cd %SOURCES_PATH%
git pull
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------- BUILDING ---------------------------------"
@echo "---------------------------------------------------------------"

rmdir build /s /q
mkdir build
cd build

set BOOST_ROOT=%LOCAL_BOOST_PATH_32%
set BOOST_LIBRARYDIR=%LOCAL_BOOST_LIB_PATH_32%

cmake -D CMAKE_PREFIX_PATH="%QT32_PREFIX_PATH%" -D BUILD_GUI=TRUE  -D STATIC=FALSE -G "Visual Studio 12" ..
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


@echo "---------------- BUILDING TOOLS ---------------------------------"


msbuild src/daemon.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simplewallet.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

 
set cmd=src\Release\simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

cd src\release
zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip luid.exe 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip simplewallet.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)



cd ..\..

@echo "---------------- BUILDING GUI ---------------------------------"


msbuild src/qt-lui.vcxproj  /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo on
cd src\release
echo '%version%'
zip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip qt-lui.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"
echo '%version%'
cd %SOURCES_PATH%\src\gui\qt-daemon\
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add qt stuff"
echo '%version%'
cd %QT32_BINARIES_PATH%
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip *.*
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


unzip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%-x86-%version%.zip -d installer_src
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


"%INNOSETUP_PATH%"  /dBinariesPath=../build/installer_src /DMyAppVersion=%version% /o%BUILDS_PATH%\builds\ /f%ACHIVE_NAME_PREFIX%-x86-%version%-installer ..\utils\setup.iss 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"

endlocal


goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

cd ../..
pause



