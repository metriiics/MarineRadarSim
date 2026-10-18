#pragma once

#include "presentation/theme/theme.h"
#include "presentation/widgets/cursor_style.h"
#include "presentation/widgets/filler_panel.h"

#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QString>

namespace mrs::ui
{

// Действие, которому можно назначить клавишу. Большинство пока не подключены к модели РЛС —
// назначения сохраняются, обработчики появятся вместе с ядром симуляции.
struct KeyAction
{
    QString id;
    QString title;
    QKeySequence defaultKey;
    bool implemented{false};
};

[[nodiscard]] const QList<KeyAction>& keyActions();

namespace key_action
{
inline const QString kOpenSettings = QStringLiteral("open_settings");
inline const QString kFullScreen = QStringLiteral("full_screen");
} // namespace key_action

// Настройки интерфейса. Хранятся в INI-файле пользователя (QSettings).
struct UiSettings
{
    theme::ColorScheme colorScheme{theme::ColorScheme::Soft};
    theme::FontChoice font{theme::FontChoice::System};
    CursorStyle cursor{CursorStyle::Sight};
    FillerStyle filler{FillerStyle::Logo};
    bool fullScreen{false};
    bool headingLine{true};
    QMap<QString, QKeySequence> keys;  // id действия → клавиша; пусто — значение по умолчанию

    [[nodiscard]] QKeySequence key(const QString& actionId) const;

    [[nodiscard]] static UiSettings defaults();
    [[nodiscard]] static UiSettings load();
    void save() const;
};

} // namespace mrs::ui
