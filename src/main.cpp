#include <QApplication>
#include <QDebug>
#include "MainWindow.h"

int main(int argc, char* argv[]) {

    QApplication app(argc, argv);
    qDebug() << "Qt Version:" << QT_VERSION_STR;

    MainWindow window;
    window.show();

    return app.exec();
}



