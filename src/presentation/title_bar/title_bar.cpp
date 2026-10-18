#include "title_bar.h"

#include "presentation/theme/theme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>

#include <cmath>
#include <numbers>

namespace mrs::ui
{

namespace
{

constexpr int kTitleBarHeight = 32;
constexpr int kButtonWidth = 44;
const QColor kCloseHover{200, 30, 30};

void drawGear(QPainter& painter, const QPointF& center, double radius)
{
    constexpr int kTeeth = 8;
    const double outer = radius;
    const double inner = radius * 0.72;

    QPainterPath gear;
    for (int i = 0; i < kTeeth * 2; ++i)
    {
        // Каждый зуб — две точки на внешнем радиусе, впадина — две на внутреннем.
        const double r = (i % 2 == 0) ? outer : inner;
        for (const double offset : {-0.22, 0.22})
        {
            const double angle = (i + 0.5 + offset) * std::numbers::pi / kTeeth;
            const QPointF point(center.x() + r * std::cos(angle), center.y() + r * std::sin(angle));
            if (gear.elementCount() == 0)
            {
                gear.moveTo(point);
            }
            else
            {
                gear.lineTo(point);
            }
        }
    }
    gear.closeSubpath();
    gear.addEllipse(center, radius * 0.3, radius * 0.3);

    painter.drawPath(gear);
}

} // namespace

TitleBarButton::TitleBarButton(Kind kind, QWidget* parent)
    : QAbstractButton(parent)
    , kind_(kind)
{
    setFixedSize(kButtonWidth, kTitleBarHeight);
    setFocusPolicy(Qt::NoFocus);
}

void TitleBarButton::setKind(Kind kind)
{
    kind_ = kind;
    update();
}

void TitleBarButton::setActive(bool active)
{
    active_ = active;
    update();
}

void TitleBarButton::enterEvent(QEnterEvent* event)
{
    hovered_ = true;
    update();
    QAbstractButton::enterEvent(event);
}

void TitleBarButton::leaveEvent(QEvent* event)
{
    hovered_ = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void TitleBarButton::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (hovered_)
    {
        painter.fillRect(rect(), kind_ == Kind::Close ? kCloseHover : theme::palette().hover);
    }

    painter.setPen(QPen(active_ ? theme::palette().value : theme::palette().text, 1.4));
    painter.setBrush(Qt::NoBrush);

    const QPointF c = QRectF(rect()).center();
    constexpr double h = 5.5; // половина размера иконки

    switch (kind_)
    {
    case Kind::Settings:
        drawGear(painter, c, h + 1.5);
        break;
    case Kind::Minimize:
        painter.drawLine(QPointF(c.x() - h, c.y()), QPointF(c.x() + h, c.y()));
        break;
    case Kind::Maximize:
        painter.drawRect(QRectF(c.x() - h, c.y() - h, 2 * h, 2 * h));
        break;
    case Kind::Restore:
        painter.drawRect(QRectF(c.x() - h, c.y() - h + 2.5, 2 * h - 2.5, 2 * h - 2.5));
        painter.drawPolyline(QPolygonF{
            {c.x() - h + 2.5, c.y() - h + 2.5},
            {c.x() - h + 2.5, c.y() - h},
            {c.x() + h, c.y() - h},
            {c.x() + h, c.y() + h - 2.5},
            {c.x() + h - 2.5, c.y() + h - 2.5},
        });
        break;
    case Kind::Close:
        painter.drawLine(QPointF(c.x() - h, c.y() - h), QPointF(c.x() + h, c.y() + h));
        painter.drawLine(QPointF(c.x() - h, c.y() + h), QPointF(c.x() + h, c.y() - h));
        break;
    }
}

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
    , settingsButton_(new TitleBarButton(TitleBarButton::Kind::Settings, this))
    , maximizeButton_(new TitleBarButton(TitleBarButton::Kind::Maximize, this))
{
    setFixedHeight(kTitleBarHeight);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(theme::kPadding, 0, 0, 0);
    layout->setSpacing(0);

    auto* title = theme::createText(QStringLiteral("MarineRadarSim"), this);
    layout->addWidget(title);
    layout->addStretch();

    settingsButton_->setToolTip(tr("Настройки (F10)"));
    auto* minimizeButton = new TitleBarButton(TitleBarButton::Kind::Minimize, this);
    minimizeButton->setToolTip(tr("Свернуть"));
    auto* closeButton = new TitleBarButton(TitleBarButton::Kind::Close, this);
    closeButton->setToolTip(tr("Закрыть"));

    layout->addWidget(settingsButton_);
    layout->addWidget(minimizeButton);
    layout->addWidget(maximizeButton_);
    layout->addWidget(closeButton);

    connect(settingsButton_, &QAbstractButton::clicked, this, &TitleBar::settingsRequested);
    connect(minimizeButton, &QAbstractButton::clicked, this, [this] { window()->showMinimized(); });
    connect(maximizeButton_, &QAbstractButton::clicked, this, &TitleBar::toggleMaximized);
    connect(closeButton, &QAbstractButton::clicked, this, [this] { window()->close(); });

    window()->installEventFilter(this);
    updateMaximizeButton();
}

void TitleBar::setSettingsActive(bool active)
{
    settingsButton_->setActive(active);
}

void TitleBar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::palette().background);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && window()->windowHandle() != nullptr)
    {
        window()->windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        toggleMaximized();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool TitleBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == window() && event->type() == QEvent::WindowStateChange)
    {
        updateMaximizeButton();
    }
    return QWidget::eventFilter(watched, event);
}

void TitleBar::toggleMaximized()
{
    if (window()->isMaximized())
    {
        window()->showNormal();
    }
    else
    {
        window()->showMaximized();
    }
}

void TitleBar::updateMaximizeButton()
{
    const bool maximized = window()->isMaximized();
    maximizeButton_->setKind(maximized ? TitleBarButton::Kind::Restore : TitleBarButton::Kind::Maximize);
    maximizeButton_->setToolTip(maximized ? tr("Восстановить") : tr("Развернуть"));
}

} // namespace mrs::ui
