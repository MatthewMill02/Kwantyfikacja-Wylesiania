#include "imagepanelwidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QResizeEvent>

ImagePanelWidget::ImagePanelWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel(title, this))
    , m_imageLabel(new QLabel(this))
{
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: 600; padding: 2px;"));

    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(160, 120);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_imageLabel->setStyleSheet(
        QStringLiteral("QLabel { background: #1e1e1e; border: 1px solid #555; }"));
    m_imageLabel->setText(QStringLiteral("Brak obrazu"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_imageLabel, 1);
}

void ImagePanelWidget::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void ImagePanelWidget::setImage(const QImage &image)
{
    m_source = image;
    updatePixmap();
}

void ImagePanelWidget::clearImage()
{
    m_source = QImage();
    m_imageLabel->setPixmap(QPixmap());
    m_imageLabel->setText(QStringLiteral("Brak obrazu"));
}

void ImagePanelWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePixmap();
}

void ImagePanelWidget::updatePixmap()
{
    if (m_source.isNull()) {
        return;
    }

    const QSize target = m_imageLabel->size();
    if (target.width() < 8 || target.height() < 8) {
        return;
    }

    const QPixmap pix = QPixmap::fromImage(m_source).scaled(
        target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setText(QString());
    m_imageLabel->setPixmap(pix);
}
