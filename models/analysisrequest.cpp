#include "analysisrequest.h"

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
    // Backend pobiera lata [POCZATEK, KONIEC) — do maski ΔNDVI potrzeba ≥ 2 lat danych
    if (endYear - startYear < 2) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Zakres musi obejmować co najmniej 2 lata danych "
                "(backend pobiera lata od POCZATEK do KONIEC−1). "
                "Np. 2018–2020 daje lata 2018 i 2019.");
        }
        return false;
    }
    if (startYear < 2015 || endYear > 2030) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Lata muszą mieścić się w zakresie 2015–2030.");
        }
        return false;
    }
    if (x >= xx || y <= yy) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Błędny bbox: oczekiwany lewy górny (X,Y) i prawy dolny (XX,YY), "
                "gdzie X < XX oraz Y > YY (szerokość geograficzna maleje na południe).");
        }
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
