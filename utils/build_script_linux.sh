#!/bin/bash

QT_PREFIX_PATH="$HOME/Qt5.4.1/5.4/gcc_64"
QT_BINARIES_PATH="$HOME/lui_binaries"


prj_root=$(pwd)

git pull
if [ $? -ne 0 ]; then
    echo "Failed to pull"
    exit $?
fi

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

echo "Backupping wallet files(on the off-chance)"
#cp -v build/release/src/*.bin build/release/src/*.bin.keys build/release/src/*.bbr build/release/src/*.bbr.keys build/release/src/*.bbr.address.txt build/release/src/*.bin.address.txt ..

echo "Building...." 
rm -rf build; mkdir -p build/release; cd build/release; 
cmake -D STATIC=true -D ARCH=x86-64 -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit 1
fi

make -j daemon qt-lui simplewallet simpleminer connectivity_tool;
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi

 
read version_str <<< $(./src/luid --version | awk '/^Lui / { print $2 }')
echo $version_str


mkdir -p lui;

cp -Rv ../../src/gui/qt-daemon/html ./lui
cp -Rv ../../utils/qt-lui.sh ./lui
cp -Rv $QT_BINARIES_PATH/libs ./lui
cp -Rv $QT_BINARIES_PATH/libs ./lui
cp -Rv src/luid src/qt-lui src/simplewallet src/simpleminer src/connectivity_tool ./lui


tar -cjvf lui-linux-x64-$version_str.tar.bz2 lui
if [ $? -ne 0 ]; then
    echo "Failed to pack"
    exit 1
fi



echo "Build success"

exit 0
