#pragma once

#include <QFrame>
#include <QWidget>

class QLabel;

namespace mrs::ui
{

// Шкала уровня регулятора: зелёное заполнение (рамку даёт LevelControl).
class LevelBar final : public QFrame
{
    Q_OBJECT

public:
    explicit LevelBar(QWidget* parent = nullptr);

    [[nodiscard]] double value() const noexcept { return value_; }
    void setValue(double value);

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void setValueFromX(int x);

    double value_{0.0};
};

// Регулятор оператора — один блок в рамке: название │ шкала уровня (УСИЛЕНИЕ, ВОЛНЫ, ДОЖДЬ, АПЧ).
class LevelControl final : public QFrame
{
    Q_OBJECT

public:
    static constexpr int kBarWidth = 60;
    static constexpr int kDividerWidth = 1;  // перегородка между названием и шкалой

    // Расстояние от левого края регулятора до перегородки включительно —
    // блок такой ширины под регулятором выравнивается правой границей по перегородке.
    static constexpr int titleWidth(int controlWidth) { return controlWidth - kBarWidth - 1; }

    LevelControl(const QString& title, double initialValue, QWidget* parent = nullptr);

    [[nodiscard]] double value() const noexcept;
    void setValue(double value);

signals:
    void valueChanged(double value);

private:
    LevelBar* bar_;
};

} // namespace mrs::ui
