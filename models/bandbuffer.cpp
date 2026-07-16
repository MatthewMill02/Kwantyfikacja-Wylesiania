#include "bandbuffer.h"

#include <QtMath>

BandBuffer::BandBuffer(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_data(width * height, 0.0f)
{
    Q_ASSERT(width > 0 && height > 0);
}

float BandBuffer::value(int x, int y) const
{
    Q_ASSERT(x >= 0 && x < m_width && y >= 0 && y < m_height);
    return m_data[y * m_width + x];
}

void BandBuffer::setValue(int x, int y, float value)
{
    Q_ASSERT(x >= 0 && x < m_width && y >= 0 && y < m_height);
    m_data[y * m_width + x] = value;
}

BandBuffer BandBuffer::average(const BandBuffer &a, const BandBuffer &b)
{
    Q_ASSERT(a.width() == b.width() && a.height() == b.height());
    BandBuffer out(a.width(), a.height());
    const int n = a.pixelCount();
    for (int i = 0; i < n; ++i) {
        out.m_data[i] = 0.5f * (a.m_data[i] + b.m_data[i]);
    }
    return out;
}

BandBuffer BandBuffer::ndvi(const BandBuffer &red, const BandBuffer &nir)
{
    Q_ASSERT(red.width() == nir.width() && red.height() == nir.height());
    BandBuffer out(red.width(), red.height());
    const int n = red.pixelCount();
    for (int i = 0; i < n; ++i) {
        const float r = red.m_data[i];
        const float nVal = nir.m_data[i];
        const float denom = nVal + r;
        out.m_data[i] = (qAbs(denom) < 1e-6f) ? 0.0f : (nVal - r) / denom;
    }
    return out;
}
