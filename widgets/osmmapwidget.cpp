#include "osmmapwidget.h"

#include <QMouseEvent>
#include <QNetworkRequest>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>

namespace {

constexpr double kOriginShift = 20037508.342789244;
constexpr int kTileSize = 256;
constexpr int kMaxZoom = 18;
constexpr int kMinZoom = 2;
constexpr int kMaxCacheTiles = 256;
constexpr int kPinRadius = 10;

} // namespace

QString OsmMapWidget::tileKey(int z, int x, int y)
{
    return QStringLiteral("%1/%2/%3").arg(z).arg(x).arg(y);
}

OsmMapWidget::OsmMapWidget(QWidget *parent)
    : QWidget(parent)
    , m_network(new QNetworkAccessManager(this))
{
    setObjectName(QStringLiteral("osmMapWidget"));
    setMinimumSize(420, 320);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

OsmMapWidget::~OsmMapWidget() = default;

QPointF OsmMapWidget::lonLatToMercator(double lon, double lat) const
{
    const double x = lon * kOriginShift / 180.0;
    const double latRad = qDegreesToRadians(qBound(-85.0, lat, 85.0));
    const double y = std::log(std::tan(M_PI / 4.0 + latRad / 2.0)) * kOriginShift / M_PI;
    return {x, y};
}

QPointF OsmMapWidget::mercatorToLonLat(const QPointF &mercator) const
{
    const double lon = mercator.x() / kOriginShift * 180.0;
    const double latRad = std::atan(std::sinh(M_PI * mercator.y() / kOriginShift));
    return {lon, qRadiansToDegrees(latRad)};
}

QPointF OsmMapWidget::lonLatToWorldPixel(double lon, double lat) const
{
    const double worldSize = kTileSize * std::pow(2.0, m_zoom);
    const QPointF merc = lonLatToMercator(lon, lat);
    const double px = (merc.x() + kOriginShift) / (2.0 * kOriginShift) * worldSize;
    const double py = (kOriginShift - merc.y()) / (2.0 * kOriginShift) * worldSize;
    return {px, py};
}

QPointF OsmMapWidget::worldPixelToLonLat(const QPointF &worldPixel) const
{
    const double worldSize = kTileSize * std::pow(2.0, m_zoom);
    const double mx = worldPixel.x() / worldSize * (2.0 * kOriginShift) - kOriginShift;
    const double my = kOriginShift - worldPixel.y() / worldSize * (2.0 * kOriginShift);
    return mercatorToLonLat({mx, my});
}

QPointF OsmMapWidget::lonLatToWidget(double lon, double lat) const
{
    const QPointF centerPx = lonLatToWorldPixel(m_centerLon, m_centerLat);
    const QPointF pointPx = lonLatToWorldPixel(lon, lat);
    return {width() / 2.0 + (pointPx.x() - centerPx.x()),
            height() / 2.0 + (pointPx.y() - centerPx.y())};
}

QPointF OsmMapWidget::widgetToLonLat(const QPointF &widgetPos) const
{
    const QPointF centerPx = lonLatToWorldPixel(m_centerLon, m_centerLat);
    const QPointF worldPx = {centerPx.x() + (widgetPos.x() - width() / 2.0),
                           centerPx.y() + (widgetPos.y() - height() / 2.0)};
    return worldPixelToLonLat(worldPx);
}

void OsmMapWidget::setBbox(double xmin, double ymin, double xmax, double ymax)
{
    m_blockSignals = true;
    m_xmin = xmin;
    m_ymin = ymin;
    m_xmax = xmax;
    m_ymax = ymax;
    m_centerLon = (xmin + xmax) / 2.0;
    m_centerLat = (ymin + ymax) / 2.0;
    m_blockSignals = false;
    requestVisibleTiles();
    update();
}

void OsmMapWidget::setCorner1(double lon, double lat)
{
    m_blockSignals = true;
    m_xmin = qMin(lon, m_xmax - 1e-6);
    m_ymin = qMin(lat, m_ymax - 1e-6);
    m_blockSignals = false;
    emitBboxIfChanged();
    update();
}

void OsmMapWidget::setCorner2(double lon, double lat)
{
    m_blockSignals = true;
    m_xmax = qMax(lon, m_xmin + 1e-6);
    m_ymax = qMax(lat, m_ymin + 1e-6);
    m_blockSignals = false;
    emitBboxIfChanged();
    update();
}

void OsmMapWidget::emitBboxIfChanged()
{
    if (m_blockSignals) {
        return;
    }
    emit bboxChanged(m_xmin, m_ymin, m_xmax, m_ymax);
}

void OsmMapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    requestVisibleTiles();
}

void OsmMapWidget::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y() > 0 ? 1 : -1;
    const int newZoom = qBound(kMinZoom, m_zoom + delta, kMaxZoom);
    if (newZoom != m_zoom) {
        m_zoom = newZoom;
        requestVisibleTiles();
        update();
    }
    event->accept();
}

int OsmMapWidget::hitTestPin(const QPointF &pos) const
{
    const QPointF p1 = lonLatToWidget(m_xmin, m_ymin);
    const QPointF p2 = lonLatToWidget(m_xmax, m_ymax);

    if (QLineF(pos, p1).length() <= kPinRadius + 4) {
        return 1;
    }
    if (QLineF(pos, p2).length() <= kPinRadius + 4) {
        return 2;
    }
    return 0;
}

void OsmMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_lastMousePos = event->position();
    const int pin = hitTestPin(m_lastMousePos);
    if (pin == 1) {
        m_dragMode = DragMode::Pin1;
    } else if (pin == 2) {
        m_dragMode = DragMode::Pin2;
    } else {
        m_dragMode = DragMode::Pan;
    }
    event->accept();
}

void OsmMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = event->position();

    if (m_dragMode == DragMode::Pan) {
        const QPointF centerPx = lonLatToWorldPixel(m_centerLon, m_centerLat);
        const QPointF newCenterPx = {centerPx.x() - (pos.x() - m_lastMousePos.x()),
                                     centerPx.y() - (pos.y() - m_lastMousePos.y())};
        const QPointF lonLat = worldPixelToLonLat(newCenterPx);
        m_centerLon = lonLat.x();
        m_centerLat = lonLat.y();
        m_lastMousePos = pos;
        requestVisibleTiles();
        update();
        return;
    }

    if (m_dragMode == DragMode::Pin1 || m_dragMode == DragMode::Pin2) {
        const QPointF lonLat = widgetToLonLat(pos);
        if (m_dragMode == DragMode::Pin1) {
            m_xmin = qBound(-180.0, lonLat.x(), m_xmax - 1e-5);
            m_ymin = qBound(-85.0, lonLat.y(), m_ymax - 1e-5);
        } else {
            m_xmax = qBound(m_xmin + 1e-5, lonLat.x(), 180.0);
            m_ymax = qBound(m_ymin + 1e-5, lonLat.y(), 85.0);
        }
        emitBboxIfChanged();
        update();
    }
}

void OsmMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragMode = DragMode::None;
}

void OsmMapWidget::requestVisibleTiles()
{
    const double worldSize = kTileSize * std::pow(2.0, m_zoom);
    const QPointF centerPx = lonLatToWorldPixel(m_centerLon, m_centerLat);

    const double left = centerPx.x() - width() / 2.0;
    const double top = centerPx.y() - height() / 2.0;
    const double right = centerPx.x() + width() / 2.0;
    const double bottom = centerPx.y() + height() / 2.0;

    const int xMin = qMax(0, static_cast<int>(std::floor(left / kTileSize)));
    const int yMin = qMax(0, static_cast<int>(std::floor(top / kTileSize)));
    const int xMax = qMin(static_cast<int>(worldSize / kTileSize) - 1,
                          static_cast<int>(std::floor(right / kTileSize)));
    const int yMax = qMin(static_cast<int>(worldSize / kTileSize) - 1,
                          static_cast<int>(std::floor(bottom / kTileSize)));

    for (int x = xMin; x <= xMax; ++x) {
        for (int y = yMin; y <= yMax; ++y) {
            const QString key = tileKey(m_zoom, x, y);
            if (m_tileCache.contains(key)) {
                continue;
            }

            bool pending = false;
            for (auto it = m_pendingTiles.cbegin(); it != m_pendingTiles.cend(); ++it) {
                if (it.value() == key) {
                    pending = true;
                    break;
                }
            }
            if (pending) {
                continue;
            }

            if (m_tileCache.size() > kMaxCacheTiles) {
                m_tileCache.clear();
            }

            const QUrl url(QStringLiteral("https://tile.openstreetmap.org/%1/%2/%3.png")
                               .arg(m_zoom)
                               .arg(x)
                               .arg(y));
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::UserAgentHeader,
                              QStringLiteral("Wylesianie/1.0 (Qt; academic/educational)"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            request.setTransferTimeout(15000);
#endif
            QNetworkReply *reply = m_network->get(request);
            m_pendingTiles.insert(reply, key);
            connect(reply, &QNetworkReply::finished, this, &OsmMapWidget::onTileFetched);
        }
    }
}

void OsmMapWidget::onTileFetched()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    const QString key = m_pendingTiles.take(reply);
    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pix;
        if (pix.loadFromData(reply->readAll())) {
            m_tileCache.insert(key, pix);
        }
    }
    reply->deleteLater();
    update();
}

void OsmMapWidget::drawTiles(QPainter &painter)
{
    const double worldSize = kTileSize * std::pow(2.0, m_zoom);
    const QPointF centerPx = lonLatToWorldPixel(m_centerLon, m_centerLat);

    const double left = centerPx.x() - width() / 2.0;
    const double top = centerPx.y() - height() / 2.0;

    const int xMin = qMax(0, static_cast<int>(std::floor(left / kTileSize)));
    const int yMin = qMax(0, static_cast<int>(std::floor(top / kTileSize)));
    const int xMax = qMin(static_cast<int>(worldSize / kTileSize) - 1,
                          static_cast<int>(std::ceil((left + width()) / kTileSize)));
    const int yMax = qMin(static_cast<int>(worldSize / kTileSize) - 1,
                          static_cast<int>(std::ceil((top + height()) / kTileSize)));

    for (int x = xMin; x <= xMax; ++x) {
        for (int y = yMin; y <= yMax; ++y) {
            const QString key = tileKey(m_zoom, x, y);
            const double drawX = x * kTileSize - left;
            const double drawY = y * kTileSize - top;

            if (m_tileCache.contains(key)) {
                painter.drawPixmap(QRect(qRound(drawX), qRound(drawY), kTileSize, kTileSize),
                                   m_tileCache.value(key));
            } else {
                painter.fillRect(QRectF(drawX, drawY, kTileSize, kTileSize),
                                 QColor(QStringLiteral("#1a2435")));
            }
        }
    }
}

void OsmMapWidget::drawOverlay(QPainter &painter)
{
    const QPointF sw = lonLatToWidget(m_xmin, m_ymin);
    const QPointF ne = lonLatToWidget(m_xmax, m_ymax);
    const QRectF rect(QPointF(qMin(sw.x(), ne.x()), qMin(sw.y(), ne.y())),
                      QPointF(qMax(sw.x(), ne.x()), qMax(sw.y(), ne.y())));

    painter.setPen(QPen(QColor(QStringLiteral("#60a5fa")), 2, Qt::DashLine));
    painter.setBrush(QColor(37, 99, 235, 40));
    painter.drawRect(rect);

    auto drawPin = [&](const QPointF &pos, const QString &label, const QColor &color) {
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(color);
        painter.drawEllipse(pos, kPinRadius, kPinRadius);
        painter.setPen(Qt::white);
        painter.drawText(QRectF(pos.x() - kPinRadius, pos.y() - kPinRadius,
                                kPinRadius * 2, kPinRadius * 2),
                       Qt::AlignCenter, label);
    };

    drawPin(sw, QStringLiteral("1"), QColor(QStringLiteral("#2563eb")));
    drawPin(ne, QStringLiteral("2"), QColor(QStringLiteral("#1d4ed8")));

    painter.setPen(QColor(148, 163, 184));
    painter.drawText(8, height() - 8, QStringLiteral("© OpenStreetMap contributors"));
    painter.drawText(8, 18, QStringLiteral("Przeciągnij pinezki · scroll = zoom · LPM = przesuń mapę"));
}

void OsmMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(QStringLiteral("#0c1219")));
    drawTiles(painter);
    drawOverlay(painter);
}
