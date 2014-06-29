#!/bin/bash

QT_PREFIX_PATH="/home/roky/Qt/5.3/gcc_64"
QT_BINARIES_PATH="/home/roky/bbr_binaries"


cd boolberry
prj_root=$(pwd)

git pull
if [ $? -ne 0 ]; then
    echo "Failed to pull"
    exit $?
fi

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

rm -rf build; mkdir -p build/release; cd build/release; 
cmake -D STATIC=true -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit $?
fi

make daemon qt-boolb simplewallet simpleminer connectivity_tool;
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit $?
fi

 
read version_str <<< $(./src/boolbd --version | awk '/^Boolberry / { print $2 }')
echo $version_str

cd src
cp -Rv ../../../src/gui/qt-daemon/html .
cp -Rv $QT_BINARIES_PATH/qt-boolb.sh .
cp -Rv $QT_BINARIES_PATH/libs .


tar -cjvf linux-x64-$version_str.tar.bz2 html qt-boolb.sh libs boolbd qt-boolb simplewallet simpleminer connectivity_tool
if [ $? -ne 0 ]; then
    echo "Failed to pack"
    exit $?
fi



echo "Build success"

exit 0
