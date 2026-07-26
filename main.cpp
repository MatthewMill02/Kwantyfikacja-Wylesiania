#include "mainwindow.h"
#include "theme/apptheme.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AppTheme::apply(a);

    MainWindow w;
    w.show();
    return QApplication::exec();
}
