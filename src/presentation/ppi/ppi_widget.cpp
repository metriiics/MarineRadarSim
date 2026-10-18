#include "ppi_widget.h"

#include "presentation/theme/theme.h"

#include <QPainter>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace mrs::ui
{

namespace
{

constexpr double kMargin = 2.0;            // от края виджета до подписей шкалы
constexpr double kMinorTickLength = 4.0;   // риска через 1° — наружу от рабочей области
constexpr double kMidTickLength = 6.0;     // риска через 5° — без подписи
constexpr double kMajorTickLength = 9.0;   // риска через 10°
constexpr double kLabelOffset = 19.0;      // от края рабочей области до центра подписи
constexpr double kLabelSpace = 28.0;       // кольцо под риски и подписи снаружи рабочей области
constexpr double kReservedGap = 2.0;       // зазор между подписями и наложенными блоками
constexpr int kLabelStepDeg = 10;
constexpr int kMidTickStepDeg = 5;
constexpr int kLabelFontPx = 13;

double distanceToRect(const QPointF& point, const QRectF& rect)
{
    const double dx = std::max({rect.left() - point.x(), 0.0, point.x() - rect.right()});
    const double dy = std::max({rect.top() - point.y(), 0.0, point.y() - rect.bottom()});
    return std::hypot(dx, dy);
}

// Направление пеленга на экране: 0° — вверх, отсчёт по часовой стрелке.
QPointF bearingDirection(double degrees)
{
    const double radians = degrees * std::numbers::pi / 180.0;
    return {std::sin(radians), -std::cos(radians)};
}

} // namespace

PpiWidget::PpiWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 400);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PpiWidget::setReservedAreas(const QList<QWidget*>& widgets)
{
    reservedAreas_.clear();
    for (QWidget* widget : widgets)
    {
        reservedAreas_.append(widget);
    }
    update();
}

void PpiWidget::setAlignment(Qt::Alignment alignment)
{
    alignment_ = alignment;
    update();
}

QPointF PpiWidget::center() const
{
    const double half = std::min(width(), height()) / 2.0;
    double x = width() / 2.0;
    if (alignment_ & Qt::AlignLeft)
    {
        x = half;
    }
    else if (alignment_ & Qt::AlignRight)
    {
        x = width() - half;
    }
    return {x, height() / 2.0};
}

double PpiWidget::outerRadius() const
{
    // Внешняя граница подписей шкалы.
    return std::min(width(), height()) / 2.0 - kMargin;
}

QPointF PpiWidget::bearingLabelCenter(double degrees) const
{
    return labelRectAt(degrees, nominalRadius()).center();
}

QFont PpiWidget::labelFont() const
{
    QFont font = this->font();
    font.setPixelSize(kLabelFontPx);
    return font;
}

QRectF PpiWidget::bearingLabelRect(double degrees) const
{
    return labelRectAt(degrees, nominalRadius());
}

double PpiWidget::nominalRadius() const
{
    return std::max(0.0, outerRadius() - kLabelSpace);
}

QRectF PpiWidget::labelRectAt(double degrees, double ringRadius) const
{
    const QFontMetricsF metrics(labelFont());
    const QString text = QString::number(static_cast<int>(degrees));
    const QSizeF size(metrics.horizontalAdvance(text), metrics.height());
    const QPointF anchor = center() + bearingDirection(degrees) * (ringRadius + kLabelOffset);
    return {anchor - QPointF(size.width() / 2, size.height() / 2), size};
}

QRectF PpiWidget::bearingLabelDigitRect(double degrees, int index) const
{
    const QFontMetricsF metrics(labelFont());
    const QString text = QString::number(static_cast<int>(degrees));
    const QRectF label = bearingLabelRect(degrees);
    const double left = label.left() + metrics.horizontalAdvance(text.left(index));
    const double width = metrics.horizontalAdvance(text.mid(index, 1));
    return {left, label.top(), width, label.height()};
}

double PpiWidget::radius() const
{
    // Круг уступает место наложенным блокам, только если блок задевает риски шкалы или
    // прямоугольник какой-нибудь цифры. Блок над цифрой (а не на ней) круг не уменьшает —
    // так угловые блоки можно подводить к цифрам вплотную.
    const QPointF c = center();
    double r = nominalRadius();

    QList<QRectF> areas;
    for (const auto& area : reservedAreas_)
    {
        if (area.isNull() || !area->isVisible())
        {
            continue;
        }
        const QRectF areaRect(mapFromGlobal(area->mapToGlobal(QPoint(0, 0))), area->size());
        areas.append(areaRect);
        r = std::min(r, distanceToRect(c, areaRect) - kMajorTickLength - 1.0 - kReservedGap);
    }

    constexpr int kMaxShrinkSteps = 200;
    for (int step = 0; step < kMaxShrinkSteps && r > 0.0; ++step)
    {
        bool overlaps = false;
        for (int deg = 0; deg < 360 && !overlaps; deg += kLabelStepDeg)
        {
            const QRectF label = labelRectAt(deg, r);
            for (const QRectF& area : areas)
            {
                if (label.intersects(area))
                {
                    overlaps = true;
                    break;
                }
            }
        }
        if (!overlaps)
        {
            break;
        }
        r -= 1.0;
    }

    return std::max(0.0, r);
}

void PpiWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), theme::kBackground);

    drawBearingScale(painter);
    drawHeadingLine(painter);
}

void PpiWidget::drawHeadingLine(QPainter& painter) const
{
    // Отметка курса: при ориентации «Нос» — всегда на 0°.
    const QPointF c = center();
    painter.setPen(QPen(theme::kHeadingLine, 1.5));
    painter.drawLine(c, c + bearingDirection(0.0) * radius());
}

void PpiWidget::drawBearingScale(QPainter& painter) const
{
    const QPointF c = center();
    const double r = radius();
    if (r <= 0.0)
    {
        return;
    }

    // Окружность не рисуется сплошной линией — её образуют риски шкалы снаружи рабочей области.
    QPen minorPen(theme::kPpiOutline, 1.0);
    QPen majorPen(theme::kPpiOutline, 2.0);
    minorPen.setCapStyle(Qt::FlatCap);
    majorPen.setCapStyle(Qt::FlatCap);

    for (int deg = 0; deg < 360; ++deg)
    {
        const bool major = deg % kLabelStepDeg == 0;
        const bool mid = !major && deg % kMidTickStepDeg == 0;
        const QPointF dir = bearingDirection(deg);
        const double length = major ? kMajorTickLength : (mid ? kMidTickLength : kMinorTickLength);

        painter.setPen(major ? majorPen : minorPen);
        painter.drawLine(c + dir * (r + 1.0), c + dir * (r + 1.0 + length));
    }

    painter.setFont(labelFont());
    painter.setPen(theme::kValue);

    for (int deg = 0; deg < 360; deg += kLabelStepDeg)
    {
        painter.drawText(labelRectAt(deg, r), Qt::AlignCenter, QString::number(deg));
    }
}

} // namespace mrs::ui
