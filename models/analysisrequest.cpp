#include "analysisrequest.h"

#include "utils/geepixelbounds.h"

QJsonObject AnalysisRequest::toJson() const
{
    // Kontrakt program88.py — wyłącznie te pola trafiają do POST /analiza
    QJsonObject json;
    json.insert(QStringLiteral("POCZATEK"), startYear);
    json.insert(QStringLiteral("KONIEC"), endYear);
    json.insert(QStringLiteral("X"), x);
    json.insert(QStringLiteral("Y"), y);
    json.insert(QStringLiteral("XX"), xx);
    json.insert(QStringLiteral("YY"), yy);
    return json;
}

bool AnalysisRequest::isValid(QString *errorMessage) const
{
    if (startYear >= endYear) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Rok początkowy musi być mniejszy od roku końcowego.");
        }
        return false;
    }
    // backend: lata [POCZATEK, KONIEC] włącznie — min. 2 lata
    if (endYear - startYear < 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Zakres musi obejmować co najmniej 2 lata "
                "(backend pobiera lata od POCZATEK do KONIEC włącznie). "
                "Np. 2018–2019.");
        }
        return false;
    }
    if (startYear < 2015 || endYear > 2030) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Lata muszą mieścić się w zakresie 2015–2030.");
        }
        return false;
    }
    // ee.Geometry.Rectangle([X, Y, XX, YY]) = [xmin, ymin, xmax, ymax]
    if (x >= xx || y >= yy) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Błędny bbox: oczekiwane X < XX oraz Y < YY "
                "(Rectangle: xmin, ymin, xmax, ymax).");
        }
        return false;
    }
    if (!GeePixelBounds::isWithinLimit(x, y, xx, yy, errorMessage)) {
        return false;
    }
    if (!(vegetationSparse < vegetationModerate
          && vegetationModerate < vegetationDense)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Progi roślinności muszą rosnąć: rzadka < umiarkowana < gęsta.");
        }
        return false;
    }
    if (!(deforestationPartial < deforestationStrong)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Próg wylesienia częściowego musi być mniejszy od silnego.");
        }
        return false;
    }
    return true;
}
