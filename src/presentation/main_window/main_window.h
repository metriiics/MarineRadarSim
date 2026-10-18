#pragma once

#include <QList>
#include <QMainWindow>

class QHBoxLayout;
class QLabel;
class QStackedWidget;

namespace mrs::ui
{

class PpiWidget;
class SettingsPage;
class TitleBar;

// Рабочее место оператора РЛС (docs/05-operator-ui.md, §5.2).
// Окно содержит две страницы: рабочее место и настройки (на всё окно).
class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* createWorkstation();
    QWidget* createPpiArea(QWidget* parent);
    QWidget* createDataColumn(QWidget* parent);
    void layoutWorkstation();
    void alignBlocksToScale();

    void toggleSettings();
    void setFullScreenMode(bool fullScreen);
    void updateClock();
    bool handleWindowResize(QEvent* event);

    TitleBar* titleBar_{nullptr};
    QStackedWidget* pages_{nullptr};
    QWidget* workstation_{nullptr};
    QWidget* ppiArea_{nullptr};
    QWidget* dataColumn_{nullptr};
    QWidget* targetPanel_{nullptr};
    QWidget* datumPanel_{nullptr};     // СК-42
    QWidget* datePanel_{nullptr};
    QWidget* clockRow_{nullptr};
    QWidget* reservedPanel_{nullptr};  // пустой блок над UTC
    QWidget* messagesPanel_{nullptr};
    QWidget* eblPanel_{nullptr};
    QWidget* rangePanel_{nullptr};
    QLabel* rangeRingsLabel_{nullptr};
    QList<QWidget*> modePanels_;    // КУРС, ОД, ВОДА  // «/ 0.25» внутри блока шкалы дальности
    QList<QWidget*> controls_;      // УСИЛЕНИЕ, ВОЛНЫ, ДОЖДЬ, АПЧ
    QWidget* emissionPanel_{nullptr};
    QWidget* tuningPanel_{nullptr};
    QWidget* bandPanel_{nullptr};
    QWidget* ipzPanel_{nullptr};
    QHBoxLayout* smallBoxesLayout_{nullptr};
    SettingsPage* settingsPage_{nullptr};
    PpiWidget* ppi_{nullptr};
    QLabel* clockLabel_{nullptr};
    QLabel* dateLabel_{nullptr};
};

} // namespace mrs::ui
