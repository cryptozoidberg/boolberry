set -x # echo on
set -e # stop on errors
curr_path=${BASH_SOURCE%/*}

: "${BOOST_ROOT:?BOOST_ROOT should be set to the root of Boost, ex.: /home/user/boost_1_56_0}"
: "${BOOST_LIBS_PATH:?BOOST_LIBS_PATH should be set to libs folder of Boost, ex.: /home/user/boost_1_56_0/stage/lib}"
: "${QT_PREFIX_PATH:?QT_PREFIX_PATH should be set to Qt libs folder, ex.: /home/user/Qt5.5.1/5.5/}"
: "${CMAKE_OSX_SYSROOT:?CMAKE_OSX_SYSROOT should be set to macOS SDK path, e.g.: /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk}"

echo "entering directory $curr_path/.."
cd $curr_path/..

rm -rf build
mkdir -p build/release
cd build/release

cmake -D CMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH/clang_64" -D CMAKE_BUILD_TYPE=Release ../..

make -j Boolberry
make -j connectivity_tool simplewallet daemon

cd src

printf "\nCopying boost files...\n\n"

mkdir -p Boolberry.app/Contents/Frameworks/boost_libs
cp -R "$BOOST_LIBS_PATH/" Boolberry.app/Contents/Frameworks/boost_libs/

cp simplewallet Boolberry.app/Contents/MacOS
cp boolbd Boolberry.app/Contents/MacOS

# fix boost links
echo "Fixing boost library links...."
source ../../../utils/fix_boost_libs.sh
update_links_in_boost_binary @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/MacOS/Boolberry
update_links_in_boost_binary @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/MacOS/boolbd
update_links_in_boost_binary @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/MacOS/simplewallet
update_links_in_boost_libs @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/Frameworks/boost_libs


$QT_PREFIX_PATH/clang_64/bin/macdeployqt Boolberry.app
cp -R ../../../src/gui/qt-daemon/html Boolberry.app/Contents/MacOS
cp ../../../src/gui/qt-daemon/app.icns Boolberry.app/Contents/Resources


read version_str <<< $(DYLD_LIBRARY_PATH=$LOCAL_BOOST_LIBS_PATH ./connectivity_tool --version | awk '/^Boolberry / { print $2 }')
printf "\nVersion built: $version_str\n\n"

echo "############### Prepearing archive... ################"
mkdir package_folder

ln -s /Applications package_folder/Applications 

mv Boolberry.app package_folder 

package_filename="boolberry-macos-x64-$version_str.dmg"

hdiutil create -format UDZO -srcfolder package_folder -volname Boolberry $package_filename

printf "\nbuild succeeded!\n\n"


echo "############### uploading... ################"

scp $package_filename bbr_build_server:/var/www/html/builds/

mail_msg="New build for macOS-x64 available at http://$BBR_BUILD_SERVER_ADDR_PORT/builds/$package_filename"

echo $mail_msg

echo $mail_msg | mail -s "Boolberry macOS-x64 build $version_str" ${emails}

cd ../..
exit 0
