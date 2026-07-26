#ifndef IMAGEPANELWIDGET_H
#define IMAGEPANELWIDGET_H

#include <QWidget>
#include <QImage>

class QLabel;
class QScrollArea;
class QShowEvent;

/**
 * Panel jednej grafiki wynikowej: zoom Ctrl+kółko, przewijanie w poziomie Shift+kółko (gdy przybliżone).
 */
class ImagePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImagePanelWidget(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setImage(const QImage &image);
    void clearImage();
    /** Ogranicza wysokość podglądu — grafika mieści się w całości (min. skala szer./wys.). */
    void setMaxFitHeight(int height);

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void resetZoom();
    void applyZoom(double factor);
    void updatePixmap();

    double fittedScaleForViewport(int viewportWidth, int maxFitHeight) const;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_imageLabel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QImage m_source;
    double m_fitScale = 1.0;
    double m_zoomFactor = 1.0;
    int m_maxFitHeight = 0;
};

#endif // IMAGEPANELWIDGET_H
