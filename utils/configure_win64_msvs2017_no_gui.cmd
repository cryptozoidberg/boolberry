call configure_win64_settings.cmd

@echo off
if not defined BOOST_ROOT (
  echo:
  echo ERROR: BOOST_ROOT env variable is not defined. Define it system-wide or put into configure_win64_settings.cmd
  exit /b 1
)

cd ..
@mkdir build_msvc2017_64
cd build_msvc2017_64

cmake -D BUILD_GUI=FALSE -D STATIC=TRUE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-14.1" -G "Visual Studio 15 2017 Win64" ".."
