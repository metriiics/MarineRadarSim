#pragma once

#include <QWidget>

namespace mrs::ui
{

// Значок часов перед временем UTC: окружность со стрелками.
class ClockIcon final : public QWidget
{
public:
    explicit ClockIcon(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace mrs::ui
