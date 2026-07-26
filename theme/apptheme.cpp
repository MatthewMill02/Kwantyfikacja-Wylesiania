#include "apptheme.h"

#include <QApplication>
#include <QPalette>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>

#include <QFont>
#include <QPainter>

namespace {

struct Colors
{
    static constexpr const char *bgDeep      = "#0c1219";
    static constexpr const char *bgMain      = "#111827";
    static constexpr const char *bgSurface   = "#1a2435";
    static constexpr const char *bgElevated  = "#1f2d42";
    static constexpr const char *bgInput     = "#152032";

    static constexpr const char *border      = "#2d4a6f";
    static constexpr const char *borderFocus = "#3b82f6";

    static constexpr const char *accent      = "#2563eb";
    static constexpr const char *accentHover = "#3b82f6";
    static constexpr const char *accentLight = "#60a5fa";
    static constexpr const char *accentDark  = "#1d4ed8";

    static constexpr const char *text        = "#e8edf4";
    static constexpr const char *textSoft    = "#94a3b8";
    static constexpr const char *textMuted   = "#64748b";
    static constexpr const char *error       = "#f87171";
};

QString stylesheet()
{
    return QStringLiteral(R"(
/* ---- Global ---- */
QWidget {
    background-color: %1;
    color: %2;
    font-family: "Segoe UI", "Inter", sans-serif;
    font-size: 13px;
}

QMainWindow, QStackedWidget, QScrollArea, QScrollArea > QWidget > QWidget {
    background-color: %1;
}

/* ---- Nagłówki stron ---- */
QLabel#labelTitle {
    font-size: 22px;
    font-weight: 700;
    color: %3;
    padding: 4px 0;
}

QLabel#labelSubtitle {
    color: %4;
    font-size: 13px;
    line-height: 1.4;
}

QLabel#labelLoadingTitle {
    font-size: 17px;
    font-weight: 600;
    color: %3;
}

QLabel#labelLoadingStage {
    color: %4;
    font-size: 13px;
}

QLabel#labelValidation {
    color: %5;
    font-weight: 500;
    padding: 4px 0;
}

QLabel#labelHectares {
    font-size: 22px;
    font-weight: 700;
    color: %6;
    padding: 4px 0;
}

QLabel#chartDetailLabel {
    color: %4;
    font-size: 14px;
    font-weight: 600;
    padding: 4px 2px 8px 2px;
}

QFrame#resultsHeaderSeparator {
    color: %8;
    background-color: %8;
    border: none;
    max-height: 1px;
}

QStackedWidget#resultsStack {
    background-color: %10;
    border: none;
    padding: 8px 0 0 0;
    margin-top: 0;
}

QScrollArea#imagePanelScroll {
    background-color: transparent;
    border: none;
}

QScrollArea#imagePanelScroll > QWidget > QWidget {
    background-color: transparent;
}

QLabel#imagePanelTitle {
    font-weight: 600;
    color: %4;
    padding: 2px;
}

QLabel#imagePanelPreview {
    background-color: %7;
    border: 1px solid %8;
    border-radius: 6px;
    color: %9;
}

QWidget#splitPanelPreview {
    background-color: %7;
    border: 1px solid %8;
    border-radius: 6px;
    color: %9;
}

QComboBox#splitPanelModeCombo {
    background-color: %10;
    border: 1px solid %8;
    border-radius: 6px;
    padding: 4px 8px;
    min-height: 18px;
    font-size: 12px;
}

QComboBox#splitPanelModeCombo:hover {
    border-color: %13;
}

QComboBox#splitPanelModeCombo::drop-down {
    border: none;
    width: 18px;
}

QLabel#splitPanelCaption {
    color: %6;
    font-weight: 600;
    font-size: 12px;
    margin: 0;
    padding: 0;
}

QSlider#splitPanelSlider {
    margin-top: 0;
}

QGroupBox#thresholdGroup {
    margin-top: 10px;
    padding-top: 14px;
}

QGroupBox#displayOptionsGroup {
    margin-top: 8px;
    padding: 10px 12px 10px 12px;
}

QGroupBox#yearPreviewGroup {
    margin-top: 12px;
    padding-top: 16px;
    padding-right: 10px;
    padding-bottom: 8px;
    padding-left: 10px;
}

QGroupBox#yearPreviewGroup::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 6px;
}

QCheckBox#displayOptionToggle {
    padding: 2px 4px;
    background: transparent;
}

/* ---- GroupBox ---- */
QGroupBox {
    background-color: %10;
    border: 1px solid %8;
    border-radius: 10px;
    margin-top: 14px;
    padding: 16px 14px 12px 14px;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 8px;
    color: %6;
}

/* ---- Taby ---- */
QTabBar::tab {
    background-color: %11;
    color: %4;
    border: 1px solid %8;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    padding: 8px 18px;
    margin-right: 2px;
    font-weight: 600;
}

QTabBar::tab:selected {
    background-color: %10;
    color: %6;
    border-color: %8;
}

QTabBar::tab:hover:!selected {
    background-color: %12;
    color: %3;
}

/* ---- Pola wejściowe ---- */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: %11;
    border: 1px solid %8;
    border-radius: 6px;
    padding: 6px 10px;
    color: %2;
    min-height: 20px;
    selection-background-color: %12;
    selection-color: %2;
}

QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover {
    border-color: %13;
}

QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: %10;
    border: none;
    width: 18px;
}

QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: %8;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 6px solid %4;
    margin-right: 8px;
}

QComboBox QAbstractItemView {
    background-color: %10;
    border: 1px solid %8;
    selection-background-color: %12;
    selection-color: %2;
    outline: none;
}

/* ---- Przyciski ---- */
QPushButton {
    background-color: %10;
    border: 1px solid %8;
    border-radius: 8px;
    padding: 8px 18px;
    color: %2;
    font-weight: 500;
    min-height: 18px;
}

QPushButton:hover {
    background-color: %14;
    border-color: %13;
}

QPushButton:pressed {
    background-color: %15;
}

QPushButton:default, QPushButton#buttonStart {
    background-color: %12;
    border: 1px solid %16;
    color: #ffffff;
    font-weight: 600;
}

QPushButton:default:hover, QPushButton#buttonStart:hover {
    background-color: %13;
    border-color: %6;
}

QPushButton:default:pressed, QPushButton#buttonStart:pressed {
    background-color: %16;
}

/* ---- Checkbox ---- */
QCheckBox {
    spacing: 8px;
    color: %2;
}

QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border-radius: 4px;
    border: 1px solid %8;
    background-color: %11;
}

QCheckBox::indicator:hover {
    border-color: %13;
}

QCheckBox::indicator:checked {
    background-color: %12;
    border-color: %6;
}

/* ---- Suwaki ---- */
QSlider::groove:horizontal {
    height: 6px;
    background: %11;
    border-radius: 3px;
    border: 1px solid %8;
}

QSlider::handle:horizontal {
    width: 16px;
    margin: -6px 0;
    border-radius: 8px;
    background: %6;
    border: 2px solid %3;
}

QSlider::handle:horizontal:hover {
    background: %13;
}

QSlider::sub-page:horizontal {
    background: %12;
    border-radius: 3px;
}

/* ---- ProgressBar ---- */
QProgressBar {
    background-color: %11;
    border: 1px solid %8;
    border-radius: 8px;
    text-align: center;
    color: %2;
    min-height: 22px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 %16, stop:1 %6);
    border-radius: 7px;
}

/* ---- Scrollbar ---- */
QScrollBar:vertical {
    background: %1;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: %8;
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background: %13;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background: %1;
    height: 10px;
}

QScrollBar::handle:horizontal {
    background: %8;
    border-radius: 5px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background: %13;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* ---- Menu / status ---- */
QMenuBar {
    background-color: %17;
    border-bottom: 1px solid %8;
    padding: 2px 0;
}

QMenuBar::item {
    padding: 6px 12px;
    background: transparent;
}

QMenuBar::item:selected {
    background-color: %14;
    border-radius: 4px;
}

QStatusBar {
    background-color: %17;
    border-top: 1px solid %8;
    color: %4;
}

/* ---- Form labels ---- */
QFormLayout QLabel, QLabel {
    background: transparent;
}

QLabel#labelPixelEstimate {
    color: %4;
    font-size: 12px;
    padding: 4px 2px;
}

QWidget#osmMapWidget {
    border: 1px solid %8;
    border-radius: 10px;
}

QChartView#chartView {
    background-color: %10;
    border: 1px solid %8;
    border-radius: 10px;
}
)")
        .arg(Colors::bgMain)       // 1
        .arg(Colors::text)         // 2
        .arg(Colors::text)         // 3 title bright
        .arg(Colors::textSoft)     // 4
        .arg(Colors::error)        // 5
        .arg(Colors::accentLight)  // 6 accent light
        .arg(Colors::bgDeep)       // 7 image bg
        .arg(Colors::border)       // 8
        .arg(Colors::textMuted)    // 9
        .arg(Colors::bgSurface)    // 10
        .arg(Colors::bgInput)      // 11
        .arg(Colors::accent)       // 12
        .arg(Colors::accentHover)  // 13
        .arg(Colors::bgElevated)   // 14 hover btn
        .arg(Colors::accentDark)   // 15 pressed
        .arg(Colors::accentDark)   // 16 gradient / border
        .arg(Colors::bgDeep);      // 17 menubar/status
}

} // namespace

void AppTheme::apply(QApplication &app)
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(Colors::bgMain));
    palette.setColor(QPalette::WindowText, QColor(Colors::text));
    palette.setColor(QPalette::Base, QColor(Colors::bgInput));
    palette.setColor(QPalette::AlternateBase, QColor(Colors::bgSurface));
    palette.setColor(QPalette::Text, QColor(Colors::text));
    palette.setColor(QPalette::Button, QColor(Colors::bgSurface));
    palette.setColor(QPalette::ButtonText, QColor(Colors::text));
    palette.setColor(QPalette::Highlight, QColor(Colors::accent));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipBase, QColor(Colors::bgElevated));
    palette.setColor(QPalette::ToolTipText, QColor(Colors::text));
    palette.setColor(QPalette::Link, QColor(Colors::accentLight));
    app.setPalette(palette);
    app.setStyleSheet(stylesheet());
}

void AppTheme::styleChart(QChart *chart, QChartView *view)
{
    if (!chart) {
        return;
    }

    const QColor bg(Colors::bgSurface);
    const QColor text(Colors::text);
    const QColor textSoft(Colors::textSoft);
    const QColor grid(QColor(Colors::border));
    const QColor barColor(Colors::accentHover);

    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(bg);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QColor(Colors::bgElevated));
    chart->setTitleBrush(text);
    chart->setTitleFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::DemiBold));

    if (QLegend *legend = chart->legend()) {
        legend->setLabelColor(textSoft);
        legend->setBackgroundVisible(false);
    }

    const auto axes = chart->axes();
    for (QAbstractAxis *axis : axes) {
        axis->setLabelsColor(textSoft);
        axis->setTitleBrush(textSoft);
        axis->setGridLineColor(grid);
        axis->setLinePenColor(grid);
    }

    const auto seriesList = chart->series();
    for (QAbstractSeries *series : seriesList) {
        if (auto *barSeries = qobject_cast<QBarSeries *>(series)) {
            for (QBarSet *set : barSeries->barSets()) {
                set->setColor(barColor);
                set->setBorderColor(QColor(Colors::accentLight));
            }
        }
    }

    if (view) {
        view->setBackgroundBrush(bg);
        view->setRenderHint(QPainter::Antialiasing);
    }
}
