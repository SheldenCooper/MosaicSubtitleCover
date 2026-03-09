#include "mosaichandler.h"
#include <QGuiApplication> // Qt6 必须包含，获取屏幕对象
#include <QScreen>         // QScreen::grabRect() 依赖
#include <QImage>
#include <QPainter>
#include <chrono>
#include <thread>
#include <QMutexLocker>

MosaicHandler::MosaicHandler(QObject *parent)
    : QThread(parent)
    , m_isRunning(false)
    , m_blockSize(8)
    , m_fps(30) // 目标帧率：30帧流畅，可改60
{
}

MosaicHandler::~MosaicHandler()
{
    stop(); // 析构时停止线程，避免内存泄漏
}

// 线程安全设置截图区域和马赛克块大小
void MosaicHandler::setCaptureParams(const QRect &captureRect, int blockSize)
{
    QMutexLocker locker(&m_mutex);
    m_captureRect = captureRect;
    m_blockSize = qMax(2, blockSize); // 块大小最小为2，避免无马赛克效果
}

// 停止线程（线程安全）
void MosaicHandler::stop()
{
    QMutexLocker locker(&m_mutex);
    m_isRunning = false;
    m_condition.wakeAll(); // 唤醒等待的线程
    locker.unlock();

    if (isRunning()) {
        wait(1000); // 等待1秒，确保线程退出
    }
}

// 线程主逻辑：异步截图 + 马赛克计算（Qt6官方API）
void MosaicHandler::run()
{
    // 初始化线程运行状态
    QMutexLocker locker(&m_mutex);
    m_isRunning = true;
    QRect captureRect = m_captureRect;
    int blockSize = m_blockSize;
    locker.unlock();

    // 帧间隔（ms）：30帧=33ms，60帧=16ms
    const int frameInterval = 1000 / m_fps;

    while (m_isRunning) {
        // 记录当前帧开始时间（控制帧率）
        auto frameStart = std::chrono::high_resolution_clock::now();

        // 线程安全获取最新参数
        QMutexLocker paramLocker(&m_mutex);
        captureRect = m_captureRect;
        blockSize = m_blockSize;
        paramLocker.unlock();

        // ===== 核心：Qt6全版本兼容的截图（官方推荐grabRect）=====
        QPixmap mosaicPixmap;
        if (!captureRect.isEmpty() && captureRect.width() > 0 && captureRect.height() > 0) {
            // 1. 获取窗口所在屏幕（多屏兼容）
            QScreen *targetScreen = QGuiApplication::screenAt(captureRect.center());
            if (!targetScreen) {
                //  fallback：使用主屏幕
                targetScreen = QGuiApplication::primaryScreen();
            }

            // 2. Qt6官方截图接口：grabRect（替代废弃的grabWindow）
            // 参数：要截取的屏幕区域（全局坐标）
            QPixmap sourcePixmap = targetScreen->grabWindow(              0,                          // 桌面窗口句柄（固定传0）
                                                            captureRect.x(),            // 截图区域左上角X（窗口全局坐标）
                                                            captureRect.y(),            // 截图区域左上角Y（窗口全局坐标）
                                                            captureRect.width(),        // 截图宽度（窗口宽度）
                                                            captureRect.height() );

            // 3. 计算马赛克（优化算法，保证效率）
            if (!sourcePixmap.isNull()) {
                mosaicPixmap = createSmoothMosaic(sourcePixmap.toImage(), blockSize);
            }
        }

        // 发送马赛克图像给主线程绘制
        if (!mosaicPixmap.isNull()) {
            emit mosaicReady(mosaicPixmap);
        }

        // 控制帧率：计算当前帧耗时，补足剩余时间，避免CPU占用过高
        auto frameEnd = std::chrono::high_resolution_clock::now();
        int elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart).count();
        int sleepMs = qMax(1, frameInterval - elapsedMs); // 至少休眠1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

    }
}

// 优化的马赛克算法（直接操作像素，效率高）
QPixmap MosaicHandler::createSmoothMosaic(const QImage &source, int blockSize)
{
    // 空图像直接返回
    if (source.isNull() || blockSize <= 1) {
        return QPixmap::fromImage(source);
    }

    int imgWidth = source.width();
    int imgHeight = source.height();
    QImage mosaicImg(imgWidth, imgHeight, source.format());

    // 遍历每个马赛克块，计算平均色并填充
    for (int y = 0; y < imgHeight; y += blockSize) {
        for (int x = 0; x < imgWidth; x += blockSize) {
            // 计算当前块的有效范围（避免越界）
            int blockW = qMin(blockSize, imgWidth - x);
            int blockH = qMin(blockSize, imgHeight - y);

            // 计算当前块的RGB/A平均值
            long long sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int pixelCount = 0;

            for (int by = 0; by < blockH; ++by) {
                for (int bx = 0; bx < blockW; ++bx) {
                    QRgb pixel = source.pixel(x + bx, y + by);
                    sumR += qRed(pixel);
                    sumG += qGreen(pixel);
                    sumB += qBlue(pixel);
                    sumA += qAlpha(pixel);
                    pixelCount++;
                }
            }

            // 平均色（避免除0）
            QRgb avgColor = qRgba(
                pixelCount > 0 ? sumR / pixelCount : 0,
                pixelCount > 0 ? sumG / pixelCount : 0,
                pixelCount > 0 ? sumB / pixelCount : 0,
                pixelCount > 0 ? sumA / pixelCount : 255
                );

            // 快速填充块（直接操作扫描线，比setPixel快50%+）
            for (int by = 0; by < blockH; ++by) {
                uchar *scanLine = mosaicImg.scanLine(y + by);
                int bpp = mosaicImg.bytesPerLine() / imgWidth; // 每个像素的字节数
                for (int bx = 0; bx < blockW; ++bx) {
                    uchar *pixelPtr = scanLine + (x + bx) * bpp;
                    pixelPtr[0] = qBlue(avgColor);   // B通道
                    pixelPtr[1] = qGreen(avgColor);  // G通道
                    pixelPtr[2] = qRed(avgColor);    // R通道
                    if (bpp == 4) {
                        pixelPtr[3] = qAlpha(avgColor); // A通道（透明）
                    }
                }
            }
        }
    }

    return QPixmap::fromImage(mosaicImg);
}
