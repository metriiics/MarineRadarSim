#pragma once

#include <QWidget>

class QCheckBox;

namespace mrs::ui
{

// Настройки приложения (не органы РЛС) — страница на всё окно вместо рабочего места.
// Слева — разделы, справа — содержимое раздела.
class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    // Синхронизировать элементы с текущим состоянием окна перед показом.
    void setFullScreen(bool fullScreen);

signals:
    void closeRequested();
    void fullScreenToggled(bool fullScreen);

private:
    QWidget* createDisplaySection();
    QWidget* createAboutSection();

    QCheckBox* fullScreen_{nullptr};
};

} // namespace mrs::ui
