#include "interface_settings.h"

#include <QSettings>

#include <memory>

namespace mrs::ui
{

namespace
{

// INI-файл пользователя: ~/.config/MarineRadarSim/MarineRadarSim.ini (Linux), %APPDATA% (Windows).
std::unique_ptr<QSettings> openStorage()
{
    return std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope,
                                       QStringLiteral("MarineRadarSim"), QStringLiteral("MarineRadarSim"));
}

// Чтение перечисления с проверкой диапазона: испорченный файл не должен ломать запуск.
template <typename Enum>
Enum readEnum(const QSettings& settings, const QString& key, Enum fallback, const QList<Enum>& allowed)
{
    bool ok = false;
    const int raw = settings.value(key).toInt(&ok);
    for (Enum value : allowed)
    {
        if (ok && static_cast<int>(value) == raw)
        {
            return value;
        }
    }
    return fallback;
}

} // namespace

const QList<KeyAction>& keyActions()
{
    static const QList<KeyAction> actions = {
        {QStringLiteral("gain_up"), QStringLiteral("Усиление +"), QKeySequence(Qt::Key_Q)},
        {QStringLiteral("gain_down"), QStringLiteral("Усиление −"), QKeySequence(Qt::Key_A)},
        {QStringLiteral("sea_up"), QStringLiteral("Волны +"), QKeySequence(Qt::Key_W)},
        {QStringLiteral("sea_down"), QStringLiteral("Волны −"), QKeySequence(Qt::Key_S)},
        {QStringLiteral("rain_up"), QStringLiteral("Дождь +"), QKeySequence(Qt::Key_E)},
        {QStringLiteral("rain_down"), QStringLiteral("Дождь −"), QKeySequence(Qt::Key_D)},
        {QStringLiteral("range_up"), QStringLiteral("Шкала дальности +"), QKeySequence(Qt::Key_PageUp)},
        {QStringLiteral("range_down"), QStringLiteral("Шкала дальности −"), QKeySequence(Qt::Key_PageDown)},
        {QStringLiteral("transmit"), QStringLiteral("Излучение вкл/выкл"), QKeySequence(Qt::Key_T)},
        {QStringLiteral("orientation"), QStringLiteral("Ориентация (Курс / Север / Нос)"), QKeySequence(Qt::Key_O)},
        {QStringLiteral("motion"), QStringLiteral("Режим движения (ОД / ИД)"), QKeySequence(Qt::Key_M)},
        {QStringLiteral("ebl"), QStringLiteral("Визир направления (ВН)"), QKeySequence(Qt::Key_B)},
        {QStringLiteral("vrm"), QStringLiteral("Визир дальности (ВД)"), QKeySequence(Qt::Key_R)},
        {QStringLiteral("acquire"), QStringLiteral("Захват цели (САРП)"), QKeySequence(Qt::Key_Return)},
        {QStringLiteral("drop"), QStringLiteral("Сброс цели"), QKeySequence(Qt::Key_Delete)},
        {QStringLiteral("acknowledge"), QStringLiteral("Квитирование тревоги"), QKeySequence(Qt::Key_F2)},
        {key_action::kOpenSettings, QStringLiteral("Открыть настройки"), QKeySequence(Qt::Key_F10), true},
        {key_action::kFullScreen, QStringLiteral("Полноэкранный режим"), QKeySequence(Qt::Key_F11), true},
    };
    return actions;
}

QKeySequence UiSettings::key(const QString& actionId) const
{
    if (const auto it = keys.find(actionId); it != keys.end())
    {
        return it.value();
    }
    for (const KeyAction& action : keyActions())
    {
        if (action.id == actionId)
        {
            return action.defaultKey;
        }
    }
    return {};
}

UiSettings UiSettings::defaults()
{
    return {};
}

UiSettings UiSettings::load()
{
    const auto storage = openStorage();
    const QSettings& settings = *storage;
    UiSettings result;
    result.colorScheme =
        readEnum(settings, QStringLiteral("screen/colorScheme"), result.colorScheme, theme::allSchemes());
    result.font = readEnum(settings, QStringLiteral("screen/font"), result.font, theme::allFonts());
    result.cursor = readEnum(settings, QStringLiteral("screen/cursor"), result.cursor, allCursorStyles());
    result.filler = readEnum(settings, QStringLiteral("screen/filler"), result.filler, allFillerStyles());
    result.fullScreen = settings.value(QStringLiteral("screen/fullScreen"), result.fullScreen).toBool();
    result.headingLine = settings.value(QStringLiteral("screen/headingLine"), result.headingLine).toBool();

    for (const KeyAction& action : keyActions())
    {
        const QString key = QStringLiteral("keys/") + action.id;
        if (settings.contains(key))
        {
            result.keys.insert(action.id, QKeySequence::fromString(settings.value(key).toString(),
                                                                   QKeySequence::PortableText));
        }
    }
    return result;
}

void UiSettings::save() const
{
    const auto storage = openStorage();
    QSettings& settings = *storage;
    settings.setValue(QStringLiteral("screen/colorScheme"), static_cast<int>(colorScheme));
    settings.setValue(QStringLiteral("screen/font"), static_cast<int>(font));
    settings.setValue(QStringLiteral("screen/cursor"), static_cast<int>(cursor));
    settings.setValue(QStringLiteral("screen/filler"), static_cast<int>(filler));
    settings.setValue(QStringLiteral("screen/fullScreen"), fullScreen);
    settings.setValue(QStringLiteral("screen/headingLine"), headingLine);

    settings.remove(QStringLiteral("keys"));
    for (auto it = keys.begin(); it != keys.end(); ++it)
    {
        settings.setValue(QStringLiteral("keys/") + it.key(), it.value().toString(QKeySequence::PortableText));
    }
}

} // namespace mrs::ui
