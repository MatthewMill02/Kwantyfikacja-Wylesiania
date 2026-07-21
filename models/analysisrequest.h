#ifndef ANALYSISREQUEST_H
#define ANALYSISREQUEST_H

#include <QJsonObject>

/**
 * Parametry analizy.
 * doJson() wysyła tylko pola kontraktu program88 (POCZATEK/KONIEC/X/Y/XX/YY).
 * Bbox jak ee.Geometry.Rectangle: X < XX, Y < YY (xmin, ymin, xmax, ymax).
 * Progi roślinności / wylesienia zostają po stronie frontendu (lokalny render).
 */
struct AnalysisRequest
{
    int startYear = 2018;
    int endYear = 2023;

    double x = -55.86;   // xmin
    double y = -7.58;    // ymin
    double xx = -55.81;  // xmax
    double yy = -7.555;  // ymax

    // Tylko frontend — nie idą do backendu
    double vegetationSparse = 0.2;
    double vegetationModerate = 0.4;
    double vegetationDense = 0.6;
    double deforestationPartial = 0.25;
    double deforestationStrong = 0.5;

    QJsonObject toJson() const;
    bool isValid(QString *errorMessage = nullptr) const;
};

#endif // ANALYSISREQUEST_H
