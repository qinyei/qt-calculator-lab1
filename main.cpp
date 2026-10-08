#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // 高分屏（本机为 2560x1600 @150%）下让 Qt 自己按 DPI 缩放，
    // 否则界面会被系统整体拉伸，文字发虚、控件比例失真。
    // 注意：这个属性必须在创建 QApplication 之前设置。
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
