#include "geepixelbounds.h"

#include <QtMath>

namespace {

constexpr double kOriginShift = 20037508.342789244;

double lonToMercatorX(double lon)
{
    return lon * kOriginShift / 180.0;
}

double latToMercatorY(double lat)
{
    const double latRad = qDegreesToRadians(lat);
    return std::log(std::tan(M_PI / 4.0 + latRad / 2.0)) * kOriginShift / M_PI;
}

} // namespace

GeePixelEstimate GeePixelBounds::estimate(double xmin, double ymin,
                                          double xmax, double ymax)
{
    GeePixelEstimate result;
    if (xmin >= xmax || ymin >= ymax) {
        result.withinLimit = false;
        return result;
    }

    const double widthM = lonToMercatorX(xmax) - lonToMercatorX(xmin);
    const double heightM = latToMercatorY(ymax) - latToMercatorY(ymin);

    result.pixelWidth = qMax(1, static_cast<int>(std::ceil(widthM / kScaleMeters)));
    result.pixelHeight = qMax(1, static_cast<int>(std::ceil(heightM / kScaleMeters)));
    result.totalPixels = static_cast<qint64>(result.pixelWidth) * result.pixelHeight;
    result.withinLimit = result.totalPixels <= kMaxPixels;
    return result;
}

QString GeePixelBounds::formatEstimate(const GeePixelEstimate &estimate)
{
    if (estimate.pixelWidth <= 0 || estimate.pixelHeight <= 0) {
        return QStringLiteral("Nieprawidłowy obszar");
    }

    const QString limitText = estimate.withinLimit
                                  ? QStringLiteral("OK")
                                  : QStringLiteral("ZA DUŻY");

    return QStringLiteral("%1 × %2 = %3 px  (limit: %4)  [%5]")
        .arg(estimate.pixelWidth)
        .arg(estimate.pixelHeight)
        .arg(estimate.totalPixels)
        .arg(kMaxPixels)
        .arg(limitText);
}

bool GeePixelBounds::isWithinLimit(double xmin, double ymin, double xmax, double ymax,
                                   QString *errorMessage)
{
    const GeePixelEstimate est = estimate(xmin, ymin, xmax, ymax);
    if (est.withinLimit) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
                            "Obszar przekracza limit GEE (%1 px).\n"
                            "Szacowana rozdzielczość: %2 × %3 = %4 px.\n"
                            "Zmniejsz zaznaczony prostokąt na mapie.")
                            .arg(kMaxPixels)
                            .arg(est.pixelWidth)
                            .arg(est.pixelHeight)
                            .arg(est.totalPixels);
    }
    return false;
}
