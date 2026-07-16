#ifndef BANDBUFFER_H
#define BANDBUFFER_H

#include <QVector>
#include <QtGlobal>

/**
 * Bufor jednego pasma satelitarnego (siatka float, wierszami).
 * Wartości typowo w zakresie reflectance Sentinel-2 (0–10000) lub NDVI (-1–1).
 */
class BandBuffer
{
public:
    BandBuffer() = default;
    BandBuffer(int width, int height);

    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isEmpty() const { return m_width <= 0 || m_height <= 0; }
    int pixelCount() const { return m_width * m_height; }

    float value(int x, int y) const;
    void setValue(int x, int y, float value);

    float *data() { return m_data.data(); }
    const float *data() const { return m_data.constData(); }

    /** Uśrednienie dwóch pasm pixel-by-pixel. */
    static BandBuffer average(const BandBuffer &a, const BandBuffer &b);

    /** NDVI = (nir - red) / (nir + red). */
    static BandBuffer ndvi(const BandBuffer &red, const BandBuffer &nir);

private:
    int m_width = 0;
    int m_height = 0;
    QVector<float> m_data;
};

#endif // BANDBUFFER_H
