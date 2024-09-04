#include <QApplication>
#include <QDebug>
#include "MainWindow.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>

typedef boost::geometry::model::d2::point_xy<double> Point;
typedef boost::geometry::model::polygon<Point> Polygon;

int main(int argc, char* argv[]) {

    QApplication app(argc, argv);
    qDebug() << "Qt Version:" << QT_VERSION_STR;

    MainWindow window;
    window.show();

    return app.exec();
}



