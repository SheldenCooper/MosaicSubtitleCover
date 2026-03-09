QT += core gui widgets opengl openglwidgets  # 新增opengl模块

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MosaicSubtitleCover
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++17

# 启用OpenGL渲染（硬件加速）
QT += gui-private
CONFIG += opengl

SOURCES += main.cpp \
           mainwindow.cpp \
           mosaichandler.cpp  # 新增子线程文件

HEADERS += mainwindow.h \
           mosaichandler.h  # 新增子线程头文件

win32 {
    CONFIG += console no_console
    RC_ICONS =
    # Windows下启用高DPI和OpenGL加速
    DEFINES += QT_OPENGL_ES_2 QT_OPENGL_FREEGLUT
}

unix:!macx {
    CONFIG += link_pkgconfig
}

macx {
    CONFIG += x86_64 arm64
    QMAKE_INFO_PLIST = Info.plist
}
