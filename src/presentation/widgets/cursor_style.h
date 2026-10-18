#pragma once

#include <QCursor>
#include <QList>
#include <QPixmap>
#include <QString>

class QColor;

namespace mrs::ui
{

// Вид курсора на ИКО.
enum class CursorStyle
{
    Sight,      // прицел: окружность с рисками и точкой в центре (по умолчанию)
    Crosshair,  // тонкое перекрестие
    Circle,     // окружность с точкой
    Arrow,      // обычная стрелка
};

[[nodiscard]] QString cursorStyleName(CursorStyle style);
[[nodiscard]] QList<CursorStyle> allCursorStyles();

// Изображение курсора (для предпросмотра в настройках). Для Arrow — пустое.
[[nodiscard]] QPixmap cursorPixmap(CursorStyle style, const QColor& color);

[[nodiscard]] QCursor makeCursor(CursorStyle style, const QColor& color);

} // namespace mrs::ui
