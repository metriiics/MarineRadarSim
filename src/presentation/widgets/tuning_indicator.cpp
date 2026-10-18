#include "tuning_indicator.h"

#include "presentation/theme/theme.h"

#include <QPainter>

#include <algorithm>

namespace mrs::ui
{

TuningIndicator::TuningIndicator(QWidget* parent)
    : QFrame(parent)
{
    setProperty("panel", true);
}

void TuningIndicator::setLevel(double level)
{
    level_ = std::clamp(level, 0.0, 1.0);
    update();
}

void TuningIndicator::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    const QRectF area = QRectF(rect()).adjusted(theme::kPadding, 5, -theme::kPadding, -5);
    const double baseY = area.bottom();
    const double stepY = baseY - area.height() * level_;
    const double stepLeft = area.left() + area.width() * 0.25;
    const double stepRight = area.left() + area.width() * 0.5;

    const QPointF points[] = {
        {area.left(), baseY},
        {stepLeft, baseY},
        {stepLeft, stepY},
        {stepRight, stepY},
        {stepRight, baseY},
        {area.right(), baseY},
    };

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(theme::palette().indicator, 1.5));
    painter.drawPolyline(points, std::size(points));
}

} // namespace mrs::ui
