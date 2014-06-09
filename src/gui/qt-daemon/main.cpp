#include <QApplication>
#include "html5applicationviewer.h"
#include "qdebug.h"
#include <thread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setWindowTitle("Boolberry");
    viewer.loadFile(QLatin1String("html/index.html"));
    if(!viewer.start_backend(argc, argv))
      return false;
    return app.exec();
}
