#include "cursor_style.h"

#include <QColor>
#include <QPainter>

namespace mrs::ui
{

namespace
{

constexpr int kCursorSize = 33;  // нечётный — центр попадает в пиксель

} // namespace

QString cursorStyleName(CursorStyle style)
{
    switch (style)
    {
    case CursorStyle::Sight:
        return QStringLiteral("Прицел");
    case CursorStyle::Crosshair:
        return QStringLiteral("Перекрестие");
    case CursorStyle::Circle:
        return QStringLiteral("Окружность с точкой");
    case CursorStyle::Arrow:
        return QStringLiteral("Стрелка");
    }
    return {};
}

QList<CursorStyle> allCursorStyles()
{
    return {CursorStyle::Sight, CursorStyle::Crosshair, CursorStyle::Circle, CursorStyle::Arrow};
}

QPixmap cursorPixmap(CursorStyle style, const QColor& color)
{
    if (style == CursorStyle::Arrow)
    {
        return {};
    }

    QPixmap pixmap(kCursorSize, kCursorSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.2));

    const QPointF c(kCursorSize / 2.0, kCursorSize / 2.0);
    const double r = kCursorSize / 2.0 - 1.5;

    switch (style)
    {
    case CursorStyle::Sight:
    {
        // Окружность, четыре риски внутрь с разрывом у центра и точка в центре.
        const double ring = r * 0.72;
        painter.drawEllipse(c, ring, ring);
        constexpr double gap = 4.0;
        painter.drawLine(QPointF(c.x(), c.y() - r), QPointF(c.x(), c.y() - gap));
        painter.drawLine(QPointF(c.x(), c.y() + gap), QPointF(c.x(), c.y() + r));
        painter.drawLine(QPointF(c.x() - r, c.y()), QPointF(c.x() - gap, c.y()));
        painter.drawLine(QPointF(c.x() + gap, c.y()), QPointF(c.x() + r, c.y()));
        painter.setBrush(color);
        painter.drawEllipse(c, 1.2, 1.2);
        break;
    }
    case CursorStyle::Crosshair:
        painter.drawLine(QPointF(c.x(), c.y() - r), QPointF(c.x(), c.y() + r));
        painter.drawLine(QPointF(c.x() - r, c.y()), QPointF(c.x() + r, c.y()));
        break;
    case CursorStyle::Circle:
        painter.drawEllipse(c, r * 0.5, r * 0.5);
        painter.setBrush(color);
        painter.drawEllipse(c, 1.5, 1.5);
        break;
    case CursorStyle::Arrow:
        break;
    }

    return pixmap;
}

QCursor makeCursor(CursorStyle style, const QColor& color)
{
    if (style == CursorStyle::Arrow)
    {
        return QCursor(Qt::ArrowCursor);
    }
    return QCursor(cursorPixmap(style, color), kCursorSize / 2, kCursorSize / 2);
}

} // namespace mrs::ui
