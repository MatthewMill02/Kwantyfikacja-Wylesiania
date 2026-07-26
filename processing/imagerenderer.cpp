#include "imagerenderer.h"

#include <QtMath>

int ImageRenderer::normalizeReflectance(float value, float maxValue)
{
    if (!qIsFinite(value)) {
        return 0;
    }
    const float clamped = qBound(0.0f, value, maxValue);
    return static_cast<int>((clamped / maxValue) * 255.0f + 0.5f);
}

QImage ImageRenderer::makeRgbImage(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::black);
    return image;
}

void ImageRenderer::setPixel(QImage &image, int x, int y, int r, int g, int b)
{
    image.setPixel(x, y, qRgb(r, g, b));
}

QImage ImageRenderer::trueColor(const BandBuffer &b2,
                                const BandBuffer &b3,
                                const BandBuffer &b4)
{
    Q_ASSERT(b2.width() == b3.width() && b3.width() == b4.width());
    Q_ASSERT(b2.height() == b3.height() && b3.height() == b4.height());

    QImage image = makeRgbImage(b4.width(), b4.height());
    for (int y = 0; y < b4.height(); ++y) {
        for (int x = 0; x < b4.width(); ++x) {
            const int r = normalizeReflectance(b4.value(x, y));
            const int g = normalizeReflectance(b3.value(x, y));
            const int b = normalizeReflectance(b2.value(x, y));
            setPixel(image, x, y, r, g, b);
        }
    }
    return image;
}

QImage ImageRenderer::falseColor(const BandBuffer &b3,
                                 const BandBuffer &b4,
                                 const BandBuffer &b8)
{
    Q_ASSERT(b3.width() == b4.width() && b4.width() == b8.width());
    QImage image = makeRgbImage(b4.width(), b4.height());
    for (int y = 0; y < b4.height(); ++y) {
        for (int x = 0; x < b4.width(); ++x) {
            const int r = normalizeReflectance(b8.value(x, y));
            const int g = normalizeReflectance(b4.value(x, y));
            const int b = normalizeReflectance(b3.value(x, y));
            setPixel(image, x, y, r, g, b);
        }
    }
    return image;
}

QImage ImageRenderer::customVegetation(const BandBuffer &ndvi,
                                       double sparse,
                                       double moderate,
                                       double dense)
{
    QImage image = makeRgbImage(ndvi.width(), ndvi.height());
    for (int y = 0; y < ndvi.height(); ++y) {
        for (int x = 0; x < ndvi.width(); ++x) {
            const float v = ndvi.value(x, y);
            int r = 40;
            int g = 40;
            int b = 40;

            if (v >= dense) {
                // Gęsta roślinność — głęboka zieleń
                r = 20;
                g = 140;
                b = 40;
            } else if (v >= moderate) {
                r = 60;
                g = 170;
                b = 70;
            } else if (v >= sparse) {
                r = 140;
                g = 190;
                b = 90;
            } else {
                // Brak / słaba roślinność
                r = 160;
                g = 140;
                b = 100;
            }
            setPixel(image, x, y, r, g, b);
        }
    }
    return image;
}

QImage ImageRenderer::deforestationMask(const BandBuffer &ndviT1,
                                        const BandBuffer &ndviT2,
                                        double partial,
                                        double strong)
{
    Q_ASSERT(ndviT1.width() == ndviT2.width());
    QImage image = makeRgbImage(ndviT1.width(), ndviT1.height());

    for (int y = 0; y < ndviT1.height(); ++y) {
        for (int x = 0; x < ndviT1.width(); ++x) {
            const float delta = ndviT2.value(x, y) - ndviT1.value(x, y);
            // Spadek NDVI → ujemne ΔNDVI; klasyfikujemy po |spadku|
            const float drop = -delta;

            int r = 30;
            int g = 30;
            int b = 30;

            if (drop > strong) {
                r = 200;
                g = 30;
                b = 30;
            } else if (drop > partial) {
                r = 220;
                g = 120;
                b = 30;
            } else {
                // Bez istotnego spadku — półprzezroczysty ciemny tło
                r = 45;
                g = 55;
                b = 45;
            }
            setPixel(image, x, y, r, g, b);
        }
    }
    return image;
}

QImage ImageRenderer::grayscaleBand(const BandBuffer &band)
{
    QImage image = makeRgbImage(band.width(), band.height());
    for (int y = 0; y < band.height(); ++y) {
        for (int x = 0; x < band.width(); ++x) {
            const int g = normalizeReflectance(band.value(x, y));
            setPixel(image, x, y, g, g, g);
        }
    }
    return image;
}

QImage ImageRenderer::normalizedDifference(const BandBuffer &first, const BandBuffer &second)
{
    Q_ASSERT(first.width() == second.width() && first.height() == second.height());
    QImage image = makeRgbImage(first.width(), first.height());

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const float a = first.value(x, y);
            const float b = second.value(x, y);
            const float denom = a + b;
            const float nd = (qAbs(denom) < 1e-6f) ? 0.0f : (a - b) / denom;
            const int g = qBound(0, static_cast<int>((nd + 1.0f) * 127.5f + 0.5f), 255);
            setPixel(image, x, y, g, g, g);
        }
    }
    return image;
}

double ImageRenderer::hectaresFromMask(const BandBuffer &ndviT1,
                                       const BandBuffer &ndviT2,
                                       double partialThreshold,
                                       const BandBuffer *pixelAreaM2,
                                       double metersPerPixel)
{
    Q_ASSERT(ndviT1.width() == ndviT2.width() && ndviT1.height() == ndviT2.height());

    const bool useArea = pixelAreaM2
                         && !pixelAreaM2->isEmpty()
                         && pixelAreaM2->width() == ndviT1.width()
                         && pixelAreaM2->height() == ndviT1.height();

    double areaM2 = 0.0;
    const double fallbackPixelM2 = metersPerPixel * metersPerPixel;

    for (int y = 0; y < ndviT1.height(); ++y) {
        for (int x = 0; x < ndviT1.width(); ++x) {
            const float drop = -(ndviT2.value(x, y) - ndviT1.value(x, y));
            if (drop > partialThreshold) {
                areaM2 += useArea ? static_cast<double>(pixelAreaM2->value(x, y))
                                  : fallbackPixelM2;
            }
        }
    }
    return areaM2 / 10000.0;
}
