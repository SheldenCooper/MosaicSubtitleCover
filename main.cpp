#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // 1. 配置OpenGL格式（硬件加速基础）
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3); // 兼容主流显卡
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapInterval(1); // 垂直同步，避免撕裂
    QSurfaceFormat::setDefaultFormat(format);

    // 2. Qt6高DPI适配（避免高分屏模糊）
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
