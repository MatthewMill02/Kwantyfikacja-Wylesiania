#include "imagepanelwidget.h"

#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QScrollBar>

namespace {

constexpr int kMinDisplayHeight = 280;
constexpr double kMinZoom = 1.0;
constexpr double kMaxZoom = 6.0;

} // namespace

ImagePanelWidget::ImagePanelWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel(title, this))
    , m_imageLabel(new QLabel())
    , m_scrollArea(new QScrollArea(this))
{
    m_titleLabel->setObjectName(QStringLiteral("imagePanelTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_imageLabel->setObjectName(QStringLiteral("imagePanelPreview"));
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_imageLabel->setMinimumSize(160, 120);
    m_imageLabel->setText(QStringLiteral("Brak obrazu"));

    m_scrollArea->setObjectName(QStringLiteral("imagePanelScroll"));
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->viewport()->installEventFilter(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_scrollArea, 0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void ImagePanelWidget::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void ImagePanelWidget::setImage(const QImage &image)
{
    m_source = image;
    resetZoom();
    updatePixmap();
}

void ImagePanelWidget::clearImage()
{
    m_source = QImage();
    resetZoom();
    m_imageLabel->setPixmap(QPixmap());
    m_imageLabel->setMinimumSize(160, 120);
    m_imageLabel->resize(160, 120);
    m_imageLabel->setText(QStringLiteral("Brak obrazu"));
    m_scrollArea->setMinimumHeight(120);
    m_scrollArea->setMaximumHeight(120);
    updateGeometry();
}

void ImagePanelWidget::setMaxFitHeight(int height)
{
    m_maxFitHeight = qMax(0, height);
    updatePixmap();
}

QSize ImagePanelWidget::sizeHint() const
{
    const auto *box = layout();
    const QMargins margins = box ? box->contentsMargins() : QMargins{};
    const int spacing = box ? box->spacing() : 0;
    const int titleH = m_titleLabel->sizeHint().height();
    const int scrollH = m_scrollArea->maximumHeight() > 0
        ? m_scrollArea->maximumHeight()
        : m_scrollArea->minimumHeight();
    return {400, titleH + scrollH + margins.top() + margins.bottom() + spacing};
}

void ImagePanelWidget::resetZoom()
{
    m_zoomFactor = 1.0;
}

void ImagePanelWidget::applyZoom(double factor)
{
    m_zoomFactor = qBound(kMinZoom, m_zoomFactor * factor, kMaxZoom);
    updatePixmap();
}

double ImagePanelWidget::fittedScaleForViewport(int viewportWidth, int maxFitHeight) const
{
    if (m_source.isNull() || viewportWidth <= 0) {
        return 1.0;
    }

    const double scaleW = static_cast<double>(viewportWidth) / m_source.width();
    double scale = scaleW;

    if (maxFitHeight > 0) {
        const double scaleH = static_cast<double>(maxFitHeight) / m_source.height();
        scale = qMin(scaleW, scaleH);
        return scale;
    }

    int h = qMax(1, static_cast<int>(m_source.height() * scale));
    if (h < kMinDisplayHeight) {
        const double heightScale = static_cast<double>(kMinDisplayHeight) / m_source.height();
        if (m_source.width() * heightScale <= viewportWidth) {
            scale = heightScale;
        }
    }

    return scale;
}

void ImagePanelWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_source.isNull()) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        if (!m_source.isNull()) {
            updatePixmap();
        }
    });
}

void ImagePanelWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePixmap();
}

bool ImagePanelWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_scrollArea->viewport() || event->type() != QEvent::Wheel) {
        return QWidget::eventFilter(watched, event);
    }

    auto *wheelEvent = static_cast<QWheelEvent *>(event);
    const Qt::KeyboardModifiers mods = wheelEvent->modifiers();

    if (mods & Qt::ControlModifier) {
        const double step = wheelEvent->angleDelta().y() > 0 ? 1.12 : 1.0 / 1.12;
        applyZoom(step);
        return true;
    }

    if ((mods & Qt::ShiftModifier) && m_zoomFactor > 1.001) {
        QScrollBar *hBar = m_scrollArea->horizontalScrollBar();
        if (hBar->maximum() > hBar->minimum()) {
            int delta = 0;
            if (!wheelEvent->pixelDelta().isNull()) {
                delta = wheelEvent->pixelDelta().y();
            } else {
                delta = wheelEvent->angleDelta().y() / 4;
            }
            hBar->setValue(hBar->value() - delta);
        }
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void ImagePanelWidget::updatePixmap()
{
    if (m_source.isNull()) {
        return;
    }

    if (!isVisible()) {
        return;
    }

    const int vpW = m_scrollArea->viewport()->width();
    if (vpW < 80) {
        return;
    }

    m_fitScale = fittedScaleForViewport(vpW, m_maxFitHeight);

    const int w = qMax(1, static_cast<int>(m_source.width() * m_fitScale * m_zoomFactor));
    const int h = qMax(1, static_cast<int>(m_source.height() * m_fitScale * m_zoomFactor));

    const QPixmap pix = QPixmap::fromImage(m_source).scaled(
        w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setText(QString());
    m_imageLabel->setPixmap(pix);
    m_imageLabel->resize(pix.size());

    const int fittedH = qMax(1, static_cast<int>(m_source.height() * m_fitScale));
    const bool zoomedIn = m_zoomFactor > 1.001;

    if (zoomedIn) {
        m_scrollArea->setMinimumHeight(fittedH);
        m_scrollArea->setMaximumHeight(fittedH);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        m_scrollArea->setMinimumHeight(h);
        m_scrollArea->setMaximumHeight(h);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    updateGeometry();
}
