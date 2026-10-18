#pragma once

#include <QColor>
#include <QString>

class QFrame;
class QLabel;
class QWidget;

namespace mrs::ui::theme
{

// Цветовая схема рабочего места оператора.
inline const QColor kBackground{0, 0, 0};
inline const QColor kFrame{230, 200, 0};      // рамки блоков — жёлтые
inline const QColor kText{60, 140, 255};      // надписи — синие
inline const QColor kValue{255, 224, 0};      // цифры — жёлтые
inline const QColor kPpiOutline{0, 200, 0};   // шкала ИКО — зелёная
inline const QColor kHeadingLine{220, 230, 255};
inline const QColor kIndicator{0, 200, 0};    // заполнение шкал регуляторов
inline const QColor kHover{30, 30, 30};

// Единая сетка размеров: все отступы и высоты блоков берутся отсюда.
inline constexpr int kSpacing = 2;          // между блоками
inline constexpr int kStackSpacing = kSpacing; // в угловых стопках — тот же шаг, чтобы строки совпадали с колонкой
inline constexpr int kPadding = 8;          // от рамки до текста внутри блока
inline constexpr int kBlockHeight = 28;     // однострочный блок
inline constexpr int kFontPx = 14;

QString styleSheet();

// Блок в жёлтой рамке.
QFrame* createPanel(QWidget* parent);

// Синяя надпись.
QLabel* createText(const QString& text, QWidget* parent);

// Жёлтое числовое значение.
QLabel* createValue(const QString& text, QWidget* parent);

// Фрагмент rich text с цветом значения — для надписей, где цифры стоят внутри текста.
QString valueSpan(const QString& text);

} // namespace mrs::ui::theme
