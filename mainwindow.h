#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QPixmap>
#include "mosaichandler.h" // 引入子线程类
#include <QMutex>
#include <QMutexLocker>
// 启用OpenGL渲染的窗口（硬件加速绘制）
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    // 重写事件
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override; // 窗口移动时更新截图区域

private slots:
    // 接收子线程的马赛克图像，更新绘制
    void onMosaicReady(const QPixmap &mosaicPixmap);

private:
    // 初始化窗口和子线程
    void initWindow();

    // 子线程对象（处理截图+马赛克）
    MosaicHandler *m_mosaicHandler;
    // 缓存马赛克图像（用于绘制）
    QPixmap m_mosaicPixmap;
    // 鼠标拖动相关
    QPoint m_dragStartPos;
    bool m_isDragging;
    QMutex m_mutex;             // 成员锁（保护临界区，生命周期与类一致）

};

#endif // MAINWINDOW_H
