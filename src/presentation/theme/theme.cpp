#include "theme.h"

#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <QFrame>
#include <QLabel>

namespace mrs::ui::theme
{

namespace
{

Palette g_palette = schemePalette(ColorScheme::Soft);
FontChoice g_font = FontChoice::System;

constexpr auto kRoundedFamily = "Nunito";

// В data/fonts лежит вариативный Nunito (google/fonts). Qt 6.4 не умеет выбирать у него толщину
// и берёт начальное начертание ExtraLight — слишком тонкое для экрана РЛС. Пока нет обычного
// Regular-начертания, для скруглённого шрифта запрашивается утолщение (вес 700), которое Qt
// синтезирует из ExtraLight; по толщине это близко к обычному начертанию.
bool g_roundedNeedsEmbolden = false;


// Каталоги с шрифтами, которые поставляются вместе с программой.
QStringList fontDirectories()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    return {appDir + QStringLiteral("/data/fonts"), appDir + QStringLiteral("/../data/fonts")};
}

} // namespace

int fontWeight(FontChoice choice)
{
    return (choice == FontChoice::Rounded && g_roundedNeedsEmbolden) ? 700 : 400;
}

const Palette& palette()
{
    return g_palette;
}

void setColorScheme(ColorScheme scheme)
{
    g_palette = schemePalette(scheme);
}

Palette schemePalette(ColorScheme scheme)
{
    switch (scheme)
    {
    case ColorScheme::Classic:
        return {QColor(0, 0, 0),       QColor(230, 200, 0),  QColor(60, 140, 255),
                QColor(255, 224, 0),   QColor(0, 200, 0),    QColor(0, 200, 0),
                QColor(30, 30, 30),    QColor(220, 230, 255), QColor(255, 224, 0)};
    case ColorScheme::Night:
        return {QColor(0, 0, 0),       QColor(107, 58, 30),  QColor(176, 106, 69),
                QColor(208, 138, 74),  QColor(138, 74, 42),  QColor(138, 74, 42),
                QColor(26, 14, 8),     QColor(160, 96, 74),  QColor(208, 138, 74)};
    case ColorScheme::Phosphor:
        return {QColor(1, 8, 5),       QColor(45, 107, 60),  QColor(92, 184, 116),
                QColor(166, 230, 180), QColor(63, 154, 82),  QColor(63, 154, 82),
                QColor(12, 26, 16),    QColor(154, 216, 168), QColor(166, 230, 180)};
    case ColorScheme::Steel:
        return {QColor(10, 13, 16),    QColor(90, 107, 122), QColor(143, 176, 204),
                QColor(217, 226, 234), QColor(79, 154, 138), QColor(79, 154, 138),
                QColor(26, 33, 40),    QColor(192, 204, 214), QColor(217, 226, 234)};
    case ColorScheme::Soft:
        break;
    }
    // Мягкая: тона исходной схемы, но рамки и цифры приглушены, фон не чисто чёрный.
    return {QColor(6, 9, 13),      QColor(168, 138, 58), QColor(110, 155, 209),
            QColor(227, 197, 101), QColor(63, 154, 85),  QColor(63, 154, 85),
            QColor(24, 32, 41),    QColor(201, 211, 224), QColor(227, 197, 101)};
}

QString schemeName(ColorScheme scheme)
{
    switch (scheme)
    {
    case ColorScheme::Soft:
        return QStringLiteral("Мягкая");
    case ColorScheme::Classic:
        return QStringLiteral("Контрастная (исходная)");
    case ColorScheme::Night:
        return QStringLiteral("Ночная");
    case ColorScheme::Phosphor:
        return QStringLiteral("Зелёный экран");
    case ColorScheme::Steel:
        return QStringLiteral("Стальная");
    }
    return {};
}

QList<ColorScheme> allSchemes()
{
    return {ColorScheme::Soft, ColorScheme::Classic, ColorScheme::Night, ColorScheme::Phosphor, ColorScheme::Steel};
}

void initFonts()
{
    // На Linux оригинальных шрифтов Microsoft обычно нет — берём метрически совместимые.
    QFont::insertSubstitutions(QStringLiteral("Times New Roman"),
                               {QStringLiteral("Liberation Serif"), QStringLiteral("Free Times"),
                                QStringLiteral("DejaVu Serif")});
    QFont::insertSubstitutions(QStringLiteral("Arial"),
                               {QStringLiteral("Liberation Sans"), QStringLiteral("DejaVu Sans")});
    QFont::insertSubstitutions(QString::fromLatin1(kRoundedFamily),
                               {QStringLiteral("Comfortaa"), QStringLiteral("Varela Round"),
                                QStringLiteral("DejaVu Sans")});

    for (const QString& path : fontDirectories())
    {
        const QDir dir(path);
        for (const QString& file : dir.entryList({QStringLiteral("*.ttf"), QStringLiteral("*.otf")}, QDir::Files))
        {
            QFontDatabase::addApplicationFont(dir.filePath(file));
        }
    }

    const QStringList roundedStyles = QFontDatabase::styles(QString::fromLatin1(kRoundedFamily));
    g_roundedNeedsEmbolden =
        !roundedStyles.isEmpty() && !roundedStyles.contains(QStringLiteral("Regular"), Qt::CaseInsensitive);
}

void setFontChoice(FontChoice choice)
{
    g_font = choice;
}

QString fontName(FontChoice choice)
{
    switch (choice)
    {
    case FontChoice::System:
        return QStringLiteral("Системный");
    case FontChoice::TimesNewRoman:
        return QStringLiteral("Times New Roman");
    case FontChoice::Arial:
        return QStringLiteral("Arial");
    case FontChoice::Rounded:
        return QStringLiteral("Скруглённый (Nunito)");
    }
    return {};
}

QList<FontChoice> allFonts()
{
    return {FontChoice::System, FontChoice::TimesNewRoman, FontChoice::Arial, FontChoice::Rounded};
}

QString fontFamily(FontChoice choice)
{
    switch (choice)
    {
    case FontChoice::TimesNewRoman:
        return QStringLiteral("Times New Roman");
    case FontChoice::Arial:
        return QStringLiteral("Arial");
    case FontChoice::Rounded:
        return QString::fromLatin1(kRoundedFamily);
    case FontChoice::System:
        break;
    }
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

QFont font(int pixelSize)
{
    QFont result(fontFamily(g_font));
    result.setPixelSize(pixelSize);
    result.setWeight(static_cast<QFont::Weight>(fontWeight(g_font)));
    return result;
}

QString styleSheet()
{
    const Palette& p = g_palette;
    const QString family = fontFamily(g_font);
    return QStringLiteral(
               "QWidget { font-family: \"%8\"; font-weight: %9; }"
               "QMainWindow, QDialog, QStackedWidget, QScrollArea, #centralWidget, #scrollContent"
               "  { background: %1; }"
               "QLabel { color: %2; background: transparent; font-size: %6px; }"
               "QLabel[role=\"value\"] { color: %3; }"
               "QFrame[panel=\"true\"] { background: %1; border: 1px solid %4; }"
               "QFrame#divider { background: %4; border: none; }"

               "QPushButton { color: %2; background: %1; border: 1px solid %4;"
               "  font-size: %6px; padding: 6px 20px; }"
               "QPushButton:hover { background: %5; }"

               "QCheckBox { color: %2; font-size: %6px; spacing: 10px; }"
               "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %4; background: %1; }"
               "QCheckBox::indicator:checked { background: %7; }"

               "QComboBox { color: %3; background: %1; border: 1px solid %4; font-size: %6px;"
               "  padding: 4px 10px; min-height: 22px; }"
               "QComboBox:hover { background: %5; }"
               "QComboBox::drop-down { border: none; width: 22px; }"
               "QComboBox QAbstractItemView { color: %2; background: %1; border: 1px solid %4;"
               "  selection-background-color: %5; selection-color: %3; outline: none; }"

               "QKeySequenceEdit, QLineEdit { color: %3; background: %1; border: 1px solid %4;"
               "  font-size: %6px; padding: 3px 6px; }"
               "QKeySequenceEdit[conflict=\"true\"] QLineEdit { border: 1px solid #c04040; }"

               "QTableWidget { color: %2; background: %1; border: 1px solid %4; gridline-color: %5;"
               "  font-size: %6px; outline: none; }"
               "QTableWidget::item { padding: 4px 8px; }"
               "QTableWidget::item:selected { background: %5; color: %3; }"
               "QHeaderView::section { color: %2; background: %1; border: none;"
               "  border-bottom: 1px solid %4; padding: 6px 8px; font-size: %6px; }"

               "QScrollArea { border: none; }"
               "QScrollBar:vertical { background: %1; width: 10px; margin: 0; }"
               "QScrollBar::handle:vertical { background: %5; border: 1px solid %4; min-height: 30px; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"

               "QListWidget { background: %1; border: 1px solid %4; outline: none;"
               "  font-size: %6px; padding: 4px 0; }"
               // Отступ повторён в каждом состоянии: иначе Qt сбрасывает его у невыбранных пунктов.
               "QListWidget::item { color: %2; padding: 8px 14px; border-left: 3px solid %1; }"
               "QListWidget::item:hover { color: %2; padding: 8px 14px; background: %5; }"
               "QListWidget::item:selected { color: %3; padding: 8px 14px; background: %5;"
               "  border-left: 3px solid %4; }")
        .arg(p.background.name(), p.text.name(), p.value.name(), p.frame.name(), p.hover.name(),
             QString::number(kFontPx), p.indicator.name(), family, QString::number(fontWeight(g_font)));
}

QFrame* createPanel(QWidget* parent)
{
    auto* panel = new QFrame(parent);
    panel->setProperty("panel", true);
    return panel;
}

QFrame* createDivider(QWidget* parent)
{
    auto* divider = new QFrame(parent);
    divider->setObjectName(QStringLiteral("divider"));
    divider->setFixedWidth(1);
    return divider;
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
    return QStringLiteral("<span style=\"color:%1\">%2</span>").arg(g_palette.value.name(), text.toHtmlEscaped());
}

} // namespace mrs::ui::theme
