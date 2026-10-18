#pragma once

#include <QList>
#include <QPointer>
#include <QWidget>

namespace mrs::ui
{

// Индикатор кругового обзора. Пока рисует рабочую область и шкалу пеленгов —
// радиолокационное видео и символика добавляются на этапе 1 (docs/05-operator-ui.md, §5.3).
class PpiWidget final : public QWidget
{
public:
    explicit PpiWidget(QWidget* parent = nullptr);

    // Блоки, наложенные на углы области ИКО: круг со шкалой уменьшается так,
    // чтобы не заходить под них при любом размере окна.
    void setReservedAreas(const QList<QWidget*>& widgets);

    // Горизонтальное положение круга, когда виджет шире своей высоты:
    // Qt::AlignRight — вплотную к правому краю (к колонке данных).
    void setAlignment(Qt::Alignment alignment);

    // Окружность рабочей области (внутренние концы рисок) в координатах виджета.
    [[nodiscard]] QPointF center() const;
    [[nodiscard]] double radius() const;

    // Центр подписи шкалы пеленгов для круга номинального размера (без учёта наложенных блоков) —
    // по нему выравниваются сами блоки (например, ВН под цифрой 220).
    [[nodiscard]] QPointF bearingLabelCenter(double degrees) const;

    // Прямоугольник, который занимает подпись шкалы (цифры) при отрисовке.
    [[nodiscard]] QRectF bearingLabelRect(double degrees) const;

    // Прямоугольник одной цифры подписи (index — номер цифры слева, с 0): «3» в «310» — index 0.
    [[nodiscard]] QRectF bearingLabelDigitRect(double degrees, int index) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] double outerRadius() const;
    [[nodiscard]] double nominalRadius() const;   // без учёта наложенных блоков
    [[nodiscard]] QRectF labelRectAt(double degrees, double ringRadius) const;
    [[nodiscard]] QFont labelFont() const;
    void drawBearingScale(QPainter& painter) const;
    void drawHeadingLine(QPainter& painter) const;

    QList<QPointer<QWidget>> reservedAreas_;
    Qt::Alignment alignment_{Qt::AlignHCenter};
};

} // namespace mrs::ui
