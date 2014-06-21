SET QT_PREFIX_PATH=C:\Qt\Qt5.3.0\5.3\msvc2013_64
SET QT_BINARIES_PATH=C:\home\projects\binaries\qt-daemon


cd boolberry
git pull
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------- BUILDING CONSOLE APPLICATIONS ----------------"
@echo "---------------------------------------------------------------"

rmdir build /s /q
mkdir build
cd build
cmake -G "Visual Studio 11 Win64" ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

setLocal 
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64

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

 
set cmd=src\Release\simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

cd src\release
zip ..\..\..\..\builds\bbr-win64-%version%.zip boolbd.exe simplewallet.exe simpleminer.exe
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

cd ..\..\..

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

msbuild src/qt-boolb.vcxproj  /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)

endlocal

cd src\release
zip ..\..\..\..\builds\bbr-win64-%version%.zip qt-boolb.exe

IF %ERRORLEVEL% NEQ 0 (
  goto error
)

@echo "Add html"

cd ..\..\..\src\gui\qt-daemon\
zip -r ..\..\..\..\builds\bbr-win64-%version%.zip html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add qt stuff"

cd %QT_BINARIES_PATH%
zip -r C:\jenkins\workdir\builds\bbr-win64-%version%.zip *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


cd ..

@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"





goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

