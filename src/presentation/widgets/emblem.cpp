#include "emblem.h"

#include "presentation/theme/theme.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace mrs::ui
{

namespace
{

// Цвета эмблемы берутся из текущей цветовой схемы, чтобы она была частью экрана РЛС.
QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(static_cast<float>(a.redF() + (b.redF() - a.redF()) * t),
                            static_cast<float>(a.greenF() + (b.greenF() - a.greenF()) * t),
                            static_cast<float>(a.blueF() + (b.blueF() - a.blueF()) * t));
}

QColor markLight() { return mix(theme::palette().value, QColor(255, 255, 255), 0.25); }
QColor markDark() { return theme::palette().frame; }
QColor accent() { return theme::palette().text; }

constexpr double kCardMaxWidth = 300.0;
constexpr double kCardMargin = 10.0;   // от края бокового поля до карточки
constexpr double kCardRadius = 14.0;

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

// Знак «рыба-глобус» в единицах радиуса глобуса R с центром глобуса в c.
// Ширина знака — 3.9R (от носа до хвоста), высота — 3R (с указателями сверху и снизу).
// Геометрия — по мотивам знака на dalrybvtuz.ru.
void drawMark(QPainter& painter, const QPointF& c, double R, const QColor& light, const QColor& dark)
{
    const auto at = [&](double x, double y) { return QPointF(c.x() + x * R, c.y() + y * R); };

    QLinearGradient fill(c.x(), c.y() - 1.5 * R, c.x(), c.y() + 1.5 * R);
    fill.setColorAt(0.0, light);
    fill.setColorAt(1.0, dark);

    QPen pen(QBrush(fill), 0.18 * R);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Глобус: обод, ось, экватор и две параллели, выгнутые к экватору (перспектива).
    painter.drawEllipse(c, 0.9 * R, 0.9 * R);
    painter.drawLine(at(0, -0.9), at(0, 0.9));
    painter.drawLine(at(-0.9, 0), at(0.9, 0));
    for (const double side : {-1.0, 1.0})
    {
        QPainterPath parallel;
        parallel.moveTo(at(-0.76, side * 0.48));
        parallel.quadTo(at(0, side * 0.12), at(0.76, side * 0.48));
        painter.drawPath(parallel);
    }

    // Голова и хвост соединены с глобусом дугами, которые обнимают его (радиус 1.22R).
    // Углы — в системе Qt: 0° — вправо, против часовой стрелки.
    constexpr double kHug = 1.22;
    const QRectF hugRect(c.x() - kHug * R, c.y() - kHug * R, 2 * kHug * R, 2 * kHug * R);
    constexpr double kHugStart = 153.2;  // точка (−1.09R, ∓0.55R)
    constexpr double kHugSweep = 36.4;   // до точки (−0.55R, ∓1.09R)
    constexpr double kBarY = 1.09;

    for (const double side : {-1.0, 1.0})  // -1 — верхняя половина, 1 — нижняя
    {
        // Голова: дуга у глобуса, перекладина и шеврон к носу.
        QPainterPath head;
        head.arcMoveTo(hugRect, side < 0 ? kHugStart : -kHugStart);
        head.arcTo(hugRect, side < 0 ? kHugStart : -kHugStart, side < 0 ? -kHugSweep : kHugSweep);
        head.lineTo(at(-1.36, side * kBarY));
        head.lineTo(at(-1.95, 0));
        painter.drawPath(head);

        // Хвост: дуга у глобуса, перекладина и лопасть с выемкой посередине.
        QPainterPath tail;
        tail.arcMoveTo(hugRect, side < 0 ? 180.0 - kHugStart : kHugStart - 180.0);
        tail.arcTo(hugRect, side < 0 ? 180.0 - kHugStart : kHugStart - 180.0, side < 0 ? kHugSweep : -kHugSweep);
        tail.lineTo(at(1.86, side * kBarY));
        tail.quadTo(at(1.86, side * 0.22), at(1.32, 0));
        painter.drawPath(tail);
    }

    // Глаз и указатели «север — юг».
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(fill));
    painter.drawEllipse(at(-1.4, 0), 0.12 * R, 0.12 * R);
    for (const double side : {-1.0, 1.0})
    {
        painter.drawPolygon(QPolygonF{at(-0.2, side * 1.1), at(0, side * 1.52), at(0.2, side * 1.1)});
    }
}

void drawSonarRings(QPainter& painter, const QPointF& c, double R)
{
    painter.setBrush(Qt::NoBrush);
    const double radii[] = {2.25, 2.85, 3.45};
    const int alphas[] = {46, 30, 18};
    for (int i = 0; i < 3; ++i)
    {
        painter.setPen(QPen(withAlpha(accent(), alphas[i]), 1.0));
        painter.drawEllipse(c, radii[i] * R, radii[i] * R);
    }

    // Светлое пятно под знаком.
    QRadialGradient glow(c, 2.3 * R);
    glow.setColorAt(0.0, withAlpha(theme::palette().value, 40));
    glow.setColorAt(1.0, withAlpha(theme::palette().value, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(c, 2.3 * R, 2.3 * R);
}

// Риски вдоль боковых краёв карточки — как шкала пеленгов: мелкие через шаг, крупные через пять.
void drawEdgeScales(QPainter& painter, const QRectF& card)
{
    constexpr double kStep = 8.0;
    const double top = card.top() + 2 * kCardRadius;
    const double bottom = card.bottom() - 2 * kCardRadius;
    int index = 0;
    for (double y = top; y <= bottom; y += kStep, ++index)
    {
        const bool major = index % 5 == 0;
        const double length = major ? 9.0 : 4.0;
        painter.setPen(QPen(withAlpha(theme::palette().ppiScale, major ? 150 : 80), 1.0));
        painter.drawLine(QPointF(card.left() + 6, y), QPointF(card.left() + 6 + length, y));
        painter.drawLine(QPointF(card.right() - 6, y), QPointF(card.right() - 6 - length, y));
    }
}

// Несколько линий волн у нижнего края карточки.
void drawWaves(QPainter& painter, const QRectF& band)
{
    constexpr int kLayers = 3;
    for (int layer = 0; layer < kLayers; ++layer)
    {
        const double y = band.top() + band.height() * (layer + 1) / (kLayers + 1);
        const double amplitude = band.height() * 0.08;
        const double wavelength = band.width() / 3.0;
        const double phase = layer * wavelength / 3.0;
        QPainterPath wave;
        for (double x = 0; x <= band.width(); x += 2.0)
        {
            const QPointF point(band.left() + x, y + amplitude * std::sin((x + phase) / wavelength * 2 * std::numbers::pi));
            if (x == 0)
            {
                wave.moveTo(point);
            }
            else
            {
                wave.lineTo(point);
            }
        }
        painter.setPen(QPen(withAlpha(accent(), 110 - layer * 25), 1.2));
        painter.drawPath(wave);
    }
}

} // namespace

void drawDalrybvtuzEmblem(QPainter& painter, const QRectF& area)
{
    const theme::Palette& p = theme::palette();
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Карточка — во всю высоту бокового поля, по ширине не больше kCardMaxWidth.
    const double cardWidth = std::min(area.width() - 2 * kCardMargin, kCardMaxWidth);
    const QRectF card(area.center().x() - cardWidth / 2, area.top() + kCardMargin, cardWidth,
                      area.height() - 2 * kCardMargin);
    if (cardWidth < 80 || card.height() < cardWidth * 1.4)
    {
        painter.restore();
        return;
    }

    QPainterPath cardPath;
    cardPath.addRoundedRect(card, kCardRadius, kCardRadius);
    QLinearGradient background(card.topLeft(), card.bottomLeft());
    background.setColorAt(0.0, mix(p.background, p.text, 0.10));
    background.setColorAt(0.5, mix(p.background, p.text, 0.05));
    background.setColorAt(1.0, mix(p.background, p.text, 0.12));
    painter.setPen(QPen(withAlpha(p.frame, 150), 1.0));
    painter.setBrush(background);
    painter.drawPath(cardPath);

    painter.save();
    painter.setClipPath(cardPath);
    drawEdgeScales(painter, card);

    // Знак — в верхней трети, вокруг кольца сонара.
    const double R = cardWidth / 5.4;
    const QPointF markCenter(card.center().x(), card.top() + card.height() * 0.30);
    drawSonarRings(painter, markCenter, R);

    // Волны у нижнего края.
    const double wavesHeight = std::min(card.height() * 0.12, 4.0 * R);
    drawWaves(painter, QRectF(card.left(), card.bottom() - wavesHeight - card.height() * 0.08, card.width(), wavesHeight));
    painter.restore();

    drawMark(painter, markCenter, R, markLight(), markDark());

    // Надпись над знаком.
    QFont header = theme::font(std::max(8, static_cast<int>(cardWidth / 26.0)));
    header.setLetterSpacing(QFont::PercentageSpacing, 140);
    const double headerHeight = QFontMetricsF(header).height();
    painter.setFont(header);
    painter.setPen(withAlpha(accent(), 170));
    painter.drawText(QRectF(card.left(), card.top() + card.height() * 0.07, card.width(), headerHeight), Qt::AlignCenter,
                     QStringLiteral("ТРЕНАЖЁР СУДОВОЙ РЛС"));

    // Название, черта, полное наименование — под знаком.
    QFont wordmark = theme::font(std::max(10, static_cast<int>(cardWidth / 11.0)));
    wordmark.setWeight(QFont::DemiBold);
    wordmark.setLetterSpacing(QFont::PercentageSpacing, 116);
    QFont subtitle = theme::font(std::max(9, static_cast<int>(cardWidth / 23.0)));
    const double wordmarkHeight = QFontMetricsF(wordmark).height();
    const double subtitleLine = QFontMetricsF(subtitle).height();

    double y = markCenter.y() + 3.6 * R;
    QLinearGradient wordmarkFill(0, y, 0, y + wordmarkHeight);
    wordmarkFill.setColorAt(0.0, markLight());
    wordmarkFill.setColorAt(1.0, markDark());
    painter.setFont(wordmark);
    painter.setPen(QPen(QBrush(wordmarkFill), 1.0));
    painter.drawText(QRectF(card.left(), y, card.width(), wordmarkHeight), Qt::AlignCenter,
                     QStringLiteral("ДАЛЬРЫБВТУЗ"));
    y += wordmarkHeight + subtitleLine * 0.5;

    painter.setPen(QPen(withAlpha(p.frame, 170), 1.0));
    painter.drawLine(QPointF(card.center().x() - cardWidth * 0.2, y), QPointF(card.center().x() + cardWidth * 0.2, y));
    y += subtitleLine * 0.6;

    painter.setFont(subtitle);
    painter.setPen(withAlpha(accent(), 200));
    for (const QString& line : {QStringLiteral("Дальневосточный государственный"),
                                QStringLiteral("технический рыбохозяйственный"), QStringLiteral("университет")})
    {
        painter.drawText(QRectF(card.left(), y, card.width(), subtitleLine), Qt::AlignCenter, line);
        y += subtitleLine;
    }

    // Город — у нижнего края, под волнами.
    QFont city = theme::font(std::max(8, static_cast<int>(cardWidth / 26.0)));
    city.setLetterSpacing(QFont::PercentageSpacing, 160);
    const double cityHeight = QFontMetricsF(city).height();
    painter.setFont(city);
    painter.setPen(withAlpha(p.value, 150));
    painter.drawText(QRectF(card.left(), card.bottom() - card.height() * 0.05 - cityHeight, card.width(), cityHeight),
                     Qt::AlignCenter, QStringLiteral("ВЛАДИВОСТОК"));

    painter.restore();
}

void drawDalrybvtuzGridEmblem(QPainter& painter, const QRectF& area)
{
    const theme::Palette& p = theme::palette();
    // Без жёлтого: знак и надписи — в тонах текста схемы, вспомогательные линии — цвет шкалы ИКО.
    const QColor light = mix(p.text, QColor(255, 255, 255), 0.35);
    const QColor dark = mix(p.text, p.ppiScale, 0.35);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Сетка во всю высоту: мелкие клетки и каждая пятая линия — ярче.
    constexpr double kCell = 14.0;
    const QPointF origin(area.center().x(), area.top());
    for (int i = -static_cast<int>(area.width() / 2 / kCell); i * kCell <= area.width() / 2; ++i)
    {
        const double x = origin.x() + i * kCell;
        painter.setPen(QPen(withAlpha(p.text, i % 5 == 0 ? 42 : 20), 1.0));
        painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }
    for (int j = 0; j * kCell <= area.height(); ++j)
    {
        const double y = area.top() + j * kCell;
        painter.setPen(QPen(withAlpha(p.text, j % 5 == 0 ? 42 : 20), 1.0));
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    const double width = std::min(area.width() - 20.0, kCardMaxWidth);
    if (width < 80 || area.height() < width * 1.4)
    {
        painter.restore();
        return;
    }
    const double R = width / 5.4;
    const QPointF markCenter(area.center().x(), area.top() + area.height() * 0.32);

    // Перекрестие через знак — во всю ширину и высоту поля, с разрывом вокруг колец.
    const double gap = 3.5 * R;
    painter.setPen(QPen(withAlpha(p.ppiScale, 90), 1.0, Qt::DashLine));
    painter.drawLine(QPointF(markCenter.x(), area.top()), QPointF(markCenter.x(), markCenter.y() - gap));
    painter.drawLine(QPointF(markCenter.x(), markCenter.y() + gap), QPointF(markCenter.x(), area.bottom()));
    painter.drawLine(QPointF(area.left(), markCenter.y()), QPointF(markCenter.x() - gap, markCenter.y()));
    painter.drawLine(QPointF(markCenter.x() + gap, markCenter.y()), QPointF(area.right(), markCenter.y()));

    // Кольца с рисками, как у ИКО.
    painter.setBrush(Qt::NoBrush);
    for (const auto& [radius, alpha] : {std::pair{2.25, 70}, std::pair{2.9, 45}, std::pair{3.5, 28}})
    {
        painter.setPen(QPen(withAlpha(p.ppiScale, alpha), 1.0));
        painter.drawEllipse(markCenter, radius * R, radius * R);
    }
    for (int deg = 0; deg < 360; deg += 10)
    {
        const double angle = deg * std::numbers::pi / 180.0;
        const QPointF dir(std::sin(angle), -std::cos(angle));
        const double length = (deg % 30 == 0) ? 0.22 * R : 0.1 * R;
        painter.setPen(QPen(withAlpha(p.ppiScale, 110), 1.0));
        painter.drawLine(markCenter + dir * (3.5 * R), markCenter + dir * (3.5 * R - length));
    }

    QRadialGradient glow(markCenter, 2.3 * R);
    glow.setColorAt(0.0, withAlpha(p.text, 45));
    glow.setColorAt(1.0, withAlpha(p.text, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(markCenter, 2.3 * R, 2.3 * R);

    drawMark(painter, markCenter, R, light, dark);

    // Уголки-скобки по краям поля, как рамка прицела.
    const double bracket = std::min(area.width(), area.height()) * 0.08;
    const QRectF frame = area.adjusted(8, 8, -8, -8);
    painter.setPen(QPen(withAlpha(p.text, 130), 1.5));
    for (const QPointF& corner : {frame.topLeft(), frame.topRight(), frame.bottomLeft(), frame.bottomRight()})
    {
        const double sx = corner.x() < frame.center().x() ? 1.0 : -1.0;
        const double sy = corner.y() < frame.center().y() ? 1.0 : -1.0;
        painter.drawLine(corner, corner + QPointF(sx * bracket, 0));
        painter.drawLine(corner, corner + QPointF(0, sy * bracket));
    }

    // Надписи.
    QFont wordmark = theme::font(std::max(10, static_cast<int>(width / 11.0)));
    wordmark.setWeight(QFont::DemiBold);
    wordmark.setLetterSpacing(QFont::PercentageSpacing, 116);
    QFont subtitle = theme::font(std::max(9, static_cast<int>(width / 23.0)));
    QFont small = theme::font(std::max(8, static_cast<int>(width / 26.0)));
    small.setLetterSpacing(QFont::PercentageSpacing, 150);
    const double wordmarkHeight = QFontMetricsF(wordmark).height();
    const double subtitleLine = QFontMetricsF(subtitle).height();
    const double smallHeight = QFontMetricsF(small).height();

    // Подложка под текст, чтобы сетка не мешала читать.
    const auto textBand = [&](double top, double height) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(p.background, 200));
        painter.drawRect(QRectF(area.center().x() - width / 2, top, width, height));
    };

    double y = markCenter.y() + 3.85 * R;
    const double blockHeight = wordmarkHeight + subtitleLine * 1.1 + 3 * subtitleLine;
    textBand(y - subtitleLine * 0.3, blockHeight + subtitleLine * 0.6);

    QLinearGradient wordmarkFill(0, y, 0, y + wordmarkHeight);
    wordmarkFill.setColorAt(0.0, light);
    wordmarkFill.setColorAt(1.0, dark);
    painter.setFont(wordmark);
    painter.setPen(QPen(QBrush(wordmarkFill), 1.0));
    painter.drawText(QRectF(area.left(), y, area.width(), wordmarkHeight), Qt::AlignCenter, QStringLiteral("ДАЛЬРЫБВТУЗ"));
    y += wordmarkHeight + subtitleLine * 0.5;

    painter.setPen(QPen(withAlpha(p.ppiScale, 160), 1.0));
    painter.drawLine(QPointF(area.center().x() - width * 0.2, y), QPointF(area.center().x() + width * 0.2, y));
    y += subtitleLine * 0.6;

    painter.setFont(subtitle);
    painter.setPen(withAlpha(p.text, 210));
    for (const QString& line : {QStringLiteral("Дальневосточный государственный"),
                                QStringLiteral("технический рыбохозяйственный"), QStringLiteral("университет")})
    {
        painter.drawText(QRectF(area.left(), y, area.width(), subtitleLine), Qt::AlignCenter, line);
        y += subtitleLine;
    }

    // Внизу — город и его координаты, как отсчёт курсора.
    const double bottomTop = area.bottom() - area.height() * 0.07 - 2 * smallHeight;
    textBand(bottomTop - 4, 2 * smallHeight + 8);
    painter.setFont(small);
    painter.setPen(withAlpha(p.text, 190));
    painter.drawText(QRectF(area.left(), bottomTop, area.width(), smallHeight), Qt::AlignCenter, QStringLiteral("ВЛАДИВОСТОК"));
    painter.setPen(withAlpha(p.ppiScale, 190));
    painter.drawText(QRectF(area.left(), bottomTop + smallHeight, area.width(), smallHeight), Qt::AlignCenter,
                     QStringLiteral("43°07′ С.Ш.   131°53′ В.Д."));

    painter.restore();
}

} // namespace mrs::ui
