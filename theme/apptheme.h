#ifndef APPTHEME_H
#define APPTHEME_H

class QApplication;
class QChart;
class QChartView;

namespace AppTheme {

/** Ciemny motyw z niebieskimi akcentami — stosowany globalnie na QApplication. */
void apply(QApplication &app);

/** Dopasowanie wykresu Qt Charts do motywu (QSS tego nie obejmuje). */
void styleChart(QChart *chart, QChartView *view = nullptr);

} // namespace AppTheme

#endif // APPTHEME_H
