#include "splitpanelwidget.h"

#include <QPainter>
#include <QPen>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QShowEvent>

SplitPreviewWidget::SplitPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("splitPanelPreview"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize SplitPreviewWidget::displaySizeForBounds(int maxWidth, int maxHeight) const
{
    if (m_left.isNull() || maxWidth <= 0 || maxHeight <= 0) {
        return {};
    }

    const double scaleW = static_cast<double>(maxWidth) / m_left.width();
    const double scaleH = static_cast<double>(maxHeight) / m_left.height();
    const double scale = qMin(scaleW, scaleH);
    return {qMax(1, static_cast<int>(m_left.width() * scale)),
            qMax(1, static_cast<int>(m_left.height() * scale))};
}

QSize SplitPreviewWidget::sizeHint() const
{
    return displaySizeForBounds(640, 360);
}

bool SplitPreviewWidget::hasImages() const
{
    return !m_left.isNull() && !m_right.isNull();
}

void SplitPreviewWidget::setImages(const QImage &left, const QImage &right)
{
    m_left = left;
    m_right = right;
    updateGeometry();
    update();
}

void SplitPreviewWidget::setSplitPercent(int percent)
{
    m_splitPercent = qBound(0, percent, 100);
    update();
}

void SplitPreviewWidget::clearImages()
{
    m_left = QImage();
    m_right = QImage();
    updateGeometry();
    update();
}

void SplitPreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(QStringLiteral("#0c1219")));

    if (m_left.isNull() || m_right.isNull()) {
        painter.setPen(QColor(QStringLiteral("#64748b")));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Brak obrazu"));
        return;
    }

    const QSize area = size();
    const int w = area.width();
    const int h = area.height();

    const QImage leftScaled = m_left.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QImage rightScaled = m_right.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int drawW = leftScaled.width();
    const int drawH = leftScaled.height();
    const int offsetX = (w - drawW) / 2;
    const int offsetY = (h - drawH) / 2;

    const int splitX = offsetX + (drawW * m_splitPercent) / 100;

    painter.drawImage(offsetX, offsetY, leftScaled);

    painter.save();
    painter.setClipRect(splitX, offsetY, offsetX + drawW - splitX, drawH);
    painter.drawImage(offsetX, offsetY, rightScaled);
    painter.restore();

    painter.setPen(QPen(QColor(QStringLiteral("#60a5fa")), 2));
    painter.drawLine(splitX, offsetY, splitX, offsetY + drawH);

    painter.setBrush(QColor(QStringLiteral("#2563eb")));
    painter.setPen(QPen(QColor(QStringLiteral("#e8edf4")), 1));
    const int handleH = qMin(36, drawH);
    painter.drawRoundedRect(splitX - 6, offsetY + drawH / 2 - handleH / 2, 12, handleH, 4, 4);
}

SplitPanelWidget::SplitPanelWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel(title, this))
    , m_previewHost(new QWidget(this))
    , m_preview(new SplitPreviewWidget(m_previewHost))
    , m_modeCombo(new QComboBox(m_previewHost))
    , m_leftCaption(new QLabel(this))
    , m_rightCaption(new QLabel(this))
    , m_splitSlider(new QSlider(Qt::Horizontal, this))
{
    m_titleLabel->setObjectName(QStringLiteral("imagePanelTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_previewHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_modeCombo->setObjectName(QStringLiteral("splitPanelModeCombo"));
    m_modeCombo->addItem(QStringLiteral("True Color"));
    m_modeCombo->addItem(QStringLiteral("False Color"));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SplitPanelWidget::modeChanged);

    m_leftCaption->setObjectName(QStringLiteral("splitPanelCaption"));
    m_rightCaption->setObjectName(QStringLiteral("splitPanelCaption"));

    m_splitSlider->setObjectName(QStringLiteral("splitPanelSlider"));
    m_splitSlider->setRange(0, 100);
    m_splitSlider->setValue(50);
    m_splitSlider->setToolTip(QStringLiteral("Przesuń, aby porównać oba lata"));

    auto *captionRow = new QHBoxLayout();
    captionRow->setContentsMargins(0, 0, 0, 0);
    captionRow->setSpacing(0);
    captionRow->addWidget(m_leftCaption);
    captionRow->addStretch();
    captionRow->addWidget(m_rightCaption);

    m_controlsHost = new QWidget(this);
    auto *controlsLayout = new QVBoxLayout(m_controlsHost);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(0);
    controlsLayout->addLayout(captionRow);
    controlsLayout->addWidget(m_splitSlider);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_previewHost, 1);
    layout->addWidget(m_controlsHost, 0, Qt::AlignHCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(m_splitSlider, &QSlider::valueChanged, this, &SplitPanelWidget::onSplitChanged);
}

int SplitPanelWidget::modeIndex() const
{
    return m_modeCombo ? m_modeCombo->currentIndex() : 0;
}

QSize SplitPanelWidget::minimumSizeHint() const
{
    return {120, 200};
}

QSize SplitPanelWidget::sizeHint() const
{
    const auto *rootLayout = layout();
    const QMargins margins = rootLayout ? rootLayout->contentsMargins() : QMargins{};
    const int spacing = rootLayout ? rootLayout->spacing() : 0;
    const int titleH = m_titleLabel ? m_titleLabel->sizeHint().height() : 20;
    const int previewH = m_preview && m_preview->hasImages() ? m_preview->height() : 200;
    const int controlsH = (m_leftCaption ? m_leftCaption->sizeHint().height() : 16)
        + (m_splitSlider ? m_splitSlider->sizeHint().height() : 24);
    const int w = width() > 0 ? width() : 320;
    return {w, titleH + previewH + controlsH + spacing * 2 + margins.top() + margins.bottom()};
}

void SplitPanelWidget::updateLayoutGeometry()
{
    if (!m_preview || !m_previewHost || !m_controlsHost || !m_splitSlider) {
        return;
    }

    if (!m_preview->hasImages()) {
        m_previewHost->setMinimumSize(0, 0);
        m_previewHost->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_controlsHost->setFixedWidth(0);
        m_splitSlider->setFixedWidth(0);
        return;
    }

    const auto *rootLayout = layout();
    const QMargins margins = rootLayout ? rootLayout->contentsMargins() : QMargins{};
    const int hMargin = margins.left() + margins.right();
    const int spacing = rootLayout ? rootLayout->spacing() : 0;

    const int availW = qMax(120, contentsRect().width() - hMargin);
    const int titleH = m_titleLabel->sizeHint().height();
    const int captionH = m_leftCaption->sizeHint().height();
    const int sliderH = m_splitSlider->sizeHint().height();
    const int controlsBlockH = captionH + sliderH;
    const int previewAreaH = qMax(
        120,
        m_previewHost->height() > 0
            ? m_previewHost->height()
            : contentsRect().height() - titleH - controlsBlockH - spacing * 2 - margins.top() - margins.bottom());

    const QSize previewSize = m_preview->displaySizeForBounds(availW, previewAreaH);
    if (previewSize.isEmpty()) {
        return;
    }

    m_preview->setFixedSize(previewSize);
    m_preview->move(qMax(0, (m_previewHost->width() - previewSize.width()) / 2),
                    qMax(0, (m_previewHost->height() - previewSize.height()) / 2));

    m_controlsHost->setFixedWidth(previewSize.width());
    m_splitSlider->setFixedWidth(previewSize.width());

    updateOverlayPosition();
    updateGeometry();
}

void SplitPanelWidget::updateOverlayPosition()
{
    if (!m_previewHost || !m_preview || !m_modeCombo) {
        return;
    }

    m_modeCombo->adjustSize();

    constexpr int margin = 8;
    const int x = m_preview->x() + qMax(margin, m_preview->width() - m_modeCombo->width() - margin);
    const int y = m_preview->y() + qMax(margin, m_preview->height() - m_modeCombo->height() - margin);
    m_modeCombo->move(x, y);
    m_modeCombo->raise();
}

void SplitPanelWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLayoutGeometry();
}

void SplitPanelWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateLayoutGeometry();
}

void SplitPanelWidget::setImages(const QImage &leftYear, const QImage &rightYear,
                                 const QString &leftLabel, const QString &rightLabel)
{
    m_leftLabel = leftLabel;
    m_rightLabel = rightLabel;
    m_leftCaption->setText(leftLabel);
    m_rightCaption->setText(rightLabel);
    m_preview->setImages(leftYear, rightYear);
    updateLayoutGeometry();
}

void SplitPanelWidget::clearImages()
{
    m_leftCaption->clear();
    m_rightCaption->clear();
    m_preview->clearImages();
    m_previewHost->setMinimumSize(0, 0);
    m_previewHost->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    m_controlsHost->setFixedWidth(0);
    m_splitSlider->setFixedWidth(0);
}

void SplitPanelWidget::onSplitChanged(int value)
{
    m_preview->setSplitPercent(value);
}
