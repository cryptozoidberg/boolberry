Building
--------

### Unix and MacOS X

Dependencies: GCC 4.7.3 or later, CMake 2.8.6 or later, and Boost 1.53(but don't use 1.54) or later. You may download them from:
http://gcc.gnu.org/
http://www.cmake.org/
http://www.boost.org/
Alternatively, it may be possible to install them using a package manager.

More detailed instructions for OS X (assume you’re using MacPorts (they’re, however, pretty self-explanatory and homebrew users shouldn't have troubles following it too):

* Install latest Xcode and command line tools (these are in fact MacPorts prerequisites)
* Install GCC 4.8 and toolchain selector tool: `sudo port install gcc48 gcc_select`
* Set GCC 4.8 as an active compiler toolchain: `sudo port select --set gcc mp-gcc48`
* Install latest Boost, CMake: `sudo port install boost cmake`

To build, change to a directory where this file is located, and run `make`. The resulting executables can be found in `build/release/src`.

**Advanced options**:

Parallel build: run `make -j<number of threads>` instead of just `make`.

Debug build: run `make build-debug`.

Test suite: run `make test-release` to run tests in addition to building. Running `make test-debug` will do the same to the debug version.

Building with Clang: it may be possible to use Clang instead of GCC, but this may not work everywhere. To build, run `export CC=clang CXX=clang++` before running `make`.

### Windows

Download and install Microsoft Visual Studio 2013 from: https://www.microsoft.com/en-us/download/details.aspx?id=44914
Download and install CMAKE from: https://cmake.org/download/
Download and install Boost from: https://sourceforge.net/projects/boost/files/boost-binaries/
 - Specifically for Boost, download and install at least version 1.53 or later (but not 1.54 or 1.55) (Boost 1.56 will be used in this    tutorial)
 - Install Boost to C:\boost\boost_1_56_0 or some other directory you wish to put it in

Next you need to opn up the VS2013 Command Prompt found here: "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\Shortcuts\VS2013 x64 Cross Tools Command Prompt"

Inside the command prompt navigate to the home directory of the Boolberry source code.

Next run the following commands:
```bash
 mkdir build
 cd build
 cmake -DBOOST_ROOT=C:\boost\boost_1_56_0 -G "Visual Studio 12 Win64" ..
 msbuild.exe boolberry.sln /p:Configuration=Release
```
This resulting executables can be found here: \build\src\Release
