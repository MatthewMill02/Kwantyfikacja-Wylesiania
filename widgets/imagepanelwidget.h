#ifndef IMAGEPANELWIDGET_H
#define IMAGEPANELWIDGET_H

#include <QWidget>
#include <QImage>

class QLabel;

/**
 * Panel jednej grafiki wynikowej: tytuł + podgląd (skalowany z zachowaniem proporcji).
 */
class ImagePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImagePanelWidget(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setImage(const QImage &image);
    void clearImage();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updatePixmap();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_imageLabel = nullptr;
    QImage m_source;
};

#endif // IMAGEPANELWIDGET_H
