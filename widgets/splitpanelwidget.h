#ifndef SPLITPANELWIDGET_H
#define SPLITPANELWIDGET_H

#include <QImage>
#include <QWidget>

class QComboBox;
class QLabel;
class QSlider;

/** Wewnętrzny podgląd — maluje split bez ustawiania pixmapy (bez rozpychania layoutu). */
class SplitPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SplitPreviewWidget(QWidget *parent = nullptr);

    void setImages(const QImage &left, const QImage &right);
    void setSplitPercent(int percent);
    void clearImages();
    QSize displaySizeForBounds(int maxWidth, int maxHeight) const;
    bool hasImages() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    QImage m_left;
    QImage m_right;
    int m_splitPercent = 50;
};

/**
 * Porównanie dwóch obrazów lat — suwak odsłania lewy/prawy rok (efekt before/after).
 */
class SplitPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SplitPanelWidget(const QString &title, QWidget *parent = nullptr);

    void setImages(const QImage &leftYear, const QImage &rightYear,
                   const QString &leftLabel, const QString &rightLabel);
    void clearImages();
    int modeIndex() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void modeChanged(int index);

private slots:
    void onSplitChanged(int value);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateLayoutGeometry();
    void updateOverlayPosition();

    QString m_leftLabel;
    QString m_rightLabel;

    QLabel *m_titleLabel = nullptr;
    QWidget *m_previewHost = nullptr;
    SplitPreviewWidget *m_preview = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QWidget *m_controlsHost = nullptr;
    QLabel *m_leftCaption = nullptr;
    QLabel *m_rightCaption = nullptr;
    QSlider *m_splitSlider = nullptr;
};

#endif // SPLITPANELWIDGET_H
