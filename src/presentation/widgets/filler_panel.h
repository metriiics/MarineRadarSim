#pragma once

#include <QFrame>
#include <QImage>
#include <QList>
#include <QString>

namespace mrs::ui
{

// Оформление боковых полей за границами интерфейса РЛС (на широком мониторе).
enum class FillerStyle
{
    Logo,         // логотип Дальрыбвтуза на карточке (по умолчанию)
    CompassRose,  // роза ветров
    Grid,         // приглушённая сетка
    Title,        // название тренажёра
    None,         // пусто
    LogoGrid,     // логотип Дальрыбвтуза на сетке во всю высоту (добавлен последним — номера
                  // остальных вариантов в сохранённых настройках не меняются)
};

[[nodiscard]] QString fillerStyleName(FillerStyle style);
[[nodiscard]] QList<FillerStyle> allFillerStyles();

// Область с выбранным оформлением: боковое поле (без рамки) или блок в рамке (предпросмотр).
//
// Логотип берётся из файла data/branding/dalrybvtuz.png (или .svg) рядом с программой.
// Если файла нет, рисуется условная эмблема: якорь в круге и надпись «ДАЛЬРЫБВТУЗ».
class FillerPanel final : public QFrame
{
public:
    enum class Frame
    {
        Panel,  // блок в рамке
        None,   // без рамки — боковое поле
    };

    explicit FillerPanel(QWidget* parent = nullptr, Frame frame = Frame::Panel);

    void setFillerStyle(FillerStyle style);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawLogo(QPainter& painter, const QRectF& area) const;
    void drawLogoGrid(QPainter& painter, const QRectF& area) const;
    void drawCompassRose(QPainter& painter, const QRectF& area) const;
    void drawGrid(QPainter& painter, const QRectF& area) const;
    void drawTitle(QPainter& painter, const QRectF& area) const;

    FillerStyle style_{FillerStyle::Logo};
};

} // namespace mrs::ui
