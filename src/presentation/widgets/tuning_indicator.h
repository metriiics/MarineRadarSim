#pragma once

#include <QFrame>

namespace mrs::ui
{

// Индикатор настройки приёмника: ступенчатая кривая в жёлтой рамке.
// Высота ступени отражает качество настройки (0 — расстроен, 1 — настроен).
class TuningIndicator final : public QFrame
{
public:
    explicit TuningIndicator(QWidget* parent = nullptr);

    void setLevel(double level);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double level_{0.5};
};

} // namespace mrs::ui
