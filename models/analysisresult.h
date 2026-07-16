#ifndef ANALYSISRESULT_H
#define ANALYSISRESULT_H

#include "bandbuffer.h"

#include <QMap>
#include <QString>
#include <QVector>

/**
 * Pasma jednego roku: BEZ chmur + CHM (z chmurami).
 * Źródło: backend program88 — klucze "{rok}_BEZ" / "{rok}_CHM".
 */
struct YearBands
{
    BandBuffer b2;
    BandBuffer b3;
    BandBuffer b4;
    BandBuffer b8;

    BandBuffer b2Clouds;
    BandBuffer b3Clouds;
    BandBuffer b4Clouds;
    BandBuffer b8Clouds;

    bool isValid() const { return !b4.isEmpty() && !b8.isEmpty(); }
};

/**
 * Pełna odpowiedź /analiza:
 *  - DANE: lata [POCZATEK .. KONIEC) — backend nie pobiera roku KONIEC
 *  - POWIERZCHNIA: m² na piksel (EPSG:3857, scale 10)
 */
struct AnalysisResult
{
    QMap<int, YearBands> years;
    BandBuffer pixelAreaM2;
    double hectaresDeforested = 0.0;
    QString statusMessage;

    QVector<int> availableYears() const;
    int firstYear() const;
    int lastYear() const;
    const YearBands *bandsFor(int year) const;

    bool isValid() const;
};

#endif // ANALYSISRESULT_H
