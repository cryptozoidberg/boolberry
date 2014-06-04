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

    std::thread th([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      qDebug() << "starting";
      viewer.after_load();
    });


    return app.exec();
}
