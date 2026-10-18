#include "settings_page.h"

#include "presentation/theme/theme.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace mrs::ui
{

namespace
{

constexpr int kNavigationWidth = 280;
constexpr int kHeaderFontPx = 24;
constexpr int kSectionTitleFontPx = 18;

QWidget* createSection(const QString& title, QWidget* parent, QVBoxLayout** contentLayout)
{
    auto* section = theme::createPanel(parent);

    auto* layout = new QVBoxLayout(section);
    layout->setContentsMargins(2 * theme::kPadding, 2 * theme::kPadding, 2 * theme::kPadding,
                               2 * theme::kPadding);
    layout->setSpacing(2 * theme::kSpacing);

    auto* titleLabel = theme::createText(title, section);
    titleLabel->setStyleSheet(QStringLiteral("font-size: %1px;").arg(kSectionTitleFontPx));
    layout->addWidget(titleLabel);

    *contentLayout = layout;
    return section;
}

} // namespace

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(theme::kSpacing);

    // Заголовок страницы и возврат к рабочему месту.
    auto* header = theme::createPanel(this);
    header->setFixedHeight(56);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(2 * theme::kPadding, 0, theme::kPadding, 0);

    auto* title = theme::createText(tr("НАСТРОЙКИ"), header);
    title->setStyleSheet(QStringLiteral("font-size: %1px;").arg(kHeaderFontPx));

    auto* backButton = new QPushButton(tr("НАЗАД  (Esc)"), header);
    backButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(backButton);

    layout->addWidget(header);

    // Разделы.
    auto* body = new QHBoxLayout();
    body->setSpacing(theme::kSpacing);

    auto* navigation = new QListWidget(this);
    navigation->setFixedWidth(kNavigationWidth);
    navigation->setCursor(Qt::PointingHandCursor);

    auto* sections = new QStackedWidget(this);

    const std::pair<QString, QWidget*> pages[] = {
        {tr("ЭКРАН"), createDisplaySection()},
        {tr("О ПРОГРАММЕ"), createAboutSection()},
    };
    for (const auto& [name, page] : pages)
    {
        navigation->addItem(name);
        sections->addWidget(page);
    }

    connect(navigation, &QListWidget::currentRowChanged, sections, &QStackedWidget::setCurrentIndex);
    navigation->setCurrentRow(0);

    body->addWidget(navigation);
    body->addWidget(sections, 1);
    layout->addLayout(body, 1);

    connect(backButton, &QPushButton::clicked, this, &SettingsPage::closeRequested);

    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape, &QShortcut::activated, this, &SettingsPage::closeRequested);
}

void SettingsPage::setFullScreen(bool fullScreen)
{
    const QSignalBlocker blocker(fullScreen_);
    fullScreen_->setChecked(fullScreen);
}

QWidget* SettingsPage::createDisplaySection()
{
    QVBoxLayout* content = nullptr;
    auto* section = createSection(tr("ЭКРАН"), this, &content);

    fullScreen_ = new QCheckBox(tr("Полноэкранный режим (без заголовка окна)"), section);
    fullScreen_->setCursor(Qt::PointingHandCursor);
    content->addWidget(fullScreen_);

    auto* hint = theme::createText(
        tr("В полноэкранном режиме настройки открываются клавишей F10."), section);
    content->addWidget(hint);

    content->addStretch();

    connect(fullScreen_, &QCheckBox::toggled, this, &SettingsPage::fullScreenToggled);

    return section;
}

QWidget* SettingsPage::createAboutSection()
{
    QVBoxLayout* content = nullptr;
    auto* section = createSection(tr("О ПРОГРАММЕ"), this, &content);

    auto* about = theme::createText(
        tr("MarineRadarSim — учебный тренажёр судовой РЛС с САРП<br>Версия %1")
            .arg(theme::valueSpan(QCoreApplication::applicationVersion())),
        section);
    content->addWidget(about);
    content->addStretch();

    return section;
}

} // namespace mrs::ui
