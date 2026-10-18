#include "clock_icon.h"

#include "presentation/theme/theme.h"

#include <QPainter>

namespace mrs::ui
{

namespace
{

constexpr int kIconSize = 16;

} // namespace

ClockIcon::ClockIcon(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(kIconSize, kIconSize);
}

void ClockIcon::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(theme::kText, 1.4);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    const QRectF face = QRectF(rect()).adjusted(1, 1, -1, -1);
    const QPointF c = face.center();
    const double r = face.width() / 2;
    painter.drawEllipse(face);

    // Минутная стрелка — на 12, часовая — на 3.
    painter.drawLine(c, c + QPointF(0, -r * 0.7));
    painter.drawLine(c, c + QPointF(r * 0.5, 0));
}

} // namespace mrs::ui
