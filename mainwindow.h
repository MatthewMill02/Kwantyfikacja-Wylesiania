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

private:
    AnalysisRequest collectRequestFromForm() const;
    void showPage(int index);

    Ui::MainWindow *ui = nullptr;
    BackendClient *m_backend = nullptr;
    ResultsViewWidget *m_resultsView = nullptr;
    AnalysisRequest m_lastRequest;
};

#endif // MAINWINDOW_H
