#pragma once

#include <QAbstractButton>
#include <QWidget>

namespace mrs::ui
{

// Кнопка заголовка окна с векторной иконкой (не зависит от шрифтов и темы ОС).
class TitleBarButton final : public QAbstractButton
{
public:
    enum class Kind
    {
        Settings,
        Minimize,
        Maximize,
        Restore,
        Close
    };

    explicit TitleBarButton(Kind kind, QWidget* parent = nullptr);

    void setKind(Kind kind);

    // Подсвеченная иконка — режим, открытый этой кнопкой, активен.
    void setActive(bool active);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    Kind kind_;
    bool hovered_{false};
    bool active_{false};
};

// Заголовок окна без системной рамки. Кнопка настроек вынесена сюда,
// рядом со «свернуть» / «развернуть» / «закрыть», чтобы не добавлять
// на экран РЛС органы, которых нет у оригинала.
class TitleBar final : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent);

    // Подсветка кнопки настроек, пока открыта страница настроек.
    void setSettingsActive(bool active);

signals:
    void settingsRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void toggleMaximized();
    void updateMaximizeButton();

    TitleBarButton* settingsButton_;
    TitleBarButton* maximizeButton_;
};

} // namespace mrs::ui
