#include "mockdatagenerator.h"
#include "imagerenderer.h"

#include <QtMath>

namespace {

float hashNoise(int x, int y, int seed)
{
    const int n = x * 374761 + y * 668265 + seed * 1274129;
    const int mixed = (n ^ (n >> 13)) * 1274126177;
    return (mixed & 0xFFFF) / 65535.0f;
}

BandBuffer makeBand(int width, int height, float base, float amplitude, int seed)
{
    BandBuffer band(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float n = hashNoise(x, y, seed);
            band.setValue(x, y, base + amplitude * n);
        }
    }
    return band;
}

void applyForestPattern(BandBuffer &b2, BandBuffer &b3, BandBuffer &b4, BandBuffer &b8,
                        bool clearedCenter)
{
    const int w = b4.width();
    const int h = b4.height();
    const int cx = w / 2;
    const int cy = h / 2;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float dx = (x - cx) / static_cast<float>(w);
            const float dy = (y - cy) / static_cast<float>(h);
            const float dist = qSqrt(dx * dx + dy * dy);

            const bool cleared = clearedCenter && dist < 0.22f;
            const bool partial = clearedCenter && dist >= 0.22f && dist < 0.32f;

            float red;
            float nir;
            float green;
            float blue;

            if (cleared) {
                red = 900.0f + 200.0f * hashNoise(x, y, 11);
                nir = 700.0f + 150.0f * hashNoise(x, y, 12);
                green = 800.0f + 100.0f * hashNoise(x, y, 13);
                blue = 600.0f + 80.0f * hashNoise(x, y, 14);
            } else if (partial) {
                red = 700.0f + 150.0f * hashNoise(x, y, 21);
                nir = 1600.0f + 200.0f * hashNoise(x, y, 22);
                green = 900.0f + 120.0f * hashNoise(x, y, 23);
                blue = 500.0f + 80.0f * hashNoise(x, y, 24);
            } else {
                red = 350.0f + 120.0f * hashNoise(x, y, 31);
                nir = 2800.0f + 400.0f * hashNoise(x, y, 32);
                green = 700.0f + 150.0f * hashNoise(x, y, 33);
                blue = 400.0f + 100.0f * hashNoise(x, y, 34);
            }

            b2.setValue(x, y, blue);
            b3.setValue(x, y, green);
            b4.setValue(x, y, red);
            b8.setValue(x, y, nir);
        }
    }
}

void addClouds(BandBuffer &b2, BandBuffer &b3, BandBuffer &b4, BandBuffer &b8)
{
    for (int y = 0; y < b4.height(); ++y) {
        for (int x = 0; x < b4.width(); ++x) {
            const float cloud = hashNoise(x / 8, y / 8, 99);
            if (cloud > 0.72f) {
                const float bright = 2800.0f + 200.0f * cloud;
                b2.setValue(x, y, bright);
                b3.setValue(x, y, bright);
                b4.setValue(x, y, bright);
                b8.setValue(x, y, bright * 0.95f);
            }
        }
    }
}

YearBands makeYear(int width, int height, bool clearedCenter)
{
    YearBands year;
    year.b2 = makeBand(width, height, 400.0f, 80.0f, 1);
    year.b3 = makeBand(width, height, 700.0f, 100.0f, 2);
    year.b4 = makeBand(width, height, 400.0f, 80.0f, 3);
    year.b8 = makeBand(width, height, 2500.0f, 300.0f, 4);
    applyForestPattern(year.b2, year.b3, year.b4, year.b8, clearedCenter);

    year.b2Clouds = year.b2;
    year.b3Clouds = year.b3;
    year.b4Clouds = year.b4;
    year.b8Clouds = year.b8;
    addClouds(year.b2Clouds, year.b3Clouds, year.b4Clouds, year.b8Clouds);
    return year;
}

} // namespace

AnalysisResult MockDataGenerator::generate(const AnalysisRequest &request,
                                           int width,
                                           int height)
{
    AnalysisResult result;

    // Jak program9: lata [POCZATEK, KONIEC] włącznie
    for (int year = request.startYear; year <= request.endYear; ++year) {
        const bool cleared = (year == request.endYear);
        result.years.insert(year, makeYear(width, height, cleared));
    }

    result.pixelAreaM2 = BandBuffer(width, height);
    const float cellM2 = 100.0f; // 10 m × 10 m
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            result.pixelAreaM2.setValue(x, y, cellM2);
        }
    }

    if (result.years.size() >= 2) {
        const YearBands *t1 = result.bandsFor(result.firstYear());
        const YearBands *t2 = result.bandsFor(result.lastYear());
        const BandBuffer ndvi1 = BandBuffer::ndvi(t1->b4, t1->b8);
        const BandBuffer ndvi2 = BandBuffer::ndvi(t2->b4, t2->b8);
        result.hectaresDeforested = ImageRenderer::hectaresFromMask(
            ndvi1, ndvi2, request.deforestationPartial, &result.pixelAreaM2);
    }

    result.statusMessage = QStringLiteral("Dane mock (lokalny generator).");
    return result;
}
