#include "analysisresponseparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

BandBuffer AnalysisResponseParser::parseBandMatrix(const QJsonValue &value,
                                                   QString *errorMessage)
{
    if (!value.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Oczekiwano macierzy 2D (tablica wierszy).");
        }
        return {};
    }

    const QJsonArray rows = value.toArray();
    if (rows.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Pusta macierz pasma.");
        }
        return {};
    }

    if (!rows.at(0).isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Pierwszy wiersz macierzy nie jest tablicą.");
        }
        return {};
    }

    const int height = rows.size();
    const int width = rows.at(0).toArray().size();
    if (width <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Szerokość macierzy wynosi 0.");
        }
        return {};
    }

    BandBuffer buffer(width, height);
    for (int y = 0; y < height; ++y) {
        if (!rows.at(y).isArray()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Wiersz %1 nie jest tablicą.").arg(y);
            }
            return {};
        }
        const QJsonArray row = rows.at(y).toArray();
        if (row.size() != width) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Niespójna szerokość wiersza %1.").arg(y);
            }
            return {};
        }
        for (int x = 0; x < width; ++x) {
            const QJsonValue cell = row.at(x);
            // Zamaskowane piksele GEE przychodzą jako null → 0
            const float v = cell.isNull() || cell.isUndefined()
                                ? 0.0f
                                : static_cast<float>(cell.toDouble(0.0));
            buffer.setValue(x, y, v);
        }
    }
    return buffer;
}

bool AnalysisResponseParser::parseYearEntry(const QJsonObject &bandsObject,
                                            YearBands *yearBands,
                                            bool clouds,
                                            QString *errorMessage)
{
    const BandBuffer b2 = parseBandMatrix(bandsObject.value(QStringLiteral("B2")), errorMessage);
    if (b2.isEmpty()) {
        return false;
    }
    const BandBuffer b3 = parseBandMatrix(bandsObject.value(QStringLiteral("B3")), errorMessage);
    if (b3.isEmpty()) {
        return false;
    }
    const BandBuffer b4 = parseBandMatrix(bandsObject.value(QStringLiteral("B4")), errorMessage);
    if (b4.isEmpty()) {
        return false;
    }
    const BandBuffer b8 = parseBandMatrix(bandsObject.value(QStringLiteral("B8")), errorMessage);
    if (b8.isEmpty()) {
        return false;
    }

    if (clouds) {
        yearBands->b2Clouds = b2;
        yearBands->b3Clouds = b3;
        yearBands->b4Clouds = b4;
        yearBands->b8Clouds = b8;
    } else {
        yearBands->b2 = b2;
        yearBands->b3 = b3;
        yearBands->b4 = b4;
        yearBands->b8 = b8;
    }
    return true;
}

AnalysisResult AnalysisResponseParser::parse(const QByteArray &jsonBytes,
                                             QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Niepoprawny JSON: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QJsonObject root = doc.object();
    const QJsonObject dane = root.value(QStringLiteral("DANE")).toObject();
    if (dane.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Brak obiektu DANE w odpowiedzi backendu.");
        }
        return {};
    }

    AnalysisResult result;
    static const QRegularExpression keyRe(QStringLiteral("^(\\d{4})_(BEZ|CHM)$"));

    for (auto it = dane.begin(); it != dane.end(); ++it) {
        const QRegularExpressionMatch match = keyRe.match(it.key());
        if (!match.hasMatch()) {
            continue;
        }
        const int year = match.captured(1).toInt();
        const bool clouds = (match.captured(2) == QLatin1String("CHM"));

        if (!it.value().isObject()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Wpis %1 nie jest obiektem.").arg(it.key());
            }
            return {};
        }

        YearBands &yearBands = result.years[year];
        if (!parseYearEntry(it.value().toObject(), &yearBands, clouds, errorMessage)) {
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("Błąd parsowania %1.").arg(it.key());
            }
            return {};
        }
    }

    if (result.years.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("DANE nie zawierają żadnych lat (*_BEZ / *_CHM).");
        }
        return {};
    }

    // Uzupełnij brakujące warianty chmur kopią wersji bez chmur (na wypadek niepełnej odpowiedzi)
    for (auto it = result.years.begin(); it != result.years.end(); ++it) {
        YearBands &y = it.value();
        if (y.b4Clouds.isEmpty() && !y.b4.isEmpty()) {
            y.b2Clouds = y.b2;
            y.b3Clouds = y.b3;
            y.b4Clouds = y.b4;
            y.b8Clouds = y.b8;
        }
        if (y.b4.isEmpty() && !y.b4Clouds.isEmpty()) {
            y.b2 = y.b2Clouds;
            y.b3 = y.b3Clouds;
            y.b4 = y.b4Clouds;
            y.b8 = y.b8Clouds;
        }
    }

    if (root.contains(QStringLiteral("POWIERZCHNIA"))) {
        result.pixelAreaM2 = parseBandMatrix(root.value(QStringLiteral("POWIERZCHNIA")),
                                             errorMessage);
        if (result.pixelAreaM2.isEmpty()) {
            return {};
        }
    }

    result.statusMessage = QStringLiteral("Dane z backendu (program88).");
    return result;
}
