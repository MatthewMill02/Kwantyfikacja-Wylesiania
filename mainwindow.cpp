#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "utils/geepixelbounds.h"
#include "widgets/osmmapwidget.h"
#include "widgets/resultsviewwidget.h"

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_backend(new BackendClient(this))
{
    ui->setupUi(this);

    m_resultsView = new ResultsViewWidget(ui->pageResults);
    ui->pageResultsLayout->addWidget(m_resultsView);

    setupMapSelector();

    connect(ui->buttonStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->buttonCancel, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(m_resultsView, &ResultsViewWidget::backRequested, this, &MainWindow::onBackToInput);

    connect(m_backend, &BackendClient::progressChanged, this, &MainWindow::onProgress);
    connect(m_backend, &BackendClient::finished, this, &MainWindow::onAnalysisFinished);
    connect(m_backend, &BackendClient::failed, this, &MainWindow::onAnalysisFailed);

    const auto connectSpin = [this](QDoubleSpinBox *spin) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &MainWindow::onCoordinateFieldChanged);
    };
    connectSpin(ui->spinX);
    connectSpin(ui->spinY);
    connectSpin(ui->spinXX);
    connectSpin(ui->spinYY);

    m_backend->setUseMock(false);
    syncMapFromFields();
    updatePixelEstimateLabel();

    showPage(0);
    statusBar()->showMessage(
        QStringLiteral("Gotowy — backend: %1/analiza").arg(m_backend->baseUrl()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMapSelector()
{
    ui->formColumnsLayout->setStretch(1, 1);

    auto *mapLayout = new QVBoxLayout(ui->mapContainer);
    mapLayout->setContentsMargins(0, 0, 0, 0);

    ui->bboxMapLayout->setStretch(1, 1);

    m_mapWidget = new OsmMapWidget(ui->mapContainer);
    mapLayout->addWidget(m_mapWidget, 1);

    connect(m_mapWidget, &OsmMapWidget::bboxChanged,
            this, &MainWindow::onMapBboxChanged);
}

void MainWindow::syncMapFromFields()
{
    if (!m_mapWidget) {
        return;
    }
    m_syncingCoordinates = true;
    m_mapWidget->setBbox(
        ui->spinX->value(), ui->spinY->value(),
        ui->spinXX->value(), ui->spinYY->value());
    m_syncingCoordinates = false;
}

void MainWindow::syncFieldsFromMap(double xmin, double ymin, double xmax, double ymax)
{
    m_syncingCoordinates = true;
    ui->spinX->setValue(xmin);
    ui->spinY->setValue(ymin);
    ui->spinXX->setValue(xmax);
    ui->spinYY->setValue(ymax);
    m_syncingCoordinates = false;
    updatePixelEstimateLabel();
}

void MainWindow::onMapBboxChanged(double xmin, double ymin, double xmax, double ymax)
{
    if (m_syncingCoordinates) {
        return;
    }
    syncFieldsFromMap(xmin, ymin, xmax, ymax);
    ui->labelValidation->clear();
}

void MainWindow::onCoordinateFieldChanged()
{
    if (m_syncingCoordinates) {
        return;
    }
    syncMapFromFields();
    updatePixelEstimateLabel();
    ui->labelValidation->clear();
}

void MainWindow::updatePixelEstimateLabel()
{
    QLabel *label = ui->pageInput->findChild<QLabel *>(QStringLiteral("labelPixelEstimate"));
    if (!label) {
        return;
    }

    const GeePixelEstimate est = GeePixelBounds::estimate(
        ui->spinX->value(), ui->spinY->value(),
        ui->spinXX->value(), ui->spinYY->value());

    label->setText(GeePixelBounds::formatEstimate(est));
    label->setStyleSheet(est.withinLimit
                             ? QStringLiteral("color: #94a3b8;")
                             : QStringLiteral("color: #f87171; font-weight: 600;"));
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
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->labelLoadingStage->setText(QStringLiteral("Łączenie z backendem…"));
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
