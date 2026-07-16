#ifndef IMAGERENDERER_H
#define IMAGERENDERER_H

#include "models/bandbuffer.h"

#include <QImage>
#include <QRgb>

/**
 * Renderowanie kompozycji satelitarnych pixel-by-pixel (C++).
 * Wszystkie metody są czystymi funkcjami bez stanu UI.
 */
class ImageRenderer
{
public:
    /** Normalizacja reflectance (0–3000 → 0–255), jak w backendzie GEE. */
    static int normalizeReflectance(float value, float maxValue = 3000.0f);

    /** True Color: R=B4, G=B3, B=B2 */
    static QImage trueColor(const BandBuffer &b2,
                            const BandBuffer &b3,
                            const BandBuffer &b4);

    /** False Color klasyczny (wegetacja): R=B8, G=B4, B=B3 */
    static QImage falseColor(const BandBuffer &b3,
                             const BandBuffer &b4,
                             const BandBuffer &b8);

    /**
     * Własny false color: odcienie zieleni wg progów NDVI.
     * sparse / moderate / dense ∈ [0, 1]
     */
    static QImage customVegetation(const BandBuffer &ndvi,
                                   double sparse,
                                   double moderate,
                                   double dense);

    /**
     * Maska wylesienia na podstawie ΔNDVI = NDVI_T2 − NDVI_T1.
     * Spadek > strong → silne (czerwony), > partial → częściowe (pomarańcz).
     */
    static QImage deforestationMask(const BandBuffer &ndviT1,
                                    const BandBuffer &ndviT2,
                                    double partial,
                                    double strong);

    /** Skala szarości pojedynczego pasma. */
    static QImage grayscaleBand(const BandBuffer &band);

    /**
     * Powierzchnia wylesienia w hektarach.
     * Jeśli podano pixelAreaM2 (z POWIERZCHNIA backendu) — sumuje m² zaznaczonych pikseli.
     * W przeciwnym razie zakłada stały rozmiar metersPerPixel × metersPerPixel.
     */
    static double hectaresFromMask(const BandBuffer &ndviT1,
                                   const BandBuffer &ndviT2,
                                   double partialThreshold,
                                   const BandBuffer *pixelAreaM2 = nullptr,
                                   double metersPerPixel = 10.0);

private:
    static QImage makeRgbImage(int width, int height);
    static void setPixel(QImage &image, int x, int y, int r, int g, int b);
};

#endif // IMAGERENDERER_H
