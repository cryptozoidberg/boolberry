// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include "html5applicationviewer/html5applicationviewer.h"
#include "qdebug.h"
#include <thread>

int main(int argc, char *argv[])
{
    string_tools::set_module_name_and_folder(argv[0]);

    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setWindowTitle("Boolberry");
    
    viewer.loadFile(QLatin1String((string_tools::get_current_module_folder() + "/html/index.html").c_str()));
    if(!viewer.start_backend(argc, argv))
      return false;
    return app.exec();
}
