# 04. Архитектура

## 4.1. Архитектурный стиль

**Модульный монолит** с послойной структурой и строгим направлением зависимостей
(«чистая архитектура»): внутренние слои ничего не знают о внешних.

```text
┌──────────────────────────────────────────────────────────────────┐
│ presentation (Qt Widgets)       ИКО, панели, диалоги             │
├──────────────────────────────────────────────────────────────────┤
│ application                     SessionController, SimulationHost│
│                                 команды, снимки, потоки          │
├───────────────────────┬──────────────────────────────────────────┤
│ training              │ infrastructure                           │
│ задания, оценивание   │ YAML/JSON, файлы, логирование, часы ОС   │
├───────────────────────┴──────────────────────────────────────────┤
│ domain (Simulation Core) — чистый C++, без Qt                    │
│ world · motion · environment · sensors · radar · tracking ·      │
│ system (модель РЛС как прибора)                                  │
├──────────────────────────────────────────────────────────────────┤
│ core — единицы, геометрия, углы, ГСЧ, время, Result/ошибки       │
└──────────────────────────────────────────────────────────────────┘
```

### Правила зависимостей

| Модуль | Может зависеть от | Не может |
|--------|-------------------|----------|
| `core` | STL | всё остальное |
| `domain` | `core`, Eigen | Qt, файлы, логирование напрямую* |
| `training` | `domain` (только чтение снимков и событий), `core` | Qt, изменение состояния мира |
| `infrastructure` | `domain`, `training`, `core`, yaml-cpp, nlohmann_json, spdlog | Qt Widgets |
| `application` | всё нижележащее, Qt Core (потоки, сигналы) | Qt Widgets |
| `presentation` | `application`, Qt Widgets | `domain` напрямую (только через снимки/DTO) |

\* Логирование из domain — через интерфейс `ILogSink`, который реализует infrastructure.

Правила **обеспечиваются сборкой**: каждый модуль — отдельная статическая CMake-библиотека; `domain`
и `training` не линкуются с Qt и не видят его заголовки (NFR-MNT-001).

## 4.2. Модули и ответственность

### core (`src/core`)

* `units.h` — строгие типы `Meters`, `Seconds`, `MetersPerSecond`, `Radians`; литералы `_nm`, `_kn`, `_deg`.
* `angle.h` — `Bearing` (0…2π, нормализация, разность углов с переходом через 0).
* `geo.h` — `GeoPoint {lat, lon}`, `LocalProjection` (WGS-84 ↔ ENU).
* `vec2.h` — 2D-вектор (свой лёгкий тип; Eigen — только в фильтрах).
* `rng.h` — детерминированный ГСЧ (`std::mt19937_64` + собственные распределения: стандартные
  распределения STL не гарантируют одинаковый результат на разных компиляторах).
* `sim_time.h` — `Tick` (int64), `SimTime`.
* `result.h` — `Result<T> = std::expected<T, Error>`.

### domain (`src/domain`)

| Подмодуль | Ключевые классы | Ответственность |
|-----------|-----------------|-----------------|
| `world` | `World`, `Entity`, `OwnShip`, `Vessel`, `Buoy`, `Coastline` | Хранилище объектов мира, пространственный индекс |
| `environment` | `Environment`, `Wind`, `SeaState`, `RainCell`, `Current` | Внешние условия |
| `motion` | `IMotionModel`, `ConstantMotion`, `RouteMotion`, `ScriptedMotion`, `ShipDynamics` | Интегрирование движения |
| `sensors` | `Gyro`, `Log`, `Gnss`, `SensorSuite` | Показания датчиков собственного судна с ошибками |
| `radar` | `Antenna`, `Transmitter`, `Receiver`, `VideoGenerator`, `ClutterModel`, `CoastEchoModel`, `Spoke` | Формирование видеосигнала (см. 03, §3.5) |
| `tracking` | `PlotExtractor`, `Tracker`, `Track`, `KalmanCV`, `CpaCalculator`, `GuardZone`, `TrialManoeuvre` | САРП |
| `system` | `RadarSystem`, `PowerStateMachine`, `DisplaySettings`, `Ebl`, `Vrm`, `AlarmManager` | Модель прибора: режимы, органы управления, тревоги |
| `simulation` | `Simulation`, `Scenario`, `ScenarioEventQueue`, `Command`, `SimulationSnapshot` | Такт симуляции, применение команд, публикация состояния |

`Simulation` — фасад ядра:

```cpp
namespace mrs::domain {

class Simulation {
public:
    explicit Simulation(ScenarioDefinition scenario, RadarModelConfig radar, std::uint64_t seed);

    void apply(const Command& command);        // команда к текущему такту
    void step();                               // ровно один фиксированный шаг dt

    [[nodiscard]] Tick tick() const noexcept;
    [[nodiscard]] SimulationSnapshot snapshot() const;          // копия для UI/оценивания
    [[nodiscard]] std::span<const Spoke> spokesProduced() const; // спицы, сгенерированные в этом шаге
    [[nodiscard]] std::span<const DomainEvent> events() const;   // события этого шага

private:
    World world_;
    Environment environment_;
    SensorSuite sensors_;
    RadarSystem radar_;
    Tracker tracker_;
    ScenarioEventQueue scenarioEvents_;
    Tick tick_{0};
};

} // namespace mrs::domain
```

### Команды (вход в ядро)

Все действия оператора и инструктора — **значения-команды**, а не вызовы методов из UI.
Это даёт запись/воспроизведение, тесты и потокобезопасность бесплатно.

```cpp
using Command = std::variant<
    PowerCommand,          // Off / Standby / Transmit
    SetRangeScale,         // индекс шкалы
    SetGain, SetSeaClutter, SetRainClutter, SetTuning,
    SetOrientation,        // NorthUp / CourseUp / HeadUp
    SetMotionMode,         // Relative / True
    SetEbl, SetVrm,
    AcquireTarget,         // экранная точка → мировые координаты, пересчитывает application
    DropTarget, DropAllTargets,
    SetCpaLimits, SetVectorTime, SetVectorMode,
    SetOwnShipOrder,       // заданный курс/скорость
    AcknowledgeAlarm
>;
```

Каждая команда журналируется с номером такта: `{tick, command}` — это и есть запись сеанса.

### training (`src/training`)

* `TrainingTask` — описание задания (загружается из YAML).
* `TaskRunner` — на каждом такте получает `SimulationSnapshot` + события + команды и обновляет прогресс.
* `Criterion` (интерфейс) и реализации: `StateCriterion`, `SequenceCriterion`, `TimeLimitCriterion`,
  `MeasurementAccuracyCriterion`, `ForbiddenActionCriterion`.
* `Assessment` — итоговая оценка, `AssessmentReport` (DTO для сохранения).

Подробно — в [07-training-and-assessment.md](07-training-and-assessment.md).

### infrastructure (`src/infrastructure`)

* `config/` — загрузчики YAML → структуры domain/training, валидация с позицией ошибки.
* `storage/` — запись результатов (JSON), запись/чтение журнала сеанса (`.mrsrec`, JSON Lines).
* `logging/` — настройка spdlog, реализация `ILogSink`.
* `paths/` — расположение каталогов `config/`, `scenarios/`, `results/`, `logs/` относительно исполняемого файла.

### application (`src/application`)

* `SimulationHost` — владеет `Simulation` и потоком симуляции, реализует часы реального времени,
  паузу и ускорение.
* `SessionController` — жизненный цикл сеанса: выбрать задание → загрузить → старт/пауза/сброс → завершить
  → сохранить отчёт.
* `CommandBus` — потокобезопасная очередь команд UI → симуляция.
* `SnapshotChannel` — передача последнего снимка симуляция → UI.
* `SpokeChannel` — передача спиц видео симуляция → UI (кольцевой буфер).
* `ScreenGeometry` — пересчёт экран ↔ мир с учётом ориентации, смещения центра, шкалы
  (нужен и UI, и для команды `AcquireTarget`).

### presentation (`src/presentation`)

* `MainWindow` — компоновка рабочего места (текущий макет в `main_window.cpp`).
* `PpiWidget` — ИКО: растр видео, кольца, ЭВН/ПКД, векторы, символы целей, курсор.
* `controls/` — виджеты органов управления (регулятор, кнопка с подсветкой, переключатель).
* `panels/` — информационные панели (цель, курсор, часы, собственное судно, тревоги).
* `training/` — окно задания, прогресс, итоговый отчёт.
* `viewmodels/` — преобразование снимков в строки/форматы отображения (широта, «КУ 37.1 ПБ» и т. п.).

Подробно — в [05-operator-ui.md](05-operator-ui.md).

## 4.3. Потоки и обмен данными

```text
          GUI thread (Qt event loop)                     Simulation thread
 ┌────────────────────────────────────┐        ┌──────────────────────────────────┐
 │ MainWindow, PpiWidget, панели      │        │ SimulationHost::run()            │
 │                                    │Command │   while (running):               │
 │ действие оператора ──► CommandBus ─┼───────►│     cmds = bus.drain()           │
 │                                    │  MPSC  │     for each due tick:           │
 │                                    │        │        sim.apply(cmds…)          │
 │ QTimer 60 Гц:                      │Snapshot│        sim.step()                │
 │   snap = SnapshotChannel.latest() ◄┼────────┤        taskRunner.update(...)    │
 │   spokes = SpokeChannel.drain()   ◄┼────────┤        publish snapshot, spokes  │
 │   PpiWidget.update()               │ SPSC   │     sleep_until(next real tick)  │
 └────────────────────────────────────┘        └──────────────────────────────────┘
```

* **CommandBus**: `std::mutex` + `std::vector<Command>` с обменом буферов (команд мало, lock-free не нужен).
* **SnapshotChannel**: `std::atomic<std::shared_ptr<const SimulationSnapshot>>` (C++20) или mutex + `shared_ptr`.
  UI всегда берёт последний снимок; промежуточные можно пропускать.
* **SpokeChannel**: SPSC кольцевой буфер спиц фиксированного размера (≥ 1 оборот). Спицы **нельзя
  пропускать** — иначе на ИКО будут «дыры»; при переполнении (UI завис) — сброс с перерисовкой оборота.
* **Время**: симуляция — ведущая. Реальное время → «долг» тактов: `due = (now − start) · speed / dt − tick`.
  При паузе тактов нет. При отставании выполняется не более N тактов за итерацию (защита от «спирали
  смерти»), отставание логируется.
* Qt-сигналы между потоками не используются для потоковых данных (спицы, снимки) — только для редких
  событий (сеанс завершён, ошибка).

## 4.4. Снимок состояния (Snapshot)

UI и оценивание не видят внутренних объектов ядра. Они получают неизменяемый DTO:

```cpp
struct SimulationSnapshot {
    Tick tick;
    SimTime time;
    RadarStatus radar;            // питание, прогрев, шкала, импульс, регулировки, ориентация, режим движения
    OwnShipView ownShip;          // по датчикам: курс, скорость, координаты; + истинное (для инструктора/оценки)
    std::vector<TrackView> tracks;// номер, состояние, позиция, вектор, ДКС/ВКС/ДПК/ВПК, прошлые позиции
    std::vector<AlarmView> alarms;
    EblVrmView eblVrm;
    GuardZonesView guardZones;
    GroundTruthView truth;        // истинные положения целей — только для training и режима инструктора
};
```

Снимок ~ десятки КБ, публикуется ≤ 50 раз/с — копирование дешёвое.

## 4.5. Отрисовка ИКО (кратко)

Полярный растр (2048 × 1024, `uint8`) хранится в UI-потоке и обновляется пришедшими спицами.
Каждый кадр: преобразование «полярный → экранный» через предвычисленную таблицу (LUT) пиксель → (спица,
ячейка), пересчитываемую только при изменении размера, ориентации, смещения центра. Послесвечение — второй
растр «возраст ячейки». MVP — CPU (`QImage`); при нехватке производительности — `QOpenGLWidget` +
фрагментный шейдер с той же логикой (ADR-0004). Подробно — [05-operator-ui.md](05-operator-ui.md).

## 4.6. Детерминизм, запись и воспроизведение

* Ядро не читает системные часы и не использует неинициализированную случайность.
* Нет `std::unordered_map` с итерацией в порядке, влияющем на результат; нет зависимости от адресов.
* Числа с плавающей точкой: одинаковые флаги компиляции, без `-ffast-math`.
* Запись сеанса = `scenario + seed + версия + [{tick, command}]`. Воспроизведение — новая `Simulation`
  и подача тех же команд на тех же тактах. Перемотка назад = повтор с нуля до нужного такта
  (при 50 Гц и ускорении без видео — секунды на час сеанса); при необходимости — контрольные точки.
* Тест детерминизма в CI: прогон сценария дважды → сравнение хэшей снимков на каждом 100-м такте.

## 4.7. Обработка ошибок

| Ситуация | Механизм |
|----------|----------|
| Ошибка в YAML / несуществующий файл | `Result<T>` с `Error{code, message, file, line}` → диалог с текстом, приложение работает |
| Нарушение инварианта в ядре (баг) | `MRS_ASSERT` → лог + аварийная остановка в Debug, лог + остановка сеанса в Release |
| Отставание симуляции | Предупреждение в лог, индикатор на экране |
| Исключения | Только на границах (загрузка, файлы); в такте симуляции — без исключений |

## 4.8. Логирование

* spdlog, логгеры по подсистемам: `app`, `sim`, `radar`, `tracking`, `training`, `ui`, `io`.
* Файл `logs/mrs_YYYYMMDD_HHMMSS.log`, ротация по размеру 10 МБ × 5.
* Журнал событий сеанса (для обучаемого) — **не** технический лог, это `DomainEvent` → панель событий + отчёт.

## 4.9. Структура каталогов

```text
MarineRadarSim/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/                       # модули: warnings, sanitizers, зависимости
├── src/
│   ├── app/main.cpp             # точка входа GUI
│   ├── core/
│   ├── domain/
│   │   ├── world/  environment/  motion/  sensors/
│   │   ├── radar/  tracking/  system/  simulation/
│   ├── training/
│   ├── infrastructure/
│   │   ├── config/  storage/  logging/  paths/
│   ├── application/
│   └── presentation/
│       ├── main_window/  ppi/  controls/  panels/  training/  viewmodels/
├── tools/
│   └── headless/                # mrs_headless: прогон сценария без GUI (тесты, проверка сценариев)
├── tests/
│   ├── unit/                    # по модулям: core, domain/*, training
│   ├── integration/             # загрузка сценариев, полный такт, детерминизм
│   └── data/                    # тестовые сценарии
├── data/                        # поставляется с программой
│   ├── config/radars/           # модели РЛС
│   ├── config/ships/            # типы судов (ЭПР, размеры, манёвренность)
│   ├── coastlines/
│   ├── scenarios/
│   └── tasks/{basic,intermediate,advanced}/
├── docs/
└── (runtime) results/  logs/
```

## 4.10. CMake-цели

| Цель | Тип | Зависимости |
|------|-----|-------------|
| `mrs_core` | static | — |
| `mrs_domain` | static | `mrs_core`, Eigen3 |
| `mrs_training` | static | `mrs_domain` |
| `mrs_infrastructure` | static | `mrs_domain`, `mrs_training`, yaml-cpp, nlohmann_json, spdlog |
| `mrs_application` | static | все выше, Qt6::Core |
| `mrs_presentation` | static | `mrs_application`, Qt6::Widgets (+ Qt6::OpenGLWidgets — этап 2) |
| `MarineRadarSim` | executable | `mrs_presentation` |
| `mrs_headless` | executable | `mrs_infrastructure`, `mrs_training` (без Qt) |
| `mrs_tests` | executable | GoogleTest, всё кроме presentation |

## 4.11. Расширяемость (без реализации в MVP)

| Будущее | Точка расширения |
|---------|------------------|
| AIS | Новый модуль `domain/ais`, цели получают `AisTransponder`; новые поля в снимке |
| Рабочее место инструктора | Второе окно в presentation, работающее с теми же командами/снимками |
| Другая модель РЛС | Новый `data/config/radars/*.yaml`; при другом интерфейсе — новый набор панелей presentation |
| Сеть (несколько мест) | Command/Snapshot уже сериализуемы — транспорт добавляется в application |
| Электронная карта | Слой отрисовки в `PpiWidget`, данные — `infrastructure/charts` |
