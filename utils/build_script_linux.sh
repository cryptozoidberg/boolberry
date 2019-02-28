#!/bin/bash

set -e
curr_path=${BASH_SOURCE%/*}

: "${BOOST_ROOT:?BOOST_ROOT should be set to the root of Boost, ex.: /home/user/boost_1_68_0}"
: "${QT_PREFIX_PATH:?QT_PREFIX_PATH should be set to Qt libs folder, ex.: /home/user/Qt5.5.1/5.5.1/gcc_64}"

echo "entering directory $curr_path/.."
cd $curr_path/..

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

echo "Backupping wallet files(on the off-chance)"
cp -v build/release/src/*.bin build/release/src/*.bin.keys build/release/src/*.bbr build/release/src/*.bbr.keys build/release/src/*.bbr.address.txt build/release/src/*.bin.address.txt .. || true

printf "\nbuilding....\n"
rm -rf build; mkdir -p build/release; cd build/release
cmake -D STATIC=TRUE -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..

make -j daemon
make -j Boolberry
make -j simplewallet
make -j connectivity_tool

read version_str <<< $(./src/boolbd --version | awk '/^Boolberry / { print $2 }')
echo $version_str

printf "\npreparing final deploy folder...\n\n"
mkdir -p boolberry

cp -Rv ../../src/gui/qt-daemon/html ./boolberry
cp  ../../utils/Boolberry.sh ./boolberry
cp  src/boolbd ./boolberry
cp  src/Boolberry ./boolberry
cp  src/simplewallet ./boolberry
cp  src/connectivity_tool ./boolberry
chmod 777 ./boolberry/Boolberry.sh

mkdir ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libicudata.so.56 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libicui18n.so.56 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libicuuc.so.56 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Core.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5DBus.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Gui.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Network.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5OpenGL.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Positioning.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5PrintSupport.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Qml.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Quick.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Sensors.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Sql.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5Widgets.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngine.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineCore.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineWidgets.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5WebChannel.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5XcbQpa.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/lib/libQt5QuickWidgets.so.5 ./boolberry/lib
cp $QT_PREFIX_PATH/libexec/QtWebEngineProcess ./boolberry
cp $QT_PREFIX_PATH/resources/qtwebengine_resources.pak ./boolberry
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_100p.pak ./boolberry
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_200p.pak ./boolberry
cp $QT_PREFIX_PATH/resources/icudtl.dat ./boolberry

mkdir ./boolberry/lib/platforms
cp $QT_PREFIX_PATH/plugins/platforms/libqxcb.so ./boolberry/lib/platforms
mkdir ./boolberry/xcbglintegrations
cp $QT_PREFIX_PATH/plugins/xcbglintegrations/libqxcb-glx-integration.so ./boolberry/xcbglintegrations

printf "\nmaking compressed build archive...\n\n"

package_filename=boolberry-linux-x64-$version_str.tar.bz2

tar -cjvf $package_filename boolberry

printf "\nbuild succeeded!\n"

echo "uploading to build server..."
scp $package_filename bbr_build_server:/var/www/html/builds


mail_msg="New build for linux-x64 available at http://$BBR_BUILD_SERVER_ADDR_PORT/builds/$package_filename"
echo $mail_msg
echo $mail_msg | mail -s "Boolberry linux-x64 build $version_str" ${emails}

exit 0
