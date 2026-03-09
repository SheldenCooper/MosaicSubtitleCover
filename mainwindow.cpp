#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QOpenGLWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mosaicHandler(new MosaicHandler(this))
    , m_isDragging(false)
{
    initWindow();

    // 连接子线程信号：马赛克图像就绪 → 触发重绘
    connect(m_mosaicHandler, &MosaicHandler::mosaicReady,
            this, &MainWindow::onMosaicReady);

    // 启动子线程（异步处理）
    m_mosaicHandler->start();
}

MainWindow::~MainWindow()
{
    // 停止子线程，避免内存泄漏
    m_mosaicHandler->stop();
}

// 初始化窗口（硬件加速+无边框+置顶）
void MainWindow::initWindow()
{
    // 1. 启用OpenGL渲染（硬件加速绘制，解决卡顿）
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // 2. 窗口基础配置（1000×200，无边框，置顶，透明）
    setFixedSize(1000, 200);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    // 3. 初始位置（屏幕居中）
    QScreen *screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    move((screenRect.width() - width()) / 2, (screenRect.height() - height()) / 2);

    // 4. 初始化截图区域（当前窗口位置）
    m_mosaicHandler->setCaptureParams(geometry(), 8);
}

// 双击最小化
void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    showMinimized();
}

// 硬件加速绘制（流畅无闪烁）
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform); // 抗锯齿，更流畅
    // 绘制马赛克图像（覆盖整个窗口）
    if (!m_mosaicPixmap.isNull()) {
        painter.drawPixmap(rect(), m_mosaicPixmap);
    }
}

// 鼠标按下：记录拖动起点
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_isDragging = true;
    }
    QMainWindow::mousePressEvent(event);
}

// 鼠标拖动：更新窗口位置
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = event->globalPosition().toPoint() - m_dragStartPos;
        move(newPos);
    }
    QMainWindow::mouseMoveEvent(event);
}

// 窗口移动：更新截图区域（保证马赛克跟随窗口）
void MainWindow::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
    // 线程安全更新截图区域
    m_mosaicHandler->setCaptureParams(geometry(), 8);
}

// 接收子线程的马赛克图像，更新缓存并触发重绘
void MainWindow::onMosaicReady(const QPixmap &mosaicPixmap)
{
    QMutexLocker locker(&m_mutex); // 作用域内自动加锁，出作用域自动解锁
    m_mosaicPixmap = mosaicPixmap;
    update(); // 触发paintEvent，绘制新的马赛克图像
}
