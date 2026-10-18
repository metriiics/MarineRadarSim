#include "theme.h"

#include <QFrame>
#include <QLabel>

namespace mrs::ui::theme
{

QString styleSheet()
{
    return QStringLiteral(
               "QMainWindow, QDialog, QStackedWidget, #centralWidget { background: %1; }"
               "QLabel { color: %2; background: transparent; font-size: %6px; }"
               "QLabel[role=\"value\"] { color: %3; }"
               "QFrame[panel=\"true\"] { background: %1; border: 1px solid %4; }"

               "QPushButton { color: %2; background: %1; border: 1px solid %4;"
               "  font-size: %6px; padding: 6px 20px; }"
               "QPushButton:hover { background: %5; }"

               "QCheckBox { color: %2; font-size: %6px; spacing: 10px; }"
               "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %4; background: %1; }"
               "QCheckBox::indicator:checked { background: %7; }"

               "QListWidget { background: %1; border: 1px solid %4; outline: none;"
               "  font-size: %6px; padding: 4px 0; }"
               // Отступ повторён в каждом состоянии: иначе Qt сбрасывает его у невыбранных пунктов.
               "QListWidget::item { color: %2; padding: 8px 14px; border-left: 3px solid %1; }"
               "QListWidget::item:hover { color: %2; padding: 8px 14px; background: %5; }"
               "QListWidget::item:selected { color: %3; padding: 8px 14px; background: %5;"
               "  border-left: 3px solid %4; }")
        .arg(kBackground.name(), kText.name(), kValue.name(), kFrame.name(), kHover.name(),
             QString::number(kFontPx), kIndicator.name());
}

QFrame* createPanel(QWidget* parent)
{
    auto* panel = new QFrame(parent);
    panel->setProperty("panel", true);
    return panel;
}

QLabel* createText(const QString& text, QWidget* parent)
{
    return new QLabel(text, parent);
}

QLabel* createValue(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setProperty("role", "value");
    return label;
}

QString valueSpan(const QString& text)
{
    return QStringLiteral("<span style=\"color:%1\">%2</span>").arg(kValue.name(), text.toHtmlEscaped());
}

} // namespace mrs::ui::theme
