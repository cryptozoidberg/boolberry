// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include "html5applicationviewer/html5applicationviewer.h"
#include "qdebug.h"
#include <thread>
#include "qtlogger.h"


int main(int argc, char *argv[])
{
    string_tools::set_module_name_and_folder(argv[0]);

#ifndef _MSC_VER
    epee::log_space::log_singletone::add_logger(new qt_console_stream());
#endif
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setWindowTitle("Luidor");
   
    if(!viewer.start_backend(argc, argv))
      return false;
    viewer.init_config();
    return app.exec();
}
