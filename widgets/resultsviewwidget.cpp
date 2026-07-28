#include "resultsviewwidget.h"
#include "imagepanelwidget.h"
#include "splitpanelwidget.h"
#include "theme/apptheme.h"

#include "models/bandbuffer.h"
#include "processing/imagerenderer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>

#include <QPainter>
#include <QColor>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

namespace {

QSlider *makePercentSlider(int min, int max, int value, QWidget *parent)
{
    auto *slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(min, max);
    slider->setValue(value);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(qMax(1, (max - min) / 5));
    return slider;
}

QString format01(int sliderValue)
{
    return QString::number(sliderValue / 100.0, 'f', 2);
}

void addSliderRow(QFormLayout *form, const QString &name, QSlider *slider, QLabel *valueLabel)
{
    auto *row = new QWidget();
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(slider, 1);
    rowLayout->addWidget(valueLabel);
    form->addRow(name, row);
}

QGroupBox *makeThresholdGroup(const QString &title, QWidget *parent)
{
    auto *box = new QGroupBox(title, parent);
    box->setObjectName(QStringLiteral("thresholdGroup"));
    return box;
}

} // namespace

ResultsViewWidget::ResultsViewWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void ResultsViewWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(0);

    auto *subHeaderRow = new QHBoxLayout();
    subHeaderRow->setContentsMargins(0, 0, 0, 8);
    subHeaderRow->setSpacing(12);

    m_hectaresLabel = new QLabel(QStringLiteral("Utrata lasu: — ha"), this);
    m_hectaresLabel->setObjectName(QStringLiteral("labelHectares"));
    m_hectaresLabel->setWordWrap(true);

    m_yearBox = new QGroupBox(QStringLiteral("Rok podglądu"), this);
    m_yearBox->setObjectName(QStringLiteral("yearPreviewGroup"));
    m_yearBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    auto *yearLayout = new QVBoxLayout(m_yearBox);
    yearLayout->setContentsMargins(8, 4, 8, 6);
    yearLayout->setSpacing(4);
    m_yearCombo = new QComboBox(m_yearBox);
    m_yearCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(m_yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResultsViewWidget::onYearIndexChanged);
    yearLayout->addWidget(m_yearCombo);

    m_newAnalysisButton = new QPushButton(QStringLiteral("Nowa analiza"), this);
    m_newAnalysisButton->setMinimumHeight(36);
    connect(m_newAnalysisButton, &QPushButton::clicked,
            this, &ResultsViewWidget::onNewAnalysisClicked);

    subHeaderRow->addWidget(m_hectaresLabel, 1);
    subHeaderRow->addWidget(m_yearBox, 0, Qt::AlignTop);
    subHeaderRow->addWidget(m_newAnalysisButton, 0, Qt::AlignTop);
    root->addLayout(subHeaderRow);

    auto *tabRow = new QHBoxLayout();
    tabRow->setContentsMargins(0, 0, 0, 0);
    m_tabBar = new QTabBar(this);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->addTab(QStringLiteral("Kompozycje"));
    m_tabBar->addTab(QStringLiteral("Pasma"));
    m_tabBar->addTab(QStringLiteral("Statystyki"));
    tabRow->addWidget(m_tabBar);
    tabRow->addStretch();
    root->addLayout(tabRow);

    auto *separator = new QFrame(this);
    separator->setObjectName(QStringLiteral("resultsHeaderSeparator"));
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setFixedHeight(1);
    root->addWidget(separator);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("resultsStack"));

    m_displayGroup = new QGroupBox(QStringLiteral("Opcje wyświetlania"));
    m_displayGroup->setObjectName(QStringLiteral("displayOptionsGroup"));
    m_displayGroup->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto *displayLayout = new QVBoxLayout(m_displayGroup);
    displayLayout->setContentsMargins(10, 10, 10, 10);
    displayLayout->setSpacing(6);

    m_cloudsToggle = new QCheckBox(QStringLiteral("Wersje z chmurami"), m_displayGroup);
    m_cloudsToggle->setObjectName(QStringLiteral("displayOptionToggle"));
    m_cloudsToggle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    m_cloudsToggle->setToolTip(QStringLiteral(
        "Przełącza grafiki (oprócz maski) na wersje *_CHM."));
    connect(m_cloudsToggle, &QCheckBox::toggled, this, &ResultsViewWidget::onCloudsToggled);

    m_deforestationToggle = new QCheckBox(QStringLiteral("Nałóż maskę wylesień"), m_displayGroup);
    m_deforestationToggle->setObjectName(QStringLiteral("displayOptionToggle"));
    m_deforestationToggle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(m_deforestationToggle, &QCheckBox::toggled,
            this, &ResultsViewWidget::onDeforestationToggled);

    displayLayout->addWidget(m_cloudsToggle, 0, Qt::AlignLeft);
    displayLayout->addWidget(m_deforestationToggle, 0, Qt::AlignLeft);

    auto *vegThresholds = makeThresholdGroup(QStringLiteral("Progi roślinności (0–1)"), this);
    auto *vegForm = new QFormLayout(vegThresholds);
    m_sparseSlider = makePercentSlider(0, 100, 20, vegThresholds);
    m_moderateSlider = makePercentSlider(0, 100, 40, vegThresholds);
    m_denseSlider = makePercentSlider(0, 100, 60, vegThresholds);
    m_sparseValue = new QLabel(format01(20), vegThresholds);
    m_moderateValue = new QLabel(format01(40), vegThresholds);
    m_denseValue = new QLabel(format01(60), vegThresholds);
    addSliderRow(vegForm, QStringLiteral("Rzadka"), m_sparseSlider, m_sparseValue);
    addSliderRow(vegForm, QStringLiteral("Umiarkowana"), m_moderateSlider, m_moderateValue);
    addSliderRow(vegForm, QStringLiteral("Gęsta"), m_denseSlider, m_denseValue);

    auto *defThresholds = makeThresholdGroup(QStringLiteral("Progi wylesienia"), this);
    auto *defForm = new QFormLayout(defThresholds);
    m_partialSlider = makePercentSlider(25, 50, 25, defThresholds);
    m_strongSlider = makePercentSlider(25, 50, 50, defThresholds);
    m_partialValue = new QLabel(format01(25), defThresholds);
    m_strongValue = new QLabel(format01(50), defThresholds);
    addSliderRow(defForm, QStringLiteral("Częściowe"), m_partialSlider, m_partialValue);
    addSliderRow(defForm, QStringLiteral("Silne"), m_strongSlider, m_strongValue);

    // ---- Tab 1: kompozycje ----
    m_compositionsPage = new QWidget(m_stack);
    m_compositionsPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *compositionsLayout = new QVBoxLayout(m_compositionsPage);
    compositionsLayout->setSpacing(10);
    compositionsLayout->setContentsMargins(0, 0, 0, 0);

    m_trueColor = new ImagePanelWidget(QStringLiteral("True Color"), m_compositionsPage);
    m_trueColor->setMaxFitHeight(360);
    m_trueColor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_falseColor = new ImagePanelWidget(QStringLiteral("False Color"), m_compositionsPage);
    m_customVegetation = new ImagePanelWidget(QStringLiteral("False Color (zieleń / NDVI)"), m_compositionsPage);
    m_deforestationMask = new ImagePanelWidget(QStringLiteral("Maska wylesień (ΔNDVI)"), m_compositionsPage);

    m_compositionsDisplaySlot = new QWidget(m_compositionsPage);
    m_compositionsDisplaySlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *compositionsDisplayLayout = new QVBoxLayout(m_compositionsDisplaySlot);
    compositionsDisplayLayout->setContentsMargins(0, 0, 0, 0);
    compositionsDisplayLayout->setSpacing(0);

    auto *trueColorColumn = new QWidget(m_compositionsPage);
    trueColorColumn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    auto *trueColorColumnLayout = new QVBoxLayout(trueColorColumn);
    trueColorColumnLayout->setContentsMargins(0, 0, 0, 0);
    trueColorColumnLayout->setSpacing(0);
    trueColorColumnLayout->addWidget(m_trueColor, 0, Qt::AlignTop);
    m_trueColorColumn = trueColorColumn;

    m_splitPanel = new SplitPanelWidget(QStringLiteral("Porównanie lat"), m_compositionsPage);
    m_splitPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_splitPanel, &SplitPanelWidget::modeChanged,
            this, &ResultsViewWidget::onSplitModeChanged);

    auto *splitColumn = new QWidget(m_compositionsPage);
    splitColumn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto *splitColumnLayout = new QVBoxLayout(splitColumn);
    splitColumnLayout->setContentsMargins(0, 0, 0, 0);
    splitColumnLayout->setSpacing(0);
    splitColumnLayout->addWidget(m_splitPanel, 1);
    m_splitColumn = splitColumn;

    m_topRowHost = new QWidget(m_compositionsPage);
    m_topRowHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *topRow = new QHBoxLayout(m_topRowHost);
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(8);
    topRow->setAlignment(Qt::AlignTop);
    topRow->addWidget(trueColorColumn);
    topRow->addWidget(splitColumn);
    compositionsLayout->addWidget(m_topRowHost);

    auto *bottomGrid = new QGridLayout();
    bottomGrid->setHorizontalSpacing(8);
    bottomGrid->setVerticalSpacing(8);
    bottomGrid->addWidget(m_falseColor, 0, 0, Qt::AlignTop);
    bottomGrid->addWidget(m_customVegetation, 0, 1, Qt::AlignTop);
    bottomGrid->addWidget(m_deforestationMask, 0, 2, Qt::AlignTop);
    bottomGrid->addWidget(m_compositionsDisplaySlot, 1, 0, Qt::AlignTop);
    bottomGrid->addWidget(vegThresholds, 1, 1, Qt::AlignTop);
    bottomGrid->addWidget(defThresholds, 1, 2, Qt::AlignTop);
    bottomGrid->setRowStretch(0, 0);
    bottomGrid->setRowStretch(1, 0);
    for (int col = 0; col < 3; ++col) {
        bottomGrid->setColumnStretch(col, 1);
    }
    compositionsLayout->addLayout(bottomGrid, 0);
    compositionsLayout->addStretch(1);
    m_stack->addWidget(m_compositionsPage);

    // ---- Tab 2: pasma ----
    auto *bandsPage = new QWidget(m_stack);
    bandsPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *bandsLayout = new QVBoxLayout(bandsPage);
    bandsLayout->setContentsMargins(0, 0, 0, 0);

    m_bandB4 = new ImagePanelWidget(QStringLiteral("Pasmo B4 (Red)"), bandsPage);
    m_bandB8 = new ImagePanelWidget(QStringLiteral("Pasmo B8 (NIR)"), bandsPage);
    m_bandAverage = new ImagePanelWidget(QStringLiteral("NDVI (normalizedDifference B8, B4)"), bandsPage);

    m_bandsDisplaySlot = new QWidget(bandsPage);
    m_bandsDisplaySlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *bandsDisplayLayout = new QVBoxLayout(m_bandsDisplaySlot);
    bandsDisplayLayout->setContentsMargins(0, 0, 0, 0);
    bandsDisplayLayout->setSpacing(0);

    auto *bandsGrid = new QGridLayout();
    bandsGrid->setHorizontalSpacing(8);
    bandsGrid->setVerticalSpacing(8);
    bandsGrid->addWidget(m_bandB4, 0, 0, Qt::AlignTop);
    bandsGrid->addWidget(m_bandB8, 0, 1, Qt::AlignTop);
    bandsGrid->addWidget(m_bandAverage, 0, 2, Qt::AlignTop);
    bandsGrid->addWidget(m_bandsDisplaySlot, 1, 0, 1, 2, Qt::AlignTop);
    bandsGrid->setRowStretch(0, 0);
    bandsGrid->setRowStretch(1, 0);
    for (int col = 0; col < 3; ++col) {
        bandsGrid->setColumnStretch(col, 1);
    }
    bandsLayout->addLayout(bandsGrid, 0);
    bandsLayout->addStretch(1);
    m_stack->addWidget(bandsPage);

    // ---- Tab 3: statystyki ----
    auto *statsPage = new QWidget(m_stack);
    auto *statsLayout = new QVBoxLayout(statsPage);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    m_chartDetailLabel = new QLabel(
        QStringLiteral("Kliknij kolumnę wykresu, aby zobaczyć wartość."), statsPage);
    m_chartDetailLabel->setObjectName(QStringLiteral("chartDetailLabel"));
    m_chartDetailLabel->setWordWrap(true);
    m_chartView = new QChartView(statsPage);
    m_chartView->setObjectName(QStringLiteral("chartView"));
    m_chartView->setMinimumHeight(320);
    statsLayout->addWidget(m_chartDetailLabel);
    statsLayout->addWidget(m_chartView, 1);
    m_stack->addWidget(statsPage);

    connect(m_tabBar, &QTabBar::currentChanged, this, &ResultsViewWidget::onTabChanged);
    root->addWidget(m_stack, 1);

    const auto connectSlider = [this](QSlider *slider) {
        connect(slider, &QSlider::valueChanged, this, &ResultsViewWidget::onThresholdUiChanged);
    };
    connectSlider(m_sparseSlider);
    connectSlider(m_moderateSlider);
    connectSlider(m_denseSlider);
    connectSlider(m_partialSlider);
    connectSlider(m_strongSlider);

    placeDisplayOptions(0);
    onTabChanged(0);

    setMinimumSize(1000, 700);
    QTimer::singleShot(0, this, &ResultsViewWidget::updateTopRowColumnWidths);
}

void ResultsViewWidget::placeDisplayOptions(int tabIndex)
{
    if (!m_displayGroup) {
        return;
    }

    if (QWidget *oldParent = m_displayGroup->parentWidget()) {
        if (QLayout *layout = oldParent->layout()) {
            layout->removeWidget(m_displayGroup);
        }
    }

    if (tabIndex == 0 && m_compositionsDisplaySlot) {
        auto *layout = qobject_cast<QVBoxLayout *>(m_compositionsDisplaySlot->layout());
        if (layout) {
            layout->addWidget(m_displayGroup, 0, Qt::AlignTop | Qt::AlignLeft);
        }
    } else if (tabIndex == 1 && m_bandsDisplaySlot) {
        auto *layout = qobject_cast<QVBoxLayout *>(m_bandsDisplaySlot->layout());
        if (layout) {
            layout->addWidget(m_displayGroup, 0, Qt::AlignTop | Qt::AlignLeft);
        }
    }

    m_displayGroup->adjustSize();
}

void ResultsViewWidget::onTabChanged(int index)
{
    if (index < 0 || !m_stack) {
        return;
    }

    m_stack->setCurrentIndex(index);
    placeDisplayOptions(index);

    const bool statsTab = (index == 2);
    m_yearBox->setVisible(!statsTab);
    m_displayGroup->setVisible(!statsTab);

    if (index == 0) {
        updateTopRowColumnWidths();
    }
}

void ResultsViewWidget::updateTopRowColumnWidths()
{
    if (!m_topRowHost || !m_trueColorColumn || !m_splitColumn) {
        return;
    }

    const int total = m_topRowHost->width();
    if (total <= 0) {
        return;
    }

    constexpr int spacing = 8;
    const int half = qMax(120, (total - spacing) / 2);
    m_trueColorColumn->setFixedWidth(half);
    m_splitColumn->setFixedWidth(half);

    const int rowH = m_trueColorColumn->sizeHint().height();
    if (rowH > 0) {
        m_splitColumn->setMinimumHeight(rowH);
    }
}

void ResultsViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTopRowColumnWidths();
}

void ResultsViewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &ResultsViewWidget::updateTopRowColumnWidths);
}

void ResultsViewWidget::onNewAnalysisClicked()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("Nowa analiza"),
        QStringLiteral("Czy na pewno chcesz rozpocząć nową analizę?\n"
                       "Bieżące wyniki zostaną utracone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer == QMessageBox::Yes) {
        emit backRequested();
    }
}

void ResultsViewWidget::onSplitModeChanged(int index)
{
    Q_UNUSED(index);
    updateSplitPanel();
}

void ResultsViewWidget::refillYearCombo()
{
    const QSignalBlocker blocker(m_yearCombo);
    m_yearCombo->clear();
    const QVector<int> years = m_result.availableYears();
    for (int year : years) {
        m_yearCombo->addItem(QString::number(year), year);
    }
    if (!years.isEmpty()) {
        m_selectedYear = years.last();
        m_yearCombo->setCurrentIndex(years.size() - 1);
    }
}

void ResultsViewWidget::setResult(const AnalysisResult &result, const AnalysisRequest &request)
{
    m_result = result;
    m_request = request;
    m_showClouds = false;
    m_cloudsToggle->setChecked(false);
    m_showDeforestation = false;
    m_deforestationToggle->setChecked(false);

    refillYearCombo();
    syncSlidersFromRequest();
    rerender();
}

void ResultsViewWidget::clear()
{
    m_result = AnalysisResult{};
    m_yearCombo->clear();
    m_trueColor->clearImage();
    m_falseColor->clearImage();
    m_customVegetation->clearImage();
    m_deforestationMask->clearImage();
    m_bandB4->clearImage();
    m_bandB8->clearImage();
    m_bandAverage->clearImage();
    m_splitPanel->clearImages();
    m_hectaresLabel->setText(QStringLiteral("Utrata lasu: — ha"));
    m_chartCategories.clear();
    if (m_chartDetailLabel) {
        m_chartDetailLabel->setText(
            QStringLiteral("Kliknij kolumnę wykresu, aby zobaczyć wartość."));
    }
    updateChart();
}

void ResultsViewWidget::syncSlidersFromRequest()
{
    const QSignalBlocker b1(m_sparseSlider);
    const QSignalBlocker b2(m_moderateSlider);
    const QSignalBlocker b3(m_denseSlider);
    const QSignalBlocker b4(m_partialSlider);
    const QSignalBlocker b5(m_strongSlider);

    m_sparseSlider->setValue(qRound(m_request.vegetationSparse * 100.0));
    m_moderateSlider->setValue(qRound(m_request.vegetationModerate * 100.0));
    m_denseSlider->setValue(qRound(m_request.vegetationDense * 100.0));
    m_partialSlider->setValue(qRound(m_request.deforestationPartial * 100.0));
    m_strongSlider->setValue(qRound(m_request.deforestationStrong * 100.0));

    m_sparseValue->setText(format01(m_sparseSlider->value()));
    m_moderateValue->setText(format01(m_moderateSlider->value()));
    m_denseValue->setText(format01(m_denseSlider->value()));
    m_partialValue->setText(format01(m_partialSlider->value()));
    m_strongValue->setText(format01(m_strongSlider->value()));
}

void ResultsViewWidget::readThresholdsToRequest()
{
    m_request.vegetationSparse = m_sparseSlider->value() / 100.0;
    m_request.vegetationModerate = m_moderateSlider->value() / 100.0;
    m_request.vegetationDense = m_denseSlider->value() / 100.0;
    m_request.deforestationPartial = m_partialSlider->value() / 100.0;
    m_request.deforestationStrong = m_strongSlider->value() / 100.0;
}

void ResultsViewWidget::onThresholdUiChanged()
{
    m_sparseValue->setText(format01(m_sparseSlider->value()));
    m_moderateValue->setText(format01(m_moderateSlider->value()));
    m_denseValue->setText(format01(m_denseSlider->value()));
    m_partialValue->setText(format01(m_partialSlider->value()));
    m_strongValue->setText(format01(m_strongSlider->value()));

    if (m_moderateSlider->value() <= m_sparseSlider->value()) {
        m_moderateSlider->setValue(m_sparseSlider->value() + 1);
        return;
    }
    if (m_denseSlider->value() <= m_moderateSlider->value()) {
        m_denseSlider->setValue(m_moderateSlider->value() + 1);
        return;
    }
    if (m_strongSlider->value() <= m_partialSlider->value()) {
        m_strongSlider->setValue(m_partialSlider->value() + 1);
        return;
    }

    readThresholdsToRequest();
    rerender();
    emit thresholdsChanged(m_request);
}

void ResultsViewWidget::onCloudsToggled(bool checked)
{
    m_showClouds = checked;
    rerender();
}

void ResultsViewWidget::onYearIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    m_selectedYear = m_yearCombo->itemData(index).toInt();
    rerender();
}

void ResultsViewWidget::onDeforestationToggled(bool checked)
{
    m_showDeforestation = checked;
    rerender();
}

const YearBands *ResultsViewWidget::activeYear() const
{
    return m_result.bandsFor(m_selectedYear);
}

QImage ResultsViewWidget::compositionImage(int year, bool trueColor) const
{
    const YearBands *yearBands = m_result.bandsFor(year);
    if (!yearBands) {
        return {};
    }

    const BandBuffer &b2 = m_showClouds ? yearBands->b2Clouds : yearBands->b2;
    const BandBuffer &b3 = m_showClouds ? yearBands->b3Clouds : yearBands->b3;
    const BandBuffer &b4 = m_showClouds ? yearBands->b4Clouds : yearBands->b4;
    const BandBuffer &b8 = m_showClouds ? yearBands->b8Clouds : yearBands->b8;

    if (trueColor) {
        return ImageRenderer::trueColor(b2, b3, b4);
    }
    return ImageRenderer::falseColor(b3, b4, b8);
}

void ResultsViewWidget::updateSplitPanel()
{
    if (!m_result.isValid()) {
        return;
    }

    const int leftYear = m_result.firstYear();
    const int rightYear = m_selectedYear;
    if (leftYear == rightYear) {
        m_splitPanel->clearImages();
        return;
    }

    const bool trueColorMode = (m_splitPanel->modeIndex() == 0);
    const QImage left = compositionImage(leftYear, trueColorMode);
    const QImage right = compositionImage(rightYear, trueColorMode);

    m_splitPanel->setImages(
        left, right,
        QString::number(leftYear),
        QString::number(rightYear));
}

void ResultsViewWidget::rerender()
{
    if (!m_result.isValid()) {
        return;
    }

    const YearBands *year = activeYear();
    if (!year) {
        return;
    }

    QImage maskImage;
    const int y1 = m_result.firstYear();
    const int y2 = m_selectedYear;
    const YearBands *t1 = m_result.bandsFor(y1);
    const YearBands *t2 = m_result.bandsFor(y2);

    if (t1 && t2 && y1 != y2) {
        const BandBuffer ndviT1 = BandBuffer::ndvi(t1->b4, t1->b8);
        const BandBuffer ndviT2 = BandBuffer::ndvi(t2->b4, t2->b8);

        maskImage = ImageRenderer::deforestationMask(
            ndviT1, ndviT2,
            m_request.deforestationPartial,
            m_request.deforestationStrong);

        m_deforestationMask->setImage(maskImage);

        const BandBuffer *area = m_result.pixelAreaM2.isEmpty() ? nullptr
                                                                : &m_result.pixelAreaM2;
        const double ha = ImageRenderer::hectaresFromMask(
            ndviT1, ndviT2, m_request.deforestationPartial, area);
        m_hectaresLabel->setText(
            QStringLiteral("Utrata lasu: %1 ha  (%2 → %3)")
                .arg(ha, 0, 'f', 2)
                .arg(y1)
                .arg(y2));
    } else {
        m_deforestationMask->clearImage();
        m_hectaresLabel->setText(QStringLiteral("Utrata lasu: — (za mało lat)"));
    }

    QImage transparentMask;
    if (m_showDeforestation && !maskImage.isNull()) {
        transparentMask = maskImage.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < transparentMask.height(); ++y) {
            for (int x = 0; x < transparentMask.width(); ++x) {
                QColor c = transparentMask.pixelColor(x, y);
                if (c.red() > 127) {
                    c.setAlpha(255);
                    transparentMask.setPixelColor(x, y, c);
                } else {
                    transparentMask.setPixelColor(x, y, Qt::transparent);
                }
            }
        }
    }

    auto applyMask = [&](const QImage &baseImage) {
        if (!m_showDeforestation) {
            return baseImage;
        }
        QImage result = baseImage.convertToFormat(QImage::Format_ARGB32);
        QPainter painter(&result);
        painter.drawImage(0, 0, transparentMask);
        return result;
    };

    const BandBuffer &b2 = m_showClouds ? year->b2Clouds : year->b2;
    const BandBuffer &b3 = m_showClouds ? year->b3Clouds : year->b3;
    const BandBuffer &b4 = m_showClouds ? year->b4Clouds : year->b4;
    const BandBuffer &b8 = m_showClouds ? year->b8Clouds : year->b8;

    m_trueColor->setImage(applyMask(ImageRenderer::trueColor(b2, b3, b4)));
    m_falseColor->setImage(applyMask(ImageRenderer::falseColor(b3, b4, b8)));

    const BandBuffer ndvi = BandBuffer::ndvi(b4, b8);
    m_customVegetation->setImage(applyMask(ImageRenderer::customVegetation(
        ndvi,
        m_request.vegetationSparse,
        m_request.vegetationModerate,
        m_request.vegetationDense)));

    m_bandB4->setImage(applyMask(ImageRenderer::grayscaleBand(b4)));
    m_bandB8->setImage(applyMask(ImageRenderer::grayscaleBand(b8)));
    m_bandAverage->setImage(applyMask(
        ImageRenderer::normalizedDifference(b8, b4)));

    updateSplitPanel();
    updateChart();
}

void ResultsViewWidget::onChartBarClicked(int index, QBarSet *barSet)
{
    if (!barSet || index < 0 || index >= m_chartCategories.size()) {
        return;
    }

    m_chartDetailLabel->setText(
        QStringLiteral("Rok %1: %2 ha")
            .arg(m_chartCategories.at(index))
            .arg(barSet->at(index), 0, 'f', 2));
}

void ResultsViewWidget::updateChart()
{
    m_chartCategories.clear();

    if (!m_result.isValid() || m_result.years.size() < 2) {
        m_chartView->setChart(new QChart());
        if (m_chartDetailLabel) {
            m_chartDetailLabel->setText(
                QStringLiteral("Brak danych statystycznych."));
        }
        return;
    }

    auto *chart = new QChart();
    chart->setTitle(QStringLiteral("Utrata lasu (ha)"));

    const int baseYear = m_result.firstYear();
    const YearBands *firstYear = m_result.bandsFor(baseYear);
    if (!firstYear) {
        m_chartView->setChart(chart);
        return;
    }

    const BandBuffer firstNdvi = BandBuffer::ndvi(firstYear->b4, firstYear->b8);
    const BandBuffer *area = m_result.pixelAreaM2.isEmpty() ? nullptr : &m_result.pixelAreaM2;

    auto *barSet = new QBarSet(QStringLiteral("hektary"));
    QStringList categories;

    int year = baseYear + 1;
    while (m_result.availableYears().contains(year)) {
        categories << QString::number(year);
        const YearBands *nextYear = m_result.bandsFor(year);
        const BandBuffer nextNdvi = BandBuffer::ndvi(nextYear->b4, nextYear->b8);
        *barSet << ImageRenderer::hectaresFromMask(
            firstNdvi, nextNdvi, m_request.deforestationPartial, area);
        ++year;
    }

    m_chartCategories = categories;

    auto *series = new QBarSeries();
    series->append(barSet);
    chart->addSeries(series);

    connect(series, &QBarSeries::clicked,
            this, &ResultsViewWidget::onChartBarClicked);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    AppTheme::styleChart(chart, m_chartView);
    m_chartView->setChart(chart);

    if (m_chartDetailLabel) {
        m_chartDetailLabel->setText(
            QStringLiteral("Kliknij kolumnę wykresu, aby zobaczyć wartość."));
    }
}
