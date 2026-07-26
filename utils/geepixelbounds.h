#ifndef GEEPIXELBOUNDS_H
#define GEEPIXELBOUNDS_H

#include <QtGlobal>

#include <QString>

/**
 * Szacowanie liczby pikseli jak w program88.py:
 * reproject(EPSG:3857, scale=10) + sampleRectangle.
 * Limit GEE: 262 000 px (łącznie).
 */
struct GeePixelEstimate
{
    int pixelWidth = 0;
    int pixelHeight = 0;
    qint64 totalPixels = 0;
    bool withinLimit = true;
};

namespace GeePixelBounds {

static constexpr int kScaleMeters = 10;
static constexpr qint64 kMaxPixels = 262000;

GeePixelEstimate estimate(double xmin, double ymin, double xmax, double ymax);

/** Tekst informacyjny do UI (np. „123 × 456 = 56 088 px”). */
QString formatEstimate(const GeePixelEstimate &estimate);

/** Walidacja limitu GEE — uwzględnia zależność szerokości stopnia od szerokości geogr. */
bool isWithinLimit(double xmin, double ymin, double xmax, double ymax,
                   QString *errorMessage = nullptr);

} // namespace GeePixelBounds

#endif // GEEPIXELBOUNDS_H
