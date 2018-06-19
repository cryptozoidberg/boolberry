set -x #echo on

export PROJECT_ROOT="/Users/roky/projects/boolberry/repo"
export QT_PATH="/Users/roky/Qt5.5.0/5.5"
export LOCAL_BOOST_LIBS_PATH="/Users/roky/projects/boolberry/deploy/boost_libs"



cd "$PROJECT_ROOT"
if [ $? -ne 0 ]; then
    echo "Failed to cd boolberry"
    exit $?
fi

git pull
if [ $? -ne 0 ]; then
    echo "Failed to git pull"
    exit $?
fi

rm -rf build; mkdir -p build/release; cd build/release;

cmake -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PATH/clang_64" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to cmake"
    exit $?
fi



make -j Boolberry
if [ $? -ne 0 ]; then
    echo "Failed to make Boolberry"
    exit $?
fi

make -j connectivity_tool simplewallet daemon
if [ $? -ne 0 ]; then
    echo "Failed to make connectivity_tool"
    exit $?
fi

cd src/
if [ $? -ne 0 ]; then
    echo "Failed to cd src"
    exit $?
fi


# copy boost files
echo "Copying boost files...."
mkdir -p Boolberry.app/Contents/Frameworks/boost_libs
cp -R "$LOCAL_BOOST_LIBS_PATH/" Boolberry.app/Contents/Frameworks/boost_libs/
if [ $? -ne 0 ]; then
    echo "Failed to cp workaround to MacOS"
    exit 1
fi


# fix boost links
echo "Fixing boost library links...."
source ../../../utils/fix_boost_libs.sh
update_links_in_boost_binary @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/MacOS/Boolberry
update_links_in_boost_libs @executable_path/../Frameworks/boost_libs Boolberry.app/Contents/Frameworks/boost_libs



$QT_PATH/clang_64/bin/macdeployqt Boolberry.app
if [ $? -ne 0 ]; then
    echo "Failed to macdeployqt Boolberry.app"
    exit $?
fi

cp -R ../../../src/gui/qt-daemon/html Boolberry.app/Contents/MacOS
if [ $? -ne 0 ]; then
    echo "Failed to cp html to MacOS"
    exit $?
fi

cp ../../../src/gui/qt-daemon/app.icns Boolberry.app/Contents/Resources
if [ $? -ne 0 ]; then
    echo "Failed to cp app.icns to resources"
    exit $?
fi

cp simplewallet Boolberry.app/Contents/MacOS
if [ $? -ne 0 ]; then
    echo "Failed to cp simplewallet to MacOS"
    exit $?
fi

cp boolbd Boolberry.app/Contents/MacOS
if [ $? -ne 0 ]; then
    echo "Failed to cp boolbd to MacOS"
    exit $?
fi


read version_str <<< $(DYLD_LIBRARY_PATH=$LOCAL_BOOST_LIBS_PATH ./connectivity_tool --version | awk '/^Boolberry / { print $2 }')
echo "Version built: $version_str"



echo "############### Prepearing archive... ################"
mkdir package_folder
if [ $? -ne 0 ]; then
    echo "Failed to zip app"
    exit 1
fi

ln -s /Applications package_folder/Applications 
if [ $? -ne 0 ]; then
    echo "Failed to copy applications link"
    exit 1
fi

mv Boolberry.app package_folder 
if [ $? -ne 0 ]; then
    echo "Failed to top app package"
    exit 1
fi


hdiutil create -format UDZO -srcfolder package_folder -volname Boolberry "Boolberry-macos-x64-$version_str.dmg"
if [ $? -ne 0 ]; then
    echo "Failed to create dmg"
    exit 1
fi


#zip -r -y "bbr-macos-x64-v0.2.0.zip" Boolberry.app
#if [ $? -ne 0 ]; then
#    echo "Failed to zip app"
#    exit $?
# fi

cd ../..
echo "Build success"

