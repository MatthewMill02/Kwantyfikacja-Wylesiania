#ifndef RESULTSVIEWWIDGET_H
#define RESULTSVIEWWIDGET_H

#include "models/analysisrequest.h"
#include "models/analysisresult.h"

#include <QWidget>

class QResizeEvent;
class QShowEvent;
class ImagePanelWidget;
class QBarSet;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QChartView;
class QGroupBox;
class QStackedWidget;
class QTabBar;
class SplitPanelWidget;

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
    void onDeforestationToggled(bool checked);
    void onSplitModeChanged(int index);
    void onNewAnalysisClicked();
    void onChartBarClicked(int index, QBarSet *barSet);
    void onTabChanged(int index);
    void placeDisplayOptions(int tabIndex);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    void syncSlidersFromRequest();
    void readThresholdsToRequest();
    void refillYearCombo();
    void rerender();
    void updateSplitPanel();
    void updateChart();
    void updateTopRowColumnWidths();

    const YearBands *activeYear() const;
    QImage compositionImage(int year, bool trueColor) const;

    AnalysisResult m_result;
    AnalysisRequest m_request;
    bool m_showClouds = false;
    bool m_showDeforestation = false;
    int m_selectedYear = 0;
    QStringList m_chartCategories;

    ImagePanelWidget *m_trueColor = nullptr;
    ImagePanelWidget *m_falseColor = nullptr;
    ImagePanelWidget *m_customVegetation = nullptr;
    ImagePanelWidget *m_deforestationMask = nullptr;
    ImagePanelWidget *m_bandB4 = nullptr;
    ImagePanelWidget *m_bandB8 = nullptr;
    ImagePanelWidget *m_bandAverage = nullptr;

    SplitPanelWidget *m_splitPanel = nullptr;

    QComboBox *m_yearCombo = nullptr;
    QGroupBox *m_yearBox = nullptr;
    QGroupBox *m_displayGroup = nullptr;
    QWidget *m_compositionsDisplaySlot = nullptr;
    QWidget *m_bandsDisplaySlot = nullptr;
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
    QLabel *m_chartDetailLabel = nullptr;

    QTabBar *m_tabBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_compositionsPage = nullptr;
    QWidget *m_topRowHost = nullptr;
    QWidget *m_trueColorColumn = nullptr;
    QWidget *m_splitColumn = nullptr;
    QChartView *m_chartView = nullptr;
    QCheckBox *m_cloudsToggle = nullptr;
    QCheckBox *m_deforestationToggle = nullptr;
    QPushButton *m_newAnalysisButton = nullptr;
};

#endif // RESULTSVIEWWIDGET_H
