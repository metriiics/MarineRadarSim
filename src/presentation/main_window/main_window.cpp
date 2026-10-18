#include "main_window.h"

#include "presentation/ppi/ppi_widget.h"
#include "presentation/settings/settings_page.h"
#include "presentation/theme/theme.h"
#include "presentation/title_bar/title_bar.h"
#include "presentation/widgets/clock_icon.h"
#include "presentation/widgets/cursor_style.h"
#include "presentation/widgets/data_panel.h"
#include "presentation/widgets/filler_panel.h"
#include "presentation/widgets/level_control.h"
#include "presentation/widgets/tuning_indicator.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace mrs::ui
{

namespace
{

using theme::kBlockHeight;
using theme::kPadding;
using theme::kSpacing;
using theme::kStackSpacing;

// Целевой экран — 1280×1024 (5:4). Круг ИКО занимает всю высоту и стоит вплотную к узкой колонке
// данных справа; слева от него остаётся полоса kLeftStrip. Остальные блоки лежат «лесенкой»
// в четырёх углах вокруг круга: каждая следующая строка уже предыдущей и повторяет изгиб шкалы
// пеленгов. Ширины рассчитаны для круга 980 px; на меньшем экране круг уменьшается и под блоки
// не заходит.
constexpr int kLeftStrip = 10;          // полоса слева от круга — под левые угловые блоки
// Длины левых верхних блоков привязаны к цифрам шкалы (alignBlocksToScale): правый край АПЧ
// (и остальных регуляторов) — над серединой цифры «3» в 320, Х-ДИАП — над серединой «3» в 310,
// индикатор — по перегородке АПЧ, ИЗЛУЧЕНИЕ — за цифру 340, не доходя до 350. Значения ниже — начальные
// и минимальные.
constexpr int kControlWidth = 185;      // регулятор: название + шкала
constexpr int kEmissionWidth = 166;
constexpr int kEmissionMinWidth = 130;   // чтобы «ИЗЛУЧЕНИЕ │ ВЫКЛ» помещалось целиком
constexpr double kControlBearing = 320.0;
constexpr double kBandBearing = 310.0;
constexpr double kEmissionBearing = 340.0;
constexpr int kTopLeftHeight = 24;      // начальная высота блоков левого верхнего угла
// Высота блоков левого верхнего угла подбирается так, чтобы АПЧ (4-я строка) стоял прямо над
// цифрой 320, а Х-ДИАП (6-я строка) — прямо над 310.
constexpr int kControlRow = 3;          // АПЧ — номер строки (с 0)
constexpr int kBandRow = 5;             // Х-ДИАП
constexpr int kAboveLabelGap = 2;       // от низа блока до верха цифры
constexpr int kTopLeftMinHeight = 20;
constexpr int kTopLeftMaxHeight = 34;
constexpr int kEmissionLabelOverrun = 22; // ИЗЛУЧЕНИЕ заходит за цифру 340, не доходя до 350
constexpr int kEmissionGap = 6;         // между УСИЛЕНИЕМ и ИЗЛУЧЕНИЕМ (стоят рядом)
constexpr int kTuningWidth = LevelControl::titleWidth(kControlWidth); // по ширине блока АПЧ
constexpr int kBandWidth = kTuningWidth;       // Х-ДИАП — по ширине индикатора над ним
constexpr int kIpzWidth = kBandWidth / 2;      // ИПЗ — в половину Х-ДИАП
constexpr int kEblMinWidth = 170;       // ВН тянется до цифры 220 шкалы, но не уже этого
constexpr double kEblRightBearing = 220.0;
constexpr int kEblLabelInset = 8;       // правая граница ВН — чуть не доходя до центра цифры
constexpr int kEblHeight = 90;

constexpr int kRangeMinWidth = 200;    // шкала дальности тянется влево чуть дальше цифры 30
constexpr int kRangeHeight = 40;
constexpr double kRangeLeftBearing = 30.0;
constexpr int kRangeLabelOverlap = 30;  // насколько левая граница заходит за центр цифры 30 (не доходя до 20)
constexpr double kRangeRingsBearing = 40.0; // «0.25» стоит над цифрой «0» отметки 40
constexpr int kRangeScalePx = 32;       // крупная цифра шкалы дальности — почти во всю высоту блока
constexpr int kModeWidth = 120;         // КУРС, ОД, ВОДА — начальная/минимальная длина
constexpr double kModeLeftBearing = 50.0; // их левый край — над серединой цифры «5» отметки 50
constexpr int kDateWidth = 86;          // дата; СК-42 — той же длины
constexpr int kModeHeight = 24;         // строки КУРС … дата
constexpr int kVrmWidth = 140;
constexpr int kSarpWidth = kVrmWidth / 2; // ЛОД и САРП — в половину блока ВД
// Курсор по высоте — два полублока по 48 px. САРП стоит от верхней границы курсора и чуть
// не доходит до половины, ВД — под ним через обычный зазор, до нижней границы курсора;
// ЛОД той же высоты, что САРП, заканчивается на уровне низа СЛЕДОВ.
constexpr int kCursorHeight = 2 * 48 + kSpacing;
constexpr int kSarpHeight = 44;
constexpr int kVrmHeight = kCursorHeight - kSarpHeight - kSpacing;

constexpr int kSideGap = 8;             // между блоками у круга и колонкой данных (стоят рядом)

// Верх колонки данных согласован со стопкой режимов у круга (без зазоров больше обычного):
// * КУРС ГИРО — чуть выше шкалы дальности, ШИРОТА/ДОЛГОТА — той же высоты, что КУРС ГИРО;
// * низ ОД — чуть выше низа ШИРОТЫ/ДОЛГОТЫ;
// * верх данных цели — на уровне цифры 50 шкалы; СК-42 и дата растягиваются по высоте так,
//   что под датой через обычный зазор — малые индикаторы на линии верха данных цели;
// * строка UTC — вровень с датой и той же высоты; пустой блок над ней добирает разницу.
// Высоты СК-42, даты, UTC и пустого блока задаёт alignBlocksToScale().
constexpr int kSensorsHeight = kRangeHeight + 6;
constexpr int kPositionHeight = kSensorsHeight;
constexpr int kDatumTop = kRangeHeight + kStackSpacing + 3 * (kModeHeight + kStackSpacing); // под КУРС, ОД, ВОДА
static_assert(kRangeHeight + 2 * (kStackSpacing + kModeHeight) < kSensorsHeight + kSpacing + kPositionHeight,
              "низ ОД должен быть выше низа блока ШИРОТА/ДОЛГОТА");

constexpr int kDataColumnWidth = 300;
constexpr int kMessagesGap = 28;        // пустое место между блоком сообщений и СЛЕДАМИ
constexpr int kBearingMarkGap = 14;     // данные цели заканчиваются чуть выше отметки 90° шкалы
constexpr double kTargetTopBearing = 50.0; // верх данных цели — на уровне цифры 50 шкалы
constexpr int kMessagesMinHeight = 60;  // пустой блок сообщений под данными цели
constexpr double kMessagesBottomBearing = 120.0; // его низ — чуть ниже цифры 120
constexpr int kMessagesBelowLabel = 12; // от нижнего края цифры 120
constexpr int kClockIconGap = 6;
constexpr int kResizeMargin = 5;        // поле по краю окна для изменения размера без системной рамки

// Блок в рамке с одной надписью (КУРС, ОД, ЛОД…).
QFrame* createTextPanel(const QString& text, int width, QWidget* parent,
                        Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter, int height = kBlockHeight)
{
    auto* panel = theme::createPanel(parent);
    panel->setFixedSize(width, height);

    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(kPadding, 0, kPadding, 0);

    auto* label = theme::createText(text, panel);
    label->setAlignment(alignment);
    layout->addWidget(label);

    return panel;
}

// Блок из двух ячеек, разделённых вертикальной чертой: «ИЗЛУЧЕНИЕ | ВЫКЛ».
QFrame* createSplitPanel(const QString& name, const QString& state, int width, QWidget* parent,
                         int height = kBlockHeight)
{
    auto* panel = theme::createPanel(parent);
    panel->setFixedSize(width, height);

    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* nameLabel = theme::createText(name, panel);
    nameLabel->setAlignment(Qt::AlignCenter);

    auto* divider = theme::createDivider(panel);

    // Ячейка состояния — по тексту: отступ от перегородки и пустое место примерно в один символ
    // после слова. Остальную длину блока занимает название.
    auto* stateLabel = theme::createText(state, panel);
    stateLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont font = stateLabel->font();
    font.setPixelSize(theme::kFontPx);
    const QFontMetrics metrics(font);
    const int trailingSpace = metrics.horizontalAdvance(QLatin1Char('0'));
    stateLabel->setContentsMargins(kPadding, 0, trailingSpace, 0);
    stateLabel->setFixedWidth(kPadding + metrics.horizontalAdvance(state) + trailingSpace);

    layout->addWidget(nameLabel, 1);
    layout->addWidget(divider);
    layout->addWidget(stateLabel);

    return panel;
}

// Блок в рамке с жёлтым значением (дата, СК-42).
QFrame* createValuePanel(const QString& value, int width, QWidget* parent, QLabel** valueLabel = nullptr,
                         int height = kBlockHeight)
{
    auto* panel = theme::createPanel(parent);
    panel->setFixedSize(width, height);

    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(kPadding, 0, kPadding, 0);

    auto* label = theme::createValue(value, panel);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    if (valueLabel != nullptr)
    {
        *valueLabel = label;
    }
    return panel;
}

// Прозрачный контейнер для блоков, наложенных на угол области ИКО.
QWidget* createCornerStack(QWidget* parent, QVBoxLayout** layout)
{
    auto* stack = new QWidget(parent);
    *layout = new QVBoxLayout(stack);
    (*layout)->setContentsMargins(0, 0, 0, 0);
    (*layout)->setSpacing(kStackSpacing);
    return stack;
}

Qt::Edges edgesAt(const QPoint& pos, const QSize& size)
{
    Qt::Edges edges;
    if (pos.x() < kResizeMargin)
    {
        edges |= Qt::LeftEdge;
    }
    if (pos.x() >= size.width() - kResizeMargin)
    {
        edges |= Qt::RightEdge;
    }
    if (pos.y() < kResizeMargin)
    {
        edges |= Qt::TopEdge;
    }
    if (pos.y() >= size.height() - kResizeMargin)
    {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

Qt::CursorShape cursorFor(Qt::Edges edges)
{
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge))
    {
        return Qt::SizeFDiagCursor;
    }
    if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge))
    {
        return Qt::SizeBDiagCursor;
    }
    if (edges & (Qt::LeftEdge | Qt::RightEdge))
    {
        return Qt::SizeHorCursor;
    }
    if (edges & (Qt::TopEdge | Qt::BottomEdge))
    {
        return Qt::SizeVerCursor;
    }
    return Qt::ArrowCursor;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Системная рамка заменена собственным заголовком: в нём кнопка настроек,
    // которой нет на пульте РЛС (docs/05-operator-ui.md, §5.6).
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("MarineRadarSim"));
    setMinimumSize(1024, 768);
    resize(1280, 1024);

    titleBar_ = new TitleBar(this);
    setMenuWidget(titleBar_);
    connect(titleBar_, &TitleBar::settingsRequested, this, &MainWindow::toggleSettings);


    auto* centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));
    centralWidget->setMouseTracking(true);
    centralWidget->installEventFilter(this);
    setCentralWidget(centralWidget);

    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(kSpacing, kSpacing, kSpacing, kSpacing);

    pages_ = new QStackedWidget(centralWidget);
    workstation_ = createWorkstation();
    settingsPage_ = new SettingsPage(pages_);
    pages_->addWidget(workstation_);
    pages_->addWidget(settingsPage_);
    layout->addWidget(pages_);

    connect(settingsPage_, &SettingsPage::closeRequested, this, &MainWindow::toggleSettings);
    connect(settingsPage_, &SettingsPage::settingsChanged, this, [this](const UiSettings& settings) {
        settings_ = settings;
        settings_.save();
        applySettings();
    });

    // Клавиши из раздела «Управление»: настройки (по умолчанию F10) работают и в полноэкранном
    // режиме, когда заголовка окна нет; полноэкранный режим — F11.
    settingsShortcut_ = new QShortcut(this);
    connect(settingsShortcut_, &QShortcut::activated, this, &MainWindow::toggleSettings);
    fullScreenShortcut_ = new QShortcut(this);
    connect(fullScreenShortcut_, &QShortcut::activated, this, [this] {
        settings_.fullScreen = !settings_.fullScreen;
        settings_.save();
        settingsPage_->setSettings(settings_);
        applySettings();
    });

    settings_ = UiSettings::load();
    settingsPage_->setSettings(settings_);
    applySettings();

    auto* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    clockTimer->start(1000);
    updateClock();
}

QWidget* MainWindow::createWorkstation()
{
    auto* workstation = new QWidget(pages_);

    // Квадрат ИКО во всю высоту у левого края и колонка данных вплотную справа.
    // Размеры задаёт layoutWorkstation(); на широком мониторе композиция центрируется.
    auto* layout = new QHBoxLayout(workstation);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    ppiArea_ = createPpiArea(workstation);
    dataColumn_ = createDataColumn(workstation);
    dataColumn_->setFixedWidth(kDataColumnWidth);

    // Боковые поля на широком мониторе — за границами интерфейса РЛС; в них оформление
    // из настроек (по умолчанию логотип Дальрыбвтуза). На экране 5:4 их ширина нулевая.
    for (int i = 0; i < 2; ++i)
    {
        auto* field = new FillerPanel(workstation, FillerPanel::Frame::None);
        field->setMinimumWidth(0);
        field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        fillers_.append(field);
    }

    layout->addWidget(fillers_[0], 1);
    layout->addWidget(ppiArea_);
    layout->addSpacing(kSideGap);
    layout->addWidget(dataColumn_);
    layout->addWidget(fillers_[1], 1);

    workstation->installEventFilter(this);

    return workstation;
}

void MainWindow::layoutWorkstation()
{
    // Область ИКО: круг во всю высоту + полоса слева под угловые блоки.
    // Всё лишнее на широком мониторе уходит в поля по краям — композиция центрируется.
    const int available = workstation_->width() - kSideGap - kDataColumnWidth;
    ppiArea_->setFixedWidth(std::max(0, std::min(available, workstation_->height() + kLeftStrip)));
}

void MainWindow::alignBlocksToScale()
{
    // Данные цели: верх — на уровне цифры 50 шкалы, низ — чуть выше отметки 90°.
    // Колонка данных и круг начинаются с одной высоты.
    const int targetTop = static_cast<int>(ppi_->bearingLabelCenter(kTargetTopBearing).y());

    // СК-42 и дата делят по высоте место между ВОДОЙ и малыми индикаторами (верх данных цели).
    const int datumAndDate = targetTop - kStackSpacing - kDatumTop - kStackSpacing;
    const int datumHeight = std::max(kModeHeight, datumAndDate / 2);
    const int dateHeight = std::max(kModeHeight, datumAndDate - datumHeight);
    datumPanel_->setFixedHeight(datumHeight);
    datePanel_->setFixedHeight(dateHeight);

    // Строка UTC — вровень с датой; пустой блок над ней добирает разницу.
    const int dateTop = kDatumTop + datumHeight + kStackSpacing;
    clockRow_->setFixedHeight(dateHeight);
    reservedPanel_->setFixedHeight(std::max(0, dateTop - (kSensorsHeight + kPositionHeight + 3 * kSpacing)));

    // Малые индикаторы у круга — верхней границей на уровне верха данных цели.
    smallBoxesLayout_->setContentsMargins(0, targetTop, 0, 0);
    const int targetHeight = std::max(targetPanel_->sizeHint().height(),
                                      static_cast<int>(ppi_->center().y()) - kBearingMarkGap - targetTop);
    targetPanel_->setFixedHeight(targetHeight);

    // Сообщения — вплотную под данными цели, низ — чуть ниже цифры 120 шкалы.
    const int messagesTop = targetTop + targetHeight + kSpacing;
    const int messagesBottom =
        static_cast<int>(ppi_->bearingLabelRect(kMessagesBottomBearing).bottom()) + kMessagesBelowLabel;
    messagesPanel_->setFixedHeight(std::max(kMessagesMinHeight, messagesBottom - messagesTop));

    // Высота строк левого верхнего угла: низ АПЧ и низ Х-ДИАП — прямо над цифрами 320 и 310.
    const auto rowHeightAbove = [this](double bearing, int row) {
        const double labelTop = ppi_->bearingLabelRect(bearing).top() - kAboveLabelGap;
        return static_cast<int>((labelTop - row * kStackSpacing) / (row + 1));
    };
    const int rowHeight = std::clamp(std::min(rowHeightAbove(kControlBearing, kControlRow),
                                              rowHeightAbove(kBandBearing, kBandRow)),
                                     kTopLeftMinHeight, kTopLeftMaxHeight);
    for (QWidget* block : controls_)
    {
        block->setFixedHeight(rowHeight);
    }
    for (QWidget* block : {emissionPanel_, tuningPanel_, bandPanel_, ipzPanel_})
    {
        block->setFixedHeight(rowHeight);
    }

    // Левый верхний угол. Блоки стоят от левого края области ИКО (x = 0 в координатах круга).
    const int controlWidth = std::max(
        kControlWidth, static_cast<int>(ppi_->bearingLabelDigitRect(kControlBearing, 0).center().x()));
    for (QWidget* control : controls_)
    {
        control->setFixedWidth(controlWidth);
    }

    const int tuningWidth = LevelControl::titleWidth(controlWidth);
    const int bandWidth = static_cast<int>(ppi_->bearingLabelDigitRect(kBandBearing, 0).center().x());
    tuningPanel_->setFixedWidth(tuningWidth);
    bandPanel_->setFixedWidth(bandWidth);
    ipzPanel_->setFixedWidth(bandWidth / 2);

    const int emissionRight =
        static_cast<int>(ppi_->bearingLabelCenter(kEmissionBearing).x()) + kEmissionLabelOverrun;
    emissionPanel_->setFixedWidth(std::max(kEmissionMinWidth, emissionRight - controlWidth - kEmissionGap));

    // Правая граница ВН — под цифрой 220 шкалы пеленгов, чуть не доходя до её центра.
    const int eblLabelX = static_cast<int>(ppi_->bearingLabelCenter(kEblRightBearing).x());
    eblPanel_->setFixedWidth(std::max(kEblMinWidth, eblLabelX - kEblLabelInset));

    // Шкала дальности — от правого края до точки чуть левее цифры 30.
    const int rangeLabelX = static_cast<int>(ppi_->bearingLabelCenter(kRangeLeftBearing).x());
    const int rangeWidth = std::max(kRangeMinWidth, ppi_->width() - rangeLabelX + kRangeLabelOverlap);
    rangePanel_->setFixedWidth(rangeWidth);

    // КУРС, ОД, ВОДА — от правого края до середины цифры «5» отметки 50.
    const int modeLeft = static_cast<int>(ppi_->bearingLabelDigitRect(kModeLeftBearing, 0).center().x());
    for (QWidget* panel : modePanels_)
    {
        panel->setFixedWidth(std::max(kModeWidth, ppi_->width() - modeLeft));
    }

    // «0.25» — центром над цифрой «0» отметки 40. Блок прижат к правому краю области круга.
    rangeRingsLabel_->ensurePolished(); // шрифт из таблицы стилей
    const QFontMetrics rangeMetrics(rangeRingsLabel_->font());
    const QString ringsText = rangeRingsLabel_->text();
    const int ringsValueStart = rangeMetrics.horizontalAdvance(ringsText.left(ringsText.indexOf(QLatin1Char('0'))));
    const int ringsValueCenter = ringsValueStart + rangeMetrics.horizontalAdvance(QStringLiteral("0.25")) / 2;
    const int digitCenterX = static_cast<int>(ppi_->bearingLabelDigitRect(kRangeRingsBearing, 1).center().x());
    const int panelLeft = ppi_->width() - rangeWidth;
    rangeRingsLabel_->adjustSize();
    rangeRingsLabel_->move(digitCenterX - panelLeft - ringsValueCenter, 1);
}

QWidget* MainWindow::createPpiArea(QWidget* parent)
{
    auto* area = new QWidget(parent);

    auto* layout = new QGridLayout(area);
    layout->setContentsMargins(0, 0, 0, 0);

    ppi_ = new PpiWidget(area);
    ppi_->setAlignment(Qt::AlignRight);
    ppi_->installEventFilter(this);
    layout->addWidget(ppi_, 0, 0);

    QList<QWidget*> reserved;

    // Верхний левый угол: регуляторы приёмника, состояние излучения, переключатели.
    QVBoxLayout* topLeft = nullptr;
    auto* topLeftStack = createCornerStack(area, &topLeft);

    const std::pair<QString, double> controls[] = {
        {QStringLiteral("УСИЛЕНИЕ"), 0.08},
        {QStringLiteral("ВОЛНЫ"), 0.45},
        {QStringLiteral("ДОЖДЬ"), 0.10},
        {QStringLiteral("АПЧ"), 0.45},
    };
    for (const auto& [title, value] : controls)
    {
        auto* control = new LevelControl(title, value, topLeftStack);
        control->setFixedSize(kControlWidth, kTopLeftHeight);
        controls_.append(control);

        if (title == controls[0].first)
        {
            // В первой строке места больше всего: справа от УСИЛЕНИЯ — состояние излучения.
            auto* row = new QHBoxLayout();
            row->setSpacing(kEmissionGap);
            row->addWidget(control);
            emissionPanel_ = createSplitPanel(QStringLiteral("ИЗЛУЧЕНИЕ"), QStringLiteral("ВКЛ"), kEmissionWidth,
                                              topLeftStack, kTopLeftHeight);
            row->addWidget(emissionPanel_);
            row->addStretch();
            topLeft->addLayout(row);
        }
        else
        {
            topLeft->addWidget(control);
        }
    }

    tuningPanel_ = new TuningIndicator(topLeftStack);
    tuningPanel_->setFixedSize(kTuningWidth, kTopLeftHeight);
    topLeft->addWidget(tuningPanel_);
    bandPanel_ = createTextPanel(QStringLiteral("Х-ДИАП"), kBandWidth, topLeftStack, Qt::AlignCenter, kTopLeftHeight);
    topLeft->addWidget(bandPanel_);
    ipzPanel_ = createTextPanel(QStringLiteral("ИПЗ"), kIpzWidth, topLeftStack, Qt::AlignCenter, kTopLeftHeight);
    topLeft->addWidget(ipzPanel_);

    layout->addWidget(topLeftStack, 0, 0, Qt::AlignTop | Qt::AlignLeft);
    reserved += topLeftStack->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);

    // Нижний левый угол: электронный визир направления.
    auto* eblPanel = theme::createPanel(area);
    eblPanel->setFixedSize(kEblMinWidth, kEblHeight); // длину задаёт updateEblWidth()
    eblPanel_ = eblPanel;

    auto* eblLayout = new QGridLayout(eblPanel);
    eblLayout->setContentsMargins(kPadding + 4, 2, kPadding + 4, kPadding / 2);
    eblLayout->setVerticalSpacing(0);

    auto* eblTitle = theme::createText(QStringLiteral("ВН"), eblPanel);
    eblTitle->setStyleSheet(QStringLiteral("font-size: 24px;"));
    eblTitle->setAlignment(Qt::AlignCenter);

    auto* eblName = theme::createText(QStringLiteral("КУ"), eblPanel);
    eblName->setStyleSheet(QStringLiteral("font-size: 18px;"));

    auto* eblValue = theme::createValue(QStringLiteral("37.1"), eblPanel);
    eblValue->setStyleSheet(QStringLiteral("font-size: 24px;"));

    eblLayout->addWidget(eblTitle, 0, 0, 1, 2);
    eblLayout->addWidget(eblName, 1, 0, Qt::AlignLeft | Qt::AlignBottom);
    eblLayout->addWidget(eblValue, 1, 1, Qt::AlignRight | Qt::AlignBottom);

    layout->addWidget(eblPanel, 0, 0, Qt::AlignBottom | Qt::AlignLeft);
    reserved.append(eblPanel);

    // Верхний правый угол: шкала дальности; ориентация, движение, стабилизация; датум, дата.
    QVBoxLayout* topRight = nullptr;
    auto* topRightStack = createCornerStack(area, &topRight);

    auto* rangePanel = theme::createPanel(topRightStack);
    rangePanel->setFixedSize(kRangeMinWidth, kRangeHeight); // длину задаёт alignBlocksToScale()
    rangePanel_ = rangePanel;
    auto* rangeLayout = new QHBoxLayout(rangePanel);
    rangeLayout->setContentsMargins(kPadding, 0, kPadding, 0);
    rangeLayout->setSpacing(0);
    auto* scale = theme::createValue(QStringLiteral("1.5"), rangePanel);
    scale->setStyleSheet(QStringLiteral("font-size: %1px;").arg(kRangeScalePx));
    scale->setFixedHeight(kRangeHeight - 2);   // внутри рамки
    rangeLayout->addWidget(scale);
    rangeLayout->addStretch();
    rangeLayout->addWidget(theme::createText(QStringLiteral("МИЛЬ"), rangePanel));

    // «/ 0.25» — вне раскладки: положение задаёт alignBlocksToScale() (над цифрой «0» отметки 40).
    rangeRingsLabel_ = theme::createValue(QStringLiteral("/ 0.25"), rangePanel);
    rangeRingsLabel_->setFixedHeight(kRangeHeight - 2);
    topRight->addWidget(rangePanel, 0, Qt::AlignRight);

    for (const QString& mode : {QStringLiteral("КУРС"), QStringLiteral("ОД"), QStringLiteral("ВОДА")})
    {
        auto* modePanel = createTextPanel(mode, kModeWidth, topRightStack, Qt::AlignCenter, kModeHeight);
        modePanels_.append(modePanel);
        topRight->addWidget(modePanel, 0, Qt::AlignRight);
    }
    datumPanel_ = createValuePanel(QStringLiteral("СК-42"), kDateWidth, topRightStack, nullptr, kModeHeight);
    topRight->addWidget(datumPanel_, 0, Qt::AlignRight);
    datePanel_ = createValuePanel({}, kDateWidth, topRightStack, &dateLabel_, kModeHeight);
    topRight->addWidget(datePanel_, 0, Qt::AlignRight);

    layout->addWidget(topRightStack, 0, 0, Qt::AlignTop | Qt::AlignRight);
    reserved += topRightStack->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);

    // Два малых индикатора под датой: по вертикали — на линии верхней границы данных цели
    // (отступ сверху задаёт layoutWorkstation()).
    auto* smallBoxesRow = new QWidget(area);
    smallBoxesLayout_ = new QHBoxLayout(smallBoxesRow);
    smallBoxesLayout_->setContentsMargins(0, 0, 0, 0);
    smallBoxesLayout_->setSpacing(kSideGap / 2);
    for (int i = 0; i < 2; ++i)
    {
        auto* box = theme::createPanel(smallBoxesRow);
        box->setFixedSize(26, 18);
        smallBoxesLayout_->addWidget(box);
        reserved.append(box);
    }
    layout->addWidget(smallBoxesRow, 0, 0, Qt::AlignTop | Qt::AlignRight);

    // Нижний правый угол: ЛОД, САРП, визир дальности.
    QVBoxLayout* bottomRight = nullptr;
    auto* bottomRightStack = createCornerStack(area, &bottomRight);

    bottomRight->setSpacing(0);
    bottomRight->addWidget(
        createTextPanel(QStringLiteral("ЛОД"), kSarpWidth, bottomRightStack, Qt::AlignCenter, kSarpHeight), 0,
        Qt::AlignRight);
    bottomRight->addSpacing(kSpacing); // как между СЛЕДАМИ и курсором
    bottomRight->addWidget(
        createTextPanel(QStringLiteral("САРП"), kSarpWidth, bottomRightStack, Qt::AlignCenter, kSarpHeight), 0,
        Qt::AlignRight);
    bottomRight->addSpacing(kCursorHeight - kVrmHeight - kSarpHeight);

    auto* vrmPanel = theme::createPanel(bottomRightStack);
    vrmPanel->setFixedSize(kVrmWidth, kVrmHeight);
    auto* vrmLayout = new QVBoxLayout(vrmPanel);
    vrmLayout->setContentsMargins(kPadding, 0, kPadding, 0);
    vrmLabel_ = theme::createText({}, vrmPanel);  // текст задаёт applySettings() — цвет цифр зависит от схемы
    vrmLabel_->setAlignment(Qt::AlignCenter);
    vrmLayout->addWidget(vrmLabel_);
    bottomRight->addWidget(vrmPanel, 0, Qt::AlignRight);

    layout->addWidget(bottomRightStack, 0, 0, Qt::AlignBottom | Qt::AlignRight);
    reserved += bottomRightStack->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);

    // Если окно меньше расчётного, круг уменьшается, а не заходит под угловые блоки.
    ppi_->setReservedAreas(reserved);

    return area;
}

QWidget* MainWindow::createDataColumn(QWidget* parent)
{
    auto* column = new QWidget(parent);

    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kSpacing);

    // Данные собственного судна.
    auto* sensors = new DataPanel(column);
    sensors->setCompact(true);
    sensors->addRow(QStringLiteral("курс гиро"), QStringLiteral("170.0"), QStringLiteral("°"));
    sensors->addRow(QStringLiteral("ск лаг (в)"), QStringLiteral("0.0"), QStringLiteral("уз"));
    sensors->setFixedHeight(kSensorsHeight);
    layout->addWidget(sensors);

    auto* position = new DataPanel(column);
    position->setCompact(true);
    position->addRow(QStringLiteral("широта"));
    position->addRow(QStringLiteral("долгота"));
    position->setFixedHeight(kPositionHeight);
    layout->addWidget(position);

    reservedPanel_ = theme::createPanel(column);  // высоту задаёт alignBlocksToScale()
    layout->addWidget(reservedPanel_);

    // Часы — без рамки, по центру колонки, перед временем — значок часов.
    auto* clockRow = new QWidget(column);
    clockRow_ = clockRow;
    clockRow->setFixedHeight(kModeHeight);
    auto* clockLayout = new QHBoxLayout(clockRow);
    clockLayout->setContentsMargins(0, 0, 0, 0);
    clockLayout->setSpacing(kClockIconGap);
    clockLabel_ = theme::createText({}, clockRow);
    clockLayout->addStretch();
    clockLayout->addWidget(new ClockIcon(clockRow), 0, Qt::AlignVCenter);
    clockLayout->addWidget(clockLabel_);
    clockLayout->addStretch();
    layout->addWidget(clockRow);

    // Данные выбранной цели САРП: заголовок и плотные строки под ним, свободное место — внизу.
    auto* target = new DataPanel(column);
    target->setCompact(true);
    targetPanel_ = target;
    target->addRow(QStringLiteral("ВСЕГО ЦЕЛЕЙ САРП"), {}, QStringLiteral("№"));
    target->addBlankLine();
    target->addRow(QStringLiteral("ПЕЛЕНГ"), {}, QStringLiteral("°"));
    target->addRow(QStringLiteral("ДАЛЬНОСТЬ"), {}, QStringLiteral("миль"));
    target->addRow(QStringLiteral("КУРС (В)"), {}, QStringLiteral("°"));
    target->addRow(QStringLiteral("СКОР (В)"), {}, QStringLiteral("уз"));
    target->addRow(QStringLiteral("ДКС"), {}, QStringLiteral("миль"));
    target->addRow(QStringLiteral("ВКС"), {}, QStringLiteral("мин"));
    target->addRow(QStringLiteral("ДПК"), {}, QStringLiteral("миль"));
    target->addRow(QStringLiteral("ВПК"), {}, QStringLiteral("мин"));
    target->addRow(QStringLiteral("ШИРОТА"));
    target->addRow(QStringLiteral("ДОЛГОТА"));
    target->addStretch();
    layout->addWidget(target);

    // Сообщения и тревоги — вплотную под данными цели; свободное место колонки — под ними.
    messagesPanel_ = theme::createPanel(column);
    messagesPanel_->setFixedHeight(kMessagesMinHeight); // высоту задаёт alignBlocksToScale()
    layout->addWidget(messagesPanel_);
    layout->addStretch(1);
    layout->addSpacing(kMessagesGap);

    auto* tracking = new DataPanel(column);
    tracking->addRow(QStringLiteral("СЛЕДЫ/точки"), QStringLiteral("откл"), QStringLiteral("мин"));
    tracking->addRow(QStringLiteral("опасная дкс"), QStringLiteral("2.00"), QStringLiteral("миль"));
    tracking->addRow(QStringLiteral("опасное вкс"), QStringLiteral("30"), QStringLiteral("мин"));
    tracking->addRow(QStringLiteral("вектор"), QStringLiteral("10"), QStringLiteral("мин"));
    tracking->setCompact(true);
    layout->addWidget(tracking);

    auto* cursor = new DataPanel(column);
    cursor->addTitle(QStringLiteral("курсор"));
    cursor->addRow(QStringLiteral("курс.угол"), QStringLiteral("60"), QStringLiteral("°"));
    cursor->addRow(QStringLiteral("дальность"), QStringLiteral("1.00"), QStringLiteral("миль"));
    cursor->addRow(QStringLiteral("широта"));
    cursor->addRow(QStringLiteral("долгота"));
    cursor->setCompact(true);
    cursor->setFixedHeight(kCursorHeight);
    layout->addWidget(cursor);

    return column;
}

void MainWindow::toggleSettings()
{
    const bool open = pages_->currentWidget() != settingsPage_;
    pages_->setCurrentWidget(open ? static_cast<QWidget*>(settingsPage_) : workstation_);
    titleBar_->setSettingsActive(open);
}

void MainWindow::showInitial()
{
    if (settings_.fullScreen)
    {
        titleBar_->setVisible(false);
        showFullScreen();
    }
    else
    {
        showMaximized();
    }
}

void MainWindow::applySettings()
{
    theme::setColorScheme(settings_.colorScheme);
    theme::setFontChoice(settings_.font);
    qApp->setStyleSheet(theme::styleSheet());

    // Виджеты, которые рисуют сами, берут шрифт и цвета не из таблицы стилей.
    ppi_->setFont(theme::font());
    ppi_->setCursor(makeCursor(settings_.cursor, theme::palette().cursor));
    ppi_->setHeadingLineVisible(settings_.headingLine);
    for (FillerPanel* filler : fillers_)
    {
        filler->setFillerStyle(settings_.filler);
    }
    vrmLabel_->setText(QStringLiteral("ВД<br>%1&nbsp;&nbsp;МИЛЬ").arg(theme::valueSpan(QStringLiteral("0.243"))));
    updateClock();

    settingsShortcut_->setKey(settings_.key(key_action::kOpenSettings));
    fullScreenShortcut_->setKey(settings_.key(key_action::kFullScreen));

    if (isVisible() && settings_.fullScreen != isFullScreen())
    {
        setFullScreenMode(settings_.fullScreen);
    }

    // Ширина текста зависит от шрифта — блоки, привязанные к цифрам шкалы, пересчитываются.
    alignBlocksToScale();
    for (QWidget* widget : QApplication::allWidgets())
    {
        widget->update();
    }
}

void MainWindow::setFullScreenMode(bool fullScreen)
{
    titleBar_->setVisible(!fullScreen);
    if (fullScreen)
    {
        showFullScreen();
    }
    else
    {
        showMaximized();
    }
}

void MainWindow::updateClock()
{
    // Пока — системное время; после появления ядра симуляции — время сценария.
    const QDateTime now = QDateTime::currentDateTime();

    const int offsetMinutes = now.offsetFromUtc() / 60;
    const QChar sign = offsetMinutes < 0 ? QLatin1Char('-') : QLatin1Char('+');
    const QString offset = QStringLiteral("%1 %2:%3")
                               .arg(sign)
                               .arg(std::abs(offsetMinutes) / 60, 2, 10, QLatin1Char('0'))
                               .arg(std::abs(offsetMinutes) % 60, 2, 10, QLatin1Char('0'));

    clockLabel_->setText(QStringLiteral("(UTC %1)&nbsp;&nbsp;&nbsp;%2")
                             .arg(theme::valueSpan(offset),
                                  theme::valueSpan(now.toString(QStringLiteral("hh : mm : ss")))));
    dateLabel_->setText(now.toString(QStringLiteral("dd.MM.yy")));
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == workstation_ && event->type() == QEvent::Resize)
    {
        layoutWorkstation();
    }
    else if (watched == ppi_ && event->type() == QEvent::Resize)
    {
        alignBlocksToScale();
    }
    else if (watched == centralWidget() && handleWindowResize(event))
    {
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

bool MainWindow::handleWindowResize(QEvent* event)
{
    // Изменение размера окна за край центрального виджета (системной рамки нет).
    if (isMaximized() || isFullScreen())
    {
        return false;
    }

    if (event->type() == QEvent::MouseMove)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const Qt::Edges edges = edgesAt(mouseEvent->position().toPoint(), centralWidget()->size());
        centralWidget()->setCursor(cursorFor(edges));
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const Qt::Edges edges = edgesAt(mouseEvent->position().toPoint(), centralWidget()->size());
        if (mouseEvent->button() == Qt::LeftButton && edges != Qt::Edges{} && windowHandle() != nullptr)
        {
            windowHandle()->startSystemResize(edges);
            return true;
        }
    }
    else if (event->type() == QEvent::Leave)
    {
        centralWidget()->unsetCursor();
    }

    return false;
}

} // namespace mrs::ui
