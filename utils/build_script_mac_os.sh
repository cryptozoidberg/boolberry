cd boolberry
git pull
rm -rf build; mkdir -p build/release; cd build/release;
cmake -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH=/Users/andre/Qt/5.3/clang_64 -D CMAKE_BUILD_TYPE=Release ../..
make qt-boolb
cd src/
/Users/andre/Qt/5.3/clang_64/bin/macdeployqt qt-boolb.app
cp -R ../../../src/gui/qt-daemon/html qt-boolb.app/Contents/MacOS
zip -r -y "bbr-macos-x64-v0.2.0.zip" qt-boolb.app
cd ../..

