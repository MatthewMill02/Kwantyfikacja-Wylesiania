#include "resultsviewwidget.h"
#include "imagepanelwidget.h"

#include "models/bandbuffer.h"
#include "processing/imagerenderer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

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

} // namespace

ResultsViewWidget::ResultsViewWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void ResultsViewWidget::buildUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *gridHost = new QWidget(scroll);
    auto *grid = new QGridLayout(gridHost);
    grid->setSpacing(8);

    m_trueColor = new ImagePanelWidget(QStringLiteral("True Color"), gridHost);
    m_falseColor = new ImagePanelWidget(QStringLiteral("False Color"), gridHost);
    m_customVegetation = new ImagePanelWidget(QStringLiteral("False Color (zieleń)"), gridHost);
    m_deforestationMask = new ImagePanelWidget(QStringLiteral("Maska wylesień"), gridHost);
    m_bandB4 = new ImagePanelWidget(QStringLiteral("Pasmo B4 (Red)"), gridHost);
    m_bandB8 = new ImagePanelWidget(QStringLiteral("Pasmo B8 (NIR)"), gridHost);
    m_bandAverage = new ImagePanelWidget(QStringLiteral("Średnia B4 + B8"), gridHost);

    grid->addWidget(m_trueColor, 0, 0);
    grid->addWidget(m_falseColor, 0, 1);
    grid->addWidget(m_customVegetation, 0, 2);
    grid->addWidget(m_deforestationMask, 1, 0);
    grid->addWidget(m_bandB4, 1, 1);
    grid->addWidget(m_bandB8, 1, 2);
    grid->addWidget(m_bandAverage, 2, 0);

    scroll->setWidget(gridHost);
    root->addWidget(scroll, 1);

    auto *side = new QVBoxLayout();
    side->setSpacing(10);

    auto *backBtn = new QPushButton(QStringLiteral("Nowa analiza"), this);
    connect(backBtn, &QPushButton::clicked, this, &ResultsViewWidget::backRequested);
    side->addWidget(backBtn);

    m_hectaresLabel = new QLabel(QStringLiteral("Utrata lasu: — ha"), this);
    m_hectaresLabel->setWordWrap(true);
    m_hectaresLabel->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 600;"));
    side->addWidget(m_hectaresLabel);

    auto *yearBox = new QGroupBox(QStringLiteral("Rok podglądu"), this);
    auto *yearLayout = new QVBoxLayout(yearBox);
    m_yearCombo = new QComboBox(yearBox);
    connect(m_yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResultsViewWidget::onYearIndexChanged);
    yearLayout->addWidget(m_yearCombo);
    side->addWidget(yearBox);

    m_cloudsToggle = new QCheckBox(QStringLiteral("Pokaż wersje z chmurami"), this);
    m_cloudsToggle->setToolTip(
        QStringLiteral("Przełącza wszystkie grafiki (oprócz maski wylesień) na wersje z chmurami (*_CHM)."));
    connect(m_cloudsToggle, &QCheckBox::toggled, this, &ResultsViewWidget::onCloudsToggled);
    side->addWidget(m_cloudsToggle);

    auto *vegBox = new QGroupBox(QStringLiteral("Progi roślinności (0–1)"), this);
    auto *vegForm = new QFormLayout(vegBox);

    m_sparseSlider = makePercentSlider(0, 100, 20, vegBox);
    m_moderateSlider = makePercentSlider(0, 100, 40, vegBox);
    m_denseSlider = makePercentSlider(0, 100, 60, vegBox);
    m_sparseValue = new QLabel(format01(20), vegBox);
    m_moderateValue = new QLabel(format01(40), vegBox);
    m_denseValue = new QLabel(format01(60), vegBox);

    auto addSliderRow = [](QFormLayout *form, const QString &name, QSlider *s, QLabel *v) {
        auto *row = new QWidget();
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(s, 1);
        h->addWidget(v);
        form->addRow(name, row);
    };

    addSliderRow(vegForm, QStringLiteral("Rzadka"), m_sparseSlider, m_sparseValue);
    addSliderRow(vegForm, QStringLiteral("Umiarkowana"), m_moderateSlider, m_moderateValue);
    addSliderRow(vegForm, QStringLiteral("Gęsta"), m_denseSlider, m_denseValue);
    side->addWidget(vegBox);

    auto *defBox = new QGroupBox(QStringLiteral("Progi wylesienia"), this);
    auto *defForm = new QFormLayout(defBox);
    m_partialSlider = makePercentSlider(25, 50, 25, defBox);
    m_strongSlider = makePercentSlider(25, 50, 50, defBox);
    m_partialValue = new QLabel(format01(25), defBox);
    m_strongValue = new QLabel(format01(50), defBox);
    addSliderRow(defForm, QStringLiteral("Częściowe"), m_partialSlider, m_partialValue);
    addSliderRow(defForm, QStringLiteral("Silne"), m_strongSlider, m_strongValue);
    side->addWidget(defBox);

    side->addStretch(1);

    const auto connectSlider = [this](QSlider *slider) {
        connect(slider, &QSlider::valueChanged, this, &ResultsViewWidget::onThresholdUiChanged);
    };
    connectSlider(m_sparseSlider);
    connectSlider(m_moderateSlider);
    connectSlider(m_denseSlider);
    connectSlider(m_partialSlider);
    connectSlider(m_strongSlider);

    root->addLayout(side, 0);
    setMinimumSize(1000, 700);
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
    m_hectaresLabel->setText(QStringLiteral("Utrata lasu: — ha"));
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

const YearBands *ResultsViewWidget::activeYear() const
{
    return m_result.bandsFor(m_selectedYear);
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

    const BandBuffer &b2 = m_showClouds ? year->b2Clouds : year->b2;
    const BandBuffer &b3 = m_showClouds ? year->b3Clouds : year->b3;
    const BandBuffer &b4 = m_showClouds ? year->b4Clouds : year->b4;
    const BandBuffer &b8 = m_showClouds ? year->b8Clouds : year->b8;

    m_trueColor->setImage(ImageRenderer::trueColor(b2, b3, b4));
    m_falseColor->setImage(ImageRenderer::falseColor(b3, b4, b8));

    const BandBuffer ndvi = BandBuffer::ndvi(b4, b8);
    m_customVegetation->setImage(ImageRenderer::customVegetation(
        ndvi,
        m_request.vegetationSparse,
        m_request.vegetationModerate,
        m_request.vegetationDense));

    const int y1 = m_result.firstYear();
    const int y2 = m_result.lastYear();
    const YearBands *t1 = m_result.bandsFor(y1);
    const YearBands *t2 = m_result.bandsFor(y2);

    if (t1 && t2 && y1 != y2) {
        const BandBuffer ndviT1 = BandBuffer::ndvi(t1->b4, t1->b8);
        const BandBuffer ndviT2 = BandBuffer::ndvi(t2->b4, t2->b8);
        m_deforestationMask->setImage(ImageRenderer::deforestationMask(
            ndviT1, ndviT2,
            m_request.deforestationPartial,
            m_request.deforestationStrong));

        const BandBuffer *area = m_result.pixelAreaM2.isEmpty() ? nullptr
                                                               : &m_result.pixelAreaM2;
        const double ha = ImageRenderer::hectaresFromMask(
            ndviT1, ndviT2, m_request.deforestationPartial, area);
        m_hectaresLabel->setText(
            QStringLiteral("Utrata lasu: %1 ha\n(%2 → %3)")
                .arg(ha, 0, 'f', 2)
                .arg(y1)
                .arg(y2));
    } else {
        m_deforestationMask->clearImage();
        m_hectaresLabel->setText(QStringLiteral("Utrata lasu: — (za mało lat)"));
    }

    m_bandB4->setImage(ImageRenderer::grayscaleBand(b4));
    m_bandB8->setImage(ImageRenderer::grayscaleBand(b8));
    m_bandAverage->setImage(
        ImageRenderer::grayscaleBand(BandBuffer::average(b4, b8)));
}
