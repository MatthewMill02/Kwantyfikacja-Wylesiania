#ifndef MOCKDATAGENERATOR_H
#define MOCKDATAGENERATOR_H

#include "models/analysisrequest.h"
#include "models/analysisresult.h"

/**
 * Syntetyczne pasma Sentinel-2 do lokalnego testu UI bez GEE.
 * Generuje realistyczny wzorzec lasu + „wylesienie” między latami.
 */
class MockDataGenerator
{
public:
    static AnalysisResult generate(const AnalysisRequest &request,
                                   int width = 256,
                                   int height = 256);
};

#endif // MOCKDATAGENERATOR_H
