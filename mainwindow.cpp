#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "widgets/resultsviewwidget.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_backend(new BackendClient(this))
{
    ui->setupUi(this);

    m_resultsView = new ResultsViewWidget(ui->pageResults);
    ui->pageResultsLayout->addWidget(m_resultsView);

    connect(ui->buttonStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->buttonCancel, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(m_resultsView, &ResultsViewWidget::backRequested, this, &MainWindow::onBackToInput);

    connect(m_backend, &BackendClient::progressChanged, this, &MainWindow::onProgress);
    connect(m_backend, &BackendClient::finished, this, &MainWindow::onAnalysisFinished);
    connect(m_backend, &BackendClient::failed, this, &MainWindow::onAnalysisFailed);

    m_backend->setUseMock(false);

    showPage(0);
    statusBar()->showMessage(
        QStringLiteral("Gotowy — backend: %1/analiza").arg(m_backend->baseUrl()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

AnalysisRequest MainWindow::collectRequestFromForm() const
{
    AnalysisRequest request;
    request.startYear = ui->spinStartYear->value();
    request.endYear = ui->spinEndYear->value();
    request.x = ui->spinX->value();
    request.y = ui->spinY->value();
    request.xx = ui->spinXX->value();
    request.yy = ui->spinYY->value();
    return request;
}

void MainWindow::showPage(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::onStartClicked()
{
    ui->labelValidation->clear();

    AnalysisRequest request = collectRequestFromForm();
    QString error;
    if (!request.isValid(&error)) {
        ui->labelValidation->setText(error);
        return;
    }

    m_lastRequest = request;
    // Brak progresu z backendu — pasek nieokreślony do czasu odpowiedzi
    ui->progressBar->setRange(0, 0);
    ui->labelLoadingStage->setText(
        QStringLiteral("Oczekiwanie na odpowiedź backendu (GEE)…"));
    showPage(1);
    statusBar()->showMessage(QStringLiteral("Analiza w toku…"));
    m_backend->startAnalysis(request);
}

void MainWindow::onCancelClicked()
{
    m_backend->cancel();
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    showPage(0);
    statusBar()->showMessage(QStringLiteral("Analiza anulowana"));
}

void MainWindow::onProgress(int percent, const QString &stage)
{
    ui->labelLoadingStage->setText(stage);
    if (percent < 0) {
        ui->progressBar->setRange(0, 0);
        return;
    }
    if (ui->progressBar->maximum() == 0) {
        ui->progressBar->setRange(0, 100);
    }
    ui->progressBar->setValue(percent);
}

void MainWindow::onAnalysisFinished(const AnalysisResult &result)
{
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(100);
    m_resultsView->setResult(result, m_lastRequest);
    showPage(2);
    statusBar()->showMessage(
        QStringLiteral("Analiza zakończona — %1 ha (%2→%3)")
            .arg(result.hectaresDeforested, 0, 'f', 2)
            .arg(result.firstYear())
            .arg(result.lastYear()));
}

void MainWindow::onAnalysisFailed(const QString &errorMessage)
{
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    showPage(0);
    QMessageBox::warning(this, QStringLiteral("Błąd analizy"), errorMessage);
    statusBar()->showMessage(QStringLiteral("Błąd analizy"));
}

void MainWindow::onBackToInput()
{
    m_resultsView->clear();
    showPage(0);
    statusBar()->showMessage(
        QStringLiteral("Gotowy — backend: %1/analiza").arg(m_backend->baseUrl()));
}
