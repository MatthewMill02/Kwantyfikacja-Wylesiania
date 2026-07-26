#ifndef OSMMAPWIDGET_H
#define OSMMAPWIDGET_H

#include <QHash>
#include <QNetworkReply>
#include <QPointF>
#include <QWidget>

/**
 * Interaktywna mapa świata (kafelki OpenStreetMap, bez klucza API).
 * Dwa rogi bbox: pin 1 = (X,Y), pin 2 = (XX,YY).
 * Synchronizacja dwukierunkowa z polami współrzędnych w MainWindow.
 */
class OsmMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OsmMapWidget(QWidget *parent = nullptr);
    ~OsmMapWidget() override;

    void setBbox(double xmin, double ymin, double xmax, double ymax);
    void setCorner1(double lon, double lat);
    void setCorner2(double lon, double lat);

signals:
    void bboxChanged(double xmin, double ymin, double xmax, double ymax);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onTileFetched();

private:
    enum class DragMode { None, Pan, Pin1, Pin2 };

    static QString tileKey(int z, int x, int y);

    QPointF lonLatToMercator(double lon, double lat) const;
    QPointF mercatorToLonLat(const QPointF &mercator) const;
    QPointF lonLatToWorldPixel(double lon, double lat) const;
    QPointF worldPixelToLonLat(const QPointF &worldPixel) const;
    QPointF lonLatToWidget(double lon, double lat) const;
    QPointF widgetToLonLat(const QPointF &widgetPos) const;

    void emitBboxIfChanged();
    void requestVisibleTiles();
    void drawTiles(QPainter &painter);
    void drawOverlay(QPainter &painter);
    int hitTestPin(const QPointF &pos) const;

    double m_xmin = -55.86;
    double m_ymin = -7.58;
    double m_xmax = -55.81;
    double m_ymax = -7.555;

    double m_centerLon = -55.835;
    double m_centerLat = -7.5675;
    int m_zoom = 10;

    DragMode m_dragMode = DragMode::None;
    QPointF m_lastMousePos;

    bool m_blockSignals = false;

    QNetworkAccessManager *m_network = nullptr;
    QHash<QString, QPixmap> m_tileCache;
    QHash<QNetworkReply *, QString> m_pendingTiles;
};

#endif // OSMMAPWIDGET_H
