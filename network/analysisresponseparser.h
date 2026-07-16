#ifndef ANALYSISRESPONSEPARSER_H
#define ANALYSISRESPONSEPARSER_H

#include "models/analysisresult.h"

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

/**
 * Parser odpowiedzi program88.py:
 * { "DANE": { "2018_BEZ": { "B2":[[..]], ... }, "2018_CHM": {...}, ... },
 *   "POWIERZCHNIA": [[m2, ...], ...] }
 */
class AnalysisResponseParser
{
public:
    static AnalysisResult parse(const QByteArray &jsonBytes, QString *errorMessage = nullptr);

private:
    static BandBuffer parseBandMatrix(const QJsonValue &value, QString *errorMessage);
    static bool parseYearEntry(const QJsonObject &bandsObject,
                               YearBands *yearBands,
                               bool clouds,
                               QString *errorMessage);
};

#endif // ANALYSISRESPONSEPARSER_H
