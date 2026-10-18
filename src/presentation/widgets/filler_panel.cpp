#include "filler_panel.h"

#include "presentation/theme/theme.h"
#include "presentation/widgets/emblem.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace mrs::ui
{

namespace
{

constexpr double kInset = 8.0;     // от рамки до рисунка
constexpr int kGridStep = 14;
constexpr int kFillerAlpha = 110;  // оформление приглушено, чтобы не спорить с данными

QColor muted(QColor color)
{
    color.setAlpha(kFillerAlpha);
    return color;
}

// Файл логотипа загружается один раз; пустой QImage — файла нет.
const QImage& logoImage()
{
    static const QImage image = [] {
        const QString appDir = QCoreApplication::applicationDirPath();
        for (const QString& dir : {appDir + QStringLiteral("/data/branding"), appDir + QStringLiteral("/../data/branding")})
        {
            for (const char* name : {"dalrybvtuz.png", "dalrybvtuz.svg"})
            {
                const QString path = dir + QLatin1Char('/') + QString::fromLatin1(name);
                if (QFileInfo::exists(path))
                {
                    QImage loaded(path);
                    if (!loaded.isNull())
                    {
                        return loaded;
                    }
                }
            }
        }
        return QImage();
    }();
    return image;
}

} // namespace

QString fillerStyleName(FillerStyle style)
{
    switch (style)
    {
    case FillerStyle::Logo:
        return QStringLiteral("Логотип Дальрыбвтуза (карточка)");
    case FillerStyle::LogoGrid:
        return QStringLiteral("Логотип Дальрыбвтуза (сетка)");
    case FillerStyle::CompassRose:
        return QStringLiteral("Роза ветров");
    case FillerStyle::Grid:
        return QStringLiteral("Сетка");
    case FillerStyle::Title:
        return QStringLiteral("Название тренажёра");
    case FillerStyle::None:
        return QStringLiteral("Пусто");
    }
    return {};
}

QList<FillerStyle> allFillerStyles()
{
    return {FillerStyle::Logo, FillerStyle::LogoGrid, FillerStyle::CompassRose, FillerStyle::Grid, FillerStyle::Title, FillerStyle::None};
}

FillerPanel::FillerPanel(QWidget* parent, Frame frame)
    : QFrame(parent)
{
    if (frame == Frame::Panel)
    {
        setProperty("panel", true);
    }
}

void FillerPanel::setFillerStyle(FillerStyle style)
{
    style_ = style;
    update();
}

void FillerPanel::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    const QRectF area = QRectF(rect()).adjusted(kInset, kInset, -kInset, -kInset);
    if (area.width() <= 0 || area.height() <= 0)
    {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(rect().adjusted(1, 1, -1, -1));

    switch (style_)
    {
    case FillerStyle::Logo:
        drawLogo(painter, area);
        break;
    case FillerStyle::LogoGrid:
        drawLogoGrid(painter, QRectF(rect()));  // сетка — до самых краёв поля
        break;
    case FillerStyle::CompassRose:
        drawCompassRose(painter, area);
        break;
    case FillerStyle::Grid:
        drawGrid(painter, area);
        break;
    case FillerStyle::Title:
        drawTitle(painter, area);
        break;
    case FillerStyle::None:
        break;
    }
}

void FillerPanel::drawLogo(QPainter& painter, const QRectF& area) const
{
    const QImage& image = logoImage();
    if (!image.isNull())
    {
        const QSizeF size = QSizeF(image.size()).scaled(area.size(), Qt::KeepAspectRatio);
        const QRectF target(area.center() - QPointF(size.width() / 2, size.height() / 2), size);
        painter.setOpacity(0.85);
        painter.drawImage(target, image);
        return;
    }

    drawDalrybvtuzEmblem(painter, area);
}

void FillerPanel::drawLogoGrid(QPainter& painter, const QRectF& area) const
{
    if (!logoImage().isNull())
    {
        drawGrid(painter, area);
        drawLogo(painter, area);  // официальный файл поверх сетки
        return;
    }
    drawDalrybvtuzGridEmblem(painter, area);
}

void FillerPanel::drawCompassRose(QPainter& painter, const QRectF& area) const
{
    const theme::Palette& p = theme::palette();
    // Над розой — место под букву «N»: роза смещена вниз на половину высоты буквы.
    const QFont labelFont = theme::font(std::max(10, static_cast<int>(std::min(area.width(), area.height()) * 0.08)));
    const double labelHeight = QFontMetricsF(labelFont).height();
    const double r = std::min(area.width(), area.height() - labelHeight) / 2 - 2;
    const QPointF c = area.center() + QPointF(0, labelHeight / 2);

    painter.setPen(QPen(muted(p.ppiScale), 1.0));
    painter.drawEllipse(c, r * 0.82, r * 0.82);

    // Восемь лучей: основные длиннее промежуточных.
    for (int i = 0; i < 8; ++i)
    {
        const double angle = i * std::numbers::pi / 4;
        const double length = (i % 2 == 0) ? r : r * 0.55;
        const double side = angle + std::numbers::pi / 2;
        const QPointF tip = c + QPointF(std::sin(angle), -std::cos(angle)) * length;
        const QPointF wing = QPointF(std::sin(side), -std::cos(side)) * (r * 0.09);
        painter.setBrush(i == 0 ? muted(p.value) : Qt::NoBrush);
        painter.drawPolygon(QPolygonF{c + wing, tip, c - wing});
    }

    if (r > 30)
    {
        painter.setFont(labelFont);
        painter.setPen(muted(p.value));
        painter.drawText(QRectF(c.x() - labelHeight, c.y() - r - labelHeight, 2 * labelHeight, labelHeight),
                         Qt::AlignCenter, QStringLiteral("N"));
    }
}

void FillerPanel::drawGrid(QPainter& painter, const QRectF& area) const
{
    QColor color = theme::palette().frame;
    color.setAlpha(45);
    painter.setPen(QPen(color, 1.0));
    for (double x = area.left(); x <= area.right(); x += kGridStep)
    {
        painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }
    for (double y = area.top(); y <= area.bottom(); y += kGridStep)
    {
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }
}

void FillerPanel::drawTitle(QPainter& painter, const QRectF& area) const
{
    const theme::Palette& p = theme::palette();
    QFont font = theme::font(std::max(11, static_cast<int>(std::min(area.height() / 4.0, 18.0))));
    font.setLetterSpacing(QFont::PercentageSpacing, 115);
    painter.setFont(font);
    painter.setPen(muted(p.text));
    painter.drawText(area, Qt::AlignCenter | Qt::TextWordWrap, QStringLiteral("ТРЕНАЖЁР\nСУДОВОЙ РЛС"));
}

} // namespace mrs::ui
