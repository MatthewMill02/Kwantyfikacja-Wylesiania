#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "models/analysisrequest.h"
#include "network/backendclient.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class OsmMapWidget;
class ResultsViewWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartClicked();
    void onCancelClicked();
    void onProgress(int percent, const QString &stage);
    void onAnalysisFinished(const AnalysisResult &result);
    void onAnalysisFailed(const QString &errorMessage);
    void onBackToInput();

    void onMapBboxChanged(double xmin, double ymin, double xmax, double ymax);
    void onCoordinateFieldChanged();
    void updatePixelEstimateLabel();

private:
    void setupMapSelector();
    void syncMapFromFields();
    void syncFieldsFromMap(double xmin, double ymin, double xmax, double ymax);
    AnalysisRequest collectRequestFromForm() const;
    void showPage(int index);

    Ui::MainWindow *ui = nullptr;
    BackendClient *m_backend = nullptr;
    ResultsViewWidget *m_resultsView = nullptr;
    OsmMapWidget *m_mapWidget = nullptr;

    bool m_syncingCoordinates = false;
    AnalysisRequest m_lastRequest;
};

#endif // MAINWINDOW_H
