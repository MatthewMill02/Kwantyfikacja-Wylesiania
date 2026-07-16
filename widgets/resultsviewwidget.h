#ifndef RESULTSVIEWWIDGET_H
#define RESULTSVIEWWIDGET_H

#include "models/analysisrequest.h"
#include "models/analysisresult.h"

#include <QWidget>

class ImagePanelWidget;
class QCheckBox;
class QComboBox;
class QLabel;
class QSlider;

/**
 * Widok wyników: siatka kompozycji + suwaki progów + przełącznik chmur.
 * Przerysowuje lokalnie (C++) po zmianie suwaków / toggle.
 */
class ResultsViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultsViewWidget(QWidget *parent = nullptr);

    void setResult(const AnalysisResult &result, const AnalysisRequest &request);
    void clear();

signals:
    void thresholdsChanged(const AnalysisRequest &request);
    void backRequested();

private slots:
    void onThresholdUiChanged();
    void onCloudsToggled(bool checked);
    void onYearIndexChanged(int index);

private:
    void buildUi();
    void syncSlidersFromRequest();
    void readThresholdsToRequest();
    void refillYearCombo();
    void rerender();

    const YearBands *activeYear() const;

    AnalysisResult m_result;
    AnalysisRequest m_request;
    bool m_showClouds = false;
    int m_selectedYear = 0;

    ImagePanelWidget *m_trueColor = nullptr;
    ImagePanelWidget *m_falseColor = nullptr;
    ImagePanelWidget *m_customVegetation = nullptr;
    ImagePanelWidget *m_deforestationMask = nullptr;
    ImagePanelWidget *m_bandB4 = nullptr;
    ImagePanelWidget *m_bandB8 = nullptr;
    ImagePanelWidget *m_bandAverage = nullptr;

    QComboBox *m_yearCombo = nullptr;
    QSlider *m_sparseSlider = nullptr;
    QSlider *m_moderateSlider = nullptr;
    QSlider *m_denseSlider = nullptr;
    QSlider *m_partialSlider = nullptr;
    QSlider *m_strongSlider = nullptr;

    QLabel *m_sparseValue = nullptr;
    QLabel *m_moderateValue = nullptr;
    QLabel *m_denseValue = nullptr;
    QLabel *m_partialValue = nullptr;
    QLabel *m_strongValue = nullptr;
    QLabel *m_hectaresLabel = nullptr;

    QCheckBox *m_cloudsToggle = nullptr;
};

#endif // RESULTSVIEWWIDGET_H
