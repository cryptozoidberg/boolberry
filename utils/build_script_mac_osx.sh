cd boolberry
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

cmake -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH=/Users/roky/Qt/5.3/clang_64 -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to cmake"
    exit $?
fi



make qt-boolb
if [ $? -ne 0 ]; then
    echo "Failed to make qt-boolb"
    exit $?
fi


cd src/
if [ $? -ne 0 ]; then
    echo "Failed to cd src"
    exit $?
fi

/Users/roky/Qt/5.3/clang_64/bin/macdeployqt qt-boolb.app
if [ $? -ne 0 ]; then
    echo "Failed to macdeployqt qt-boolb.app"
    exit $?
fi

cp -R ../../../src/gui/qt-daemon/html qt-boolb.app/Contents/MacOS
if [ $? -ne 0 ]; then
    echo "Failed to cp html to MacOS"
    exit $?
fi

cp ../../../src/gui/qt-daemon/app.icns qt-boolb.app/Contents/Resources
if [ $? -ne 0 ]; then
    echo "Failed to cp app.icns to resources"
    exit $?
fi

zip -r -y "bbr-macos-x64-v0.2.0.zip" qt-boolb.app
if [ $? -ne 0 ]; then
    echo "Failed to zip app"
    exit $?
fi

cd ../..
echo "Build success"

