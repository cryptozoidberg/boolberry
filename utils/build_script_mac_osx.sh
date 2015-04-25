set -x #echo on


git pull
if [ $? -ne 0 ]; then
    echo "Failed to git pull"
    exit $?
fi

rm -rf build; mkdir -p build/release; cd build/release;

cmake -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH=/Users/roky/Qt/5.3/clang_64 -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to cmake"
    exit $?
fi



make qt-lui
if [ $? -ne 0 ]; then
    echo "Failed to make qt-lui"
    exit $?
fi


cd src/
if [ $? -ne 0 ]; then
    echo "Failed to cd src"
    exit $?
fi

/Users/roky/Qt/5.3/clang_64/bin/macdeployqt qt-lui.app
if [ $? -ne 0 ]; then
    echo "Failed to macdeployqt qt-lui.app"
    exit $?
fi

cp -R ../../../src/gui/qt-daemon/html qt-lui.app/Contents/MacOS
if [ $? -ne 0 ]; then
    echo "Failed to cp html to MacOS"
    exit $?
fi

cp ../../../src/gui/qt-daemon/app.icns qt-lui.app/Contents/Resources
if [ $? -ne 0 ]; then
    echo "Failed to cp app.icns to resources"
    exit $?
fi

zip -r -y "lui-macos-x64-v0.1.0.zip" qt-lui.app
if [ $? -ne 0 ]; then
    echo "Failed to zip app"
    exit $?
fi

cd ../..
echo "Build success"

