#include "level_control.h"

#include "presentation/theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>

namespace mrs::ui
{

namespace
{

constexpr double kWheelStep = 0.05;

} // namespace

LevelBar::LevelBar(QWidget* parent)
    : QFrame(parent)
{
    setFixedWidth(LevelControl::kBarWidth);
    setCursor(Qt::PointingHandCursor);
}

void LevelBar::setValue(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (value == value_)
    {
        return;
    }

    value_ = value;
    update();
    emit valueChanged(value_);
}

void LevelBar::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    const QRect inner = rect();
    const int fillWidth = static_cast<int>(inner.width() * value_ + 0.5);
    if (fillWidth <= 0)
    {
        return;
    }

    QPainter painter(this);
    painter.fillRect(QRect(inner.left(), inner.top(), fillWidth, inner.height()), theme::kIndicator);
}

void LevelBar::wheelEvent(QWheelEvent* event)
{
    const int steps = event->angleDelta().y() / 120;
    setValue(value_ + steps * kWheelStep);
    event->accept();
}

void LevelBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        setValueFromX(event->position().toPoint().x());
    }
}

void LevelBar::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        setValueFromX(event->position().toPoint().x());
    }
}

void LevelBar::setValueFromX(int x)
{
    setValue(static_cast<double>(x) / width());
}

LevelControl::LevelControl(const QString& title, double initialValue, QWidget* parent)
    : QFrame(parent)
    , bar_(new LevelBar(this))
{
    setProperty("panel", true);

    // Внутри рамки толщиной 1 px: название │ перегородка │ шкала.
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);

    auto* titleLabel = theme::createText(title, this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* divider = new QFrame(this);
    divider->setFixedWidth(kDividerWidth);
    divider->setStyleSheet(QStringLiteral("background: %1;").arg(theme::kFrame.name()));

    layout->addWidget(titleLabel, 1);
    layout->addWidget(divider);
    layout->addWidget(bar_);

    setFixedHeight(theme::kBlockHeight);

    bar_->setValue(initialValue);
    connect(bar_, &LevelBar::valueChanged, this, &LevelControl::valueChanged);
}

double LevelControl::value() const noexcept
{
    return bar_->value();
}

void LevelControl::setValue(double value)
{
    bar_->setValue(value);
}

} // namespace mrs::ui
