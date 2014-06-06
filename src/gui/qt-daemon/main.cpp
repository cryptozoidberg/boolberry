#include <QApplication>
#include "html5applicationviewer.h"
#include "qdebug.h"
#include <thread>
#include "daemon_backend.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setWindowTitle("Boolberry");
    viewer.loadFile(QLatin1String("html/index.html"));
    daemon_backend daemb;
    daemb.start(argc, argv, &viewer);
    return app.exec();
}
