greaterThan(QT_MAJOR_VERSION, 4):QT += widgets webkitwidgets

# Add more folders to ship with the application, here
folder_01.source = html
folder_01.target = .
DEPLOYMENTFOLDERS = folder_01

CONFIG += static

# Define TOUCH_OPTIMIZED_NAVIGATION for touch optimization and flicking
#DEFINES += TOUCH_OPTIMIZED_NAVIGATION
INCLUDEPATH += C:/local/boost_1_55_0 \
               ../../ \
               ../../../contrib\epee\include \
               ../../build/version/ \
               ../../../contrib


QMAKE_CXXFLAGS += /bigobj /MP /W3 /GS- /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345 /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /DGTEST_HAS_TR1_TUPLE=0 /D__SSE4_1__


LIBS += ../../build/src/Release/rpc.lib \
        ../../build/src/Release/currency_core.lib \
        ../../build/src/Release/crypto.lib \
        ../../build/src/Release/common.lib \
        ../../build/contrib/miniupnpc/Release/miniupnpc.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_system-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_filesystem-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_thread-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_date_time-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_chrono-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_regex-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_serialization-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_atomic-vc120-mt-s-1_55.lib \
        C:\local\boost_1_55_0\lib64-msvc-12.0\lib\libboost_program_options-vc120-mt-s-1_55.lib

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Please do not modify the following two lines. Required for deployment.
include(html5applicationviewer/html5applicationviewer.pri)
qtcAddDeployment()


