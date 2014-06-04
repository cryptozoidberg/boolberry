#include <QApplication>
#include "html5applicationviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setWindowTitle("Boolberry");
    viewer.loadFile(QLatin1String("html/index.html"));
    viewer.after_load();
    return app.exec();
}
