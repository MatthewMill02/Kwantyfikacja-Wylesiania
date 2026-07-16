#include "analysisresult.h"

QVector<int> AnalysisResult::availableYears() const
{
    return years.keys().toVector();
}

int AnalysisResult::firstYear() const
{
    if (years.isEmpty()) {
        return 0;
    }
    return years.firstKey();
}

int AnalysisResult::lastYear() const
{
    if (years.isEmpty()) {
        return 0;
    }
    return years.lastKey();
}

const YearBands *AnalysisResult::bandsFor(int year) const
{
    const auto it = years.constFind(year);
    if (it == years.cend()) {
        return nullptr;
    }
    return &it.value();
}

bool AnalysisResult::isValid() const
{
    if (years.isEmpty()) {
        return false;
    }
    for (auto it = years.cbegin(); it != years.cend(); ++it) {
        if (!it.value().isValid()) {
            return false;
        }
    }
    return true;
}
