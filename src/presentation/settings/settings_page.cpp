#include "settings_page.h"

#include "presentation/theme/theme.h"
#include "presentation/widgets/filler_panel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

namespace mrs::ui
{

namespace
{

constexpr int kNavigationWidth = 280;
constexpr int kHeaderFontPx = 24;
constexpr int kSectionTitleFontPx = 18;
constexpr int kGroupTitleFontPx = 15;
constexpr int kLabelColumnWidth = 200;
constexpr int kControlWidth = 300;
constexpr int kKeyColumnWidth = 220;

// Образцы цветов схемы: фон, рамка, текст, цифры, шкала.
class SchemeSwatch final : public QWidget
{
public:
    explicit SchemeSwatch(QWidget* parent)
        : QWidget(parent)
    {
        setFixedSize(5 * 30 + 4 * 4, 26);
    }

    void setScheme(theme::ColorScheme scheme)
    {
        scheme_ = scheme;
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        const theme::Palette p = theme::schemePalette(scheme_);
        const QColor colors[] = {p.background, p.frame, p.text, p.value, p.ppiScale};
        QPainter painter(this);
        int x = 0;
        for (const QColor& color : colors)
        {
            painter.fillRect(QRect(x, 0, 30, height()), color);
            painter.setPen(theme::palette().frame);
            painter.drawRect(QRect(x, 0, 29, height() - 1));
            x += 34;
        }
    }

private:
    theme::ColorScheme scheme_{theme::ColorScheme::Soft};
};

QWidget* createSection(const QString& title, QWidget* parent, QVBoxLayout** contentLayout)
{
    auto* section = theme::createPanel(parent);

    auto* layout = new QVBoxLayout(section);
    layout->setContentsMargins(2 * theme::kPadding, 2 * theme::kPadding, 2 * theme::kPadding,
                               2 * theme::kPadding);
    layout->setSpacing(3 * theme::kPadding / 2);

    auto* titleLabel = theme::createText(title, section);
    titleLabel->setStyleSheet(QStringLiteral("font-size: %1px;").arg(kSectionTitleFontPx));
    layout->addWidget(titleLabel);

    *contentLayout = layout;
    return section;
}

QLabel* createGroupTitle(const QString& title, QWidget* parent)
{
    auto* label = theme::createValue(title, parent);
    label->setStyleSheet(QStringLiteral("font-size: %1px; padding-top: 8px;").arg(kGroupTitleFontPx));
    return label;
}

template <typename Enum>
QComboBox* createEnumCombo(const QList<Enum>& values, QString (*name)(Enum), QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    combo->setFixedWidth(kControlWidth);
    combo->setCursor(Qt::PointingHandCursor);
    for (Enum value : values)
    {
        combo->addItem(name(value), static_cast<int>(value));
    }
    return combo;
}

template <typename Enum>
void selectEnum(QComboBox* combo, Enum value)
{
    const QSignalBlocker blocker(combo);
    combo->setCurrentIndex(combo->findData(static_cast<int>(value)));
}

template <typename Enum>
Enum currentEnum(const QComboBox* combo)
{
    return static_cast<Enum>(combo->currentData().toInt());
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
        {tr("УПРАВЛЕНИЕ"), createControlsSection()},
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

    setSettings(UiSettings::defaults());
}

void SettingsPage::setSettings(const UiSettings& settings)
{
    settings_ = settings;

    selectEnum(scheme_, settings.colorScheme);
    selectEnum(font_, settings.font);
    selectEnum(cursor_, settings.cursor);
    selectEnum(filler_, settings.filler);
    {
        const QSignalBlocker blockFull(fullScreen_);
        const QSignalBlocker blockHeading(headingLine_);
        fullScreen_->setChecked(settings.fullScreen);
        headingLine_->setChecked(settings.headingLine);
    }
    for (auto it = keyEdits_.begin(); it != keyEdits_.end(); ++it)
    {
        const QSignalBlocker blocker(it.value());
        it.value()->setKeySequence(settings.key(it.key()));
    }

    refreshPreviews();
    refreshKeyConflicts();
}

QWidget* SettingsPage::createDisplaySection()
{
    QVBoxLayout* sectionLayout = nullptr;
    auto* section = createSection(tr("ЭКРАН"), this, &sectionLayout);

    // Содержимое прокручивается, если не помещается по высоте.
    auto* scroll = new QScrollArea(section);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    sectionLayout->addWidget(scroll, 1);

    auto* grid = new QGridLayout(content);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(2 * theme::kPadding);
    grid->setVerticalSpacing(theme::kPadding);
    grid->setColumnMinimumWidth(0, kLabelColumnWidth);
    grid->setColumnStretch(2, 1);
    int row = 0;

    const auto addRow = [&](const QString& name, QWidget* control, QWidget* preview) {
        grid->addWidget(theme::createText(name, content), row, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(control, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        if (preview != nullptr)
        {
            grid->addWidget(preview, row, 2, Qt::AlignLeft | Qt::AlignVCenter);
        }
        ++row;
    };
    const auto addGroup = [&](const QString& title) {
        grid->addWidget(createGroupTitle(title, content), row++, 0, 1, 3);
    };

    addGroup(tr("ОФОРМЛЕНИЕ"));

    scheme_ = createEnumCombo(theme::allSchemes(), &theme::schemeName, content);
    auto* swatch = new SchemeSwatch(content);
    schemePreview_ = swatch;
    addRow(tr("Цветовая схема"), scheme_, swatch);

    font_ = createEnumCombo(theme::allFonts(), &theme::fontName, content);
    fontPreview_ = theme::createText(QStringLiteral("КУРС ГИРО  170.0°   ДКС 2.00 МИЛЬ"), content);
    addRow(tr("Шрифт"), font_, fontPreview_);

    cursor_ = createEnumCombo(allCursorStyles(), &cursorStyleName, content);
    cursorPreview_ = new QLabel(content);
    cursorPreview_->setFixedSize(40, 40);
    cursorPreview_->setAlignment(Qt::AlignCenter);
    addRow(tr("Курсор на ИКО"), cursor_, cursorPreview_);

    filler_ = createEnumCombo(allFillerStyles(), &fillerStyleName, content);
    fillerPreview_ = new FillerPanel(content);
    fillerPreview_->setFixedSize(240, 150);
    addRow(tr("Боковые поля"), filler_, fillerPreview_);

    auto* logoHint = theme::createText(
        tr("Поля слева и справа от интерфейса на широком мониторе (на экране 5:4 их нет).\n"
           "Официальный логотип: файл data/branding/dalrybvtuz.png (или .svg) рядом с программой;\n"
           "пока файла нет, рисуется условная эмблема."),
        content);
    logoHint->setWordWrap(true);
    grid->addWidget(logoHint, row++, 1, 1, 2);

    addGroup(tr("ОБЩИЕ"));

    fullScreen_ = new QCheckBox(tr("Полноэкранный режим (без заголовка окна)"), content);
    fullScreen_->setCursor(Qt::PointingHandCursor);
    grid->addWidget(fullScreen_, row++, 0, 1, 3);

    headingLine_ = new QCheckBox(tr("Показывать линию курса на ИКО"), content);
    headingLine_->setCursor(Qt::PointingHandCursor);
    grid->addWidget(headingLine_, row++, 0, 1, 3);

    auto* reset = new QPushButton(tr("СБРОСИТЬ НАСТРОЙКИ ЭКРАНА"), content);
    reset->setCursor(Qt::PointingHandCursor);
    grid->addWidget(reset, row++, 0, 1, 2, Qt::AlignLeft);

    grid->setRowStretch(row, 1);

    connect(scheme_, &QComboBox::currentIndexChanged, this, [this] {
        settings_.colorScheme = currentEnum<theme::ColorScheme>(scheme_);
        emitChanged();
    });
    connect(font_, &QComboBox::currentIndexChanged, this, [this] {
        settings_.font = currentEnum<theme::FontChoice>(font_);
        emitChanged();
    });
    connect(cursor_, &QComboBox::currentIndexChanged, this, [this] {
        settings_.cursor = currentEnum<CursorStyle>(cursor_);
        emitChanged();
    });
    connect(filler_, &QComboBox::currentIndexChanged, this, [this] {
        settings_.filler = currentEnum<FillerStyle>(filler_);
        emitChanged();
    });
    connect(fullScreen_, &QCheckBox::toggled, this, [this](bool checked) {
        settings_.fullScreen = checked;
        emitChanged();
    });
    connect(headingLine_, &QCheckBox::toggled, this, [this](bool checked) {
        settings_.headingLine = checked;
        emitChanged();
    });
    connect(reset, &QPushButton::clicked, this, [this] {
        UiSettings fresh = UiSettings::defaults();
        fresh.keys = settings_.keys;  // назначения клавиш сбрасываются в своём разделе
        setSettings(fresh);
        emitChanged();
    });

    return section;
}

QWidget* SettingsPage::createControlsSection()
{
    QVBoxLayout* content = nullptr;
    auto* section = createSection(tr("УПРАВЛЕНИЕ"), this, &content);

    auto* hint = theme::createText(
        tr("Щёлкните по клавише и нажмите новую. Действия, отмеченные «позже», будут подключены "
           "вместе с моделью РЛС — назначения уже сохраняются."),
        section);
    hint->setWordWrap(true);
    content->addWidget(hint);

    const QList<KeyAction>& actions = keyActions();
    auto* table = new QTableWidget(static_cast<int>(actions.size()), 3, section);
    table->setHorizontalHeaderLabels({tr("ДЕЙСТВИЕ"), tr("КЛАВИША"), tr("СОСТОЯНИЕ")});
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setColumnWidth(1, kKeyColumnWidth);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);

    for (int row = 0; row < actions.size(); ++row)
    {
        const KeyAction& action = actions[row];
        table->setItem(row, 0, new QTableWidgetItem(action.title));

        auto* edit = new QKeySequenceEdit(table);
        keyEdits_.insert(action.id, edit);
        table->setCellWidget(row, 1, edit);

        auto* state = new QTableWidgetItem(action.implemented ? tr("работает") : tr("позже"));
        table->setItem(row, 2, state);
        table->setRowHeight(row, 32);

        // Одна клавиша (с модификаторами) на действие: лишние аккорды отбрасываются.
        connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit, id = action.id] {
            const QKeySequence recorded = edit->keySequence();
            const QKeySequence single = recorded.isEmpty() ? QKeySequence() : QKeySequence(recorded[0]);
            if (single != recorded)
            {
                const QSignalBlocker blocker(edit);
                edit->setKeySequence(single);
            }
            settings_.keys.insert(id, single);
            refreshKeyConflicts();
            emitChanged();
        });
    }
    content->addWidget(table, 1);

    keyStatus_ = theme::createValue({}, section);
    content->addWidget(keyStatus_);

    auto* reset = new QPushButton(tr("СБРОСИТЬ НАЗНАЧЕНИЯ"), section);
    reset->setCursor(Qt::PointingHandCursor);
    content->addWidget(reset, 0, Qt::AlignLeft);

    connect(reset, &QPushButton::clicked, this, [this] {
        UiSettings fresh = settings_;
        fresh.keys.clear();
        setSettings(fresh);
        emitChanged();
    });

    return section;
}

QWidget* SettingsPage::createAboutSection()
{
    QVBoxLayout* content = nullptr;
    auto* section = createSection(tr("О ПРОГРАММЕ"), this, &content);

    auto* about = theme::createText(
        tr("MarineRadarSim — учебный тренажёр судовой РЛС с САРП<br>Версия %1")
            .arg(QCoreApplication::applicationVersion()),
        section);
    content->addWidget(about);
    content->addStretch();

    return section;
}

void SettingsPage::refreshPreviews()
{
    static_cast<SchemeSwatch*>(schemePreview_)->setScheme(settings_.colorScheme);

    // Таблица стилей задаёт шрифт всем виджетам — для предпросмотра переопределяем его.
    fontPreview_->setStyleSheet(QStringLiteral("font-family: \"%1\"; font-weight: %2; font-size: %3px;")
                                    .arg(theme::fontFamily(settings_.font))
                                    .arg(theme::fontWeight(settings_.font))
                                    .arg(theme::kFontPx + 2));

    const QPixmap pixmap = cursorPixmap(settings_.cursor, theme::palette().cursor);
    if (pixmap.isNull())
    {
        cursorPreview_->setPixmap({});
        cursorPreview_->setText(QStringLiteral("↖"));
    }
    else
    {
        cursorPreview_->setText({});
        cursorPreview_->setPixmap(pixmap);
    }

    fillerPreview_->setFillerStyle(settings_.filler);
}

void SettingsPage::refreshKeyConflicts()
{
    // Одна клавиша на нескольких действиях подсвечивается.
    QMap<QKeySequence, QStringList> owners;
    for (const KeyAction& action : keyActions())
    {
        const QKeySequence key = settings_.key(action.id);
        if (!key.isEmpty())
        {
            owners[key].append(action.title);
        }
    }

    QStringList conflicts;
    for (const KeyAction& action : keyActions())
    {
        QKeySequenceEdit* edit = keyEdits_.value(action.id);
        const QKeySequence key = settings_.key(action.id);
        const bool conflict = !key.isEmpty() && owners.value(key).size() > 1;
        edit->setProperty("conflict", conflict);
        edit->style()->unpolish(edit);
        edit->style()->polish(edit);
        if (conflict && !conflicts.contains(key.toString(QKeySequence::NativeText)))
        {
            conflicts.append(key.toString(QKeySequence::NativeText));
        }
    }

    keyStatus_->setText(conflicts.isEmpty()
                            ? QString()
                            : tr("Одна клавиша назначена нескольким действиям: %1").arg(conflicts.join(QStringLiteral(", "))));
}

void SettingsPage::emitChanged()
{
    refreshPreviews();
    emit settingsChanged(settings_);
}

} // namespace mrs::ui
