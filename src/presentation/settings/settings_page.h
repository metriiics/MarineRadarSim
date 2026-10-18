#pragma once

#include "presentation/settings/interface_settings.h"

#include <QMap>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QKeySequenceEdit;
class QLabel;

namespace mrs::ui
{

class FillerPanel;

// Настройки приложения (не органы РЛС) — страница на всё окно вместо рабочего места.
// Слева — разделы, справа — содержимое раздела. Изменения применяются сразу.
class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    // Заполнить элементы текущими настройками (без сигналов об изменении).
    void setSettings(const UiSettings& settings);

signals:
    void closeRequested();
    void settingsChanged(const mrs::ui::UiSettings& settings);

private:
    QWidget* createDisplaySection();
    QWidget* createControlsSection();
    QWidget* createAboutSection();

    void refreshPreviews();
    void refreshKeyConflicts();
    void emitChanged();

    UiSettings settings_;

    QComboBox* scheme_{nullptr};
    QWidget* schemePreview_{nullptr};
    QComboBox* font_{nullptr};
    QLabel* fontPreview_{nullptr};
    QComboBox* cursor_{nullptr};
    QLabel* cursorPreview_{nullptr};
    QComboBox* filler_{nullptr};
    FillerPanel* fillerPreview_{nullptr};
    QCheckBox* fullScreen_{nullptr};
    QCheckBox* headingLine_{nullptr};

    QMap<QString, QKeySequenceEdit*> keyEdits_;
    QLabel* keyStatus_{nullptr};
};

} // namespace mrs::ui
