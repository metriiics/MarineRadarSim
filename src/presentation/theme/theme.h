#pragma once

#include <QColor>
#include <QFont>
#include <QList>
#include <QString>

class QFrame;
class QLabel;
class QWidget;

namespace mrs::ui::theme
{

// Цвета рабочего места. Текущую палитру задаёт выбранная цветовая схема.
struct Palette
{
    QColor background;   // фон, в том числе внутри круга ИКО
    QColor frame;        // рамки блоков
    QColor text;         // надписи и единицы измерения
    QColor value;        // числовые значения
    QColor ppiScale;     // риски шкалы пеленгов
    QColor indicator;    // заполнение шкал регуляторов, индикатор настройки
    QColor hover;        // подсветка под курсором
    QColor headingLine;  // линия курса
    QColor cursor;       // курсор на ИКО
};

enum class ColorScheme
{
    Soft,       // мягкая (по умолчанию): приглушённые тона исходной схемы
    Classic,    // контрастная: исходная схема эскиза
    Night,      // ночная: тёмно-красные тона, не слепит на ночной вахте
    Phosphor,   // зелёный экран, как у ЭЛТ-индикаторов
    Steel,      // стальная: серо-голубые тона
};

enum class FontChoice
{
    System,         // шрифт системы по умолчанию
    TimesNewRoman,
    Arial,
    Rounded,        // скруглённый (Nunito, если установлен или лежит в data/fonts)
};

// Единая сетка размеров: все отступы и высоты блоков берутся отсюда.
inline constexpr int kSpacing = 2;             // между блоками
inline constexpr int kStackSpacing = kSpacing; // в угловых стопках — тот же шаг, чтобы строки совпадали с колонкой
inline constexpr int kPadding = 8;             // от рамки до текста внутри блока
inline constexpr int kBlockHeight = 28;        // однострочный блок
inline constexpr int kFontPx = 14;

[[nodiscard]] const Palette& palette();
void setColorScheme(ColorScheme scheme);
[[nodiscard]] Palette schemePalette(ColorScheme scheme);
[[nodiscard]] QString schemeName(ColorScheme scheme);
[[nodiscard]] QList<ColorScheme> allSchemes();

// Регистрирует замены шрифтов (Times New Roman → Liberation Serif и т. п.) и шрифты из data/fonts.
// Вызывается один раз при запуске, после создания QApplication.
void initFonts();
void setFontChoice(FontChoice choice);
[[nodiscard]] QString fontName(FontChoice choice);
[[nodiscard]] QList<FontChoice> allFonts();
[[nodiscard]] QString fontFamily(FontChoice choice);
[[nodiscard]] int fontWeight(FontChoice choice);  // 400, или 700 для вариативного Nunito (см. theme.cpp)
[[nodiscard]] QFont font(int pixelSize = kFontPx); // шрифт текущего выбора

// Таблица стилей для текущих схемы и шрифта.
QString styleSheet();

// Блок в рамке.
QFrame* createPanel(QWidget* parent);

// Вертикальная перегородка внутри блока (цвет рамки).
QFrame* createDivider(QWidget* parent);

// Надпись цвета текста.
QLabel* createText(const QString& text, QWidget* parent);

// Числовое значение цвета значений.
QLabel* createValue(const QString& text, QWidget* parent);

// Фрагмент rich text с цветом значения — для надписей, где цифры стоят внутри текста.
// Цвет фиксируется при вызове: после смены схемы такие надписи нужно задать заново.
QString valueSpan(const QString& text);

} // namespace mrs::ui::theme
