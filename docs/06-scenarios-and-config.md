# 06. Сценарии и конфигурация

## 6.1. Виды файлов

| Файл | Формат | Где | Кто пишет |
|------|--------|-----|-----------|
| Модель РЛС | YAML | `data/config/radars/*.yaml` | Разработчик (калибровка) |
| Типы судов | YAML | `data/config/ships/*.yaml` | Разработчик / методист |
| Береговая линия | GeoJSON | `data/coastlines/*.geojson` | Подготовка из открытых данных (OSM/GSHHG) |
| Сценарий (обстановка) | YAML | `data/scenarios/*.yaml` | Методист |
| Учебное задание | YAML | `data/tasks/<уровень>/*.yaml` | Методист |
| Настройки приложения | YAML | `data/config/app.yaml` | Администратор |
| Результат | JSON | `results/` | Программа |
| Запись сеанса | JSON Lines | `results/*.mrsrec` | Программа |

Разделение **сценарий ≠ задание**: один сценарий (обстановка, «пролив с тремя судами») используется
несколькими заданиями («захватить цели», «определить опасную цель», «разойтись»).

Каждый файл содержит `schema_version`. Загрузчик проверяет версию и выдаёт ошибку с файлом и строкой.

## 6.2. Модель РЛС

```yaml
schema_version: 1
id: generic_x_band_25kw
name: "РЛС X-диапазона 25 кВт"
band: X
antenna:
  height_m: 20
  horizontal_beamwidth_deg: 1.2
  vertical_beamwidth_deg: 22
  rotation_rpm: 24
  sidelobe_level_db: -28
transmitter:
  warmup_s: 90
  pulses:                       # имя, длительность, ЧПИ
    - { name: SP, length_us: 0.07, prf_hz: 3000 }
    - { name: MP, length_us: 0.30, prf_hz: 1500 }
    - { name: LP, length_us: 0.80, prf_hz: 750 }
range_scales_nm: [0.25, 0.5, 0.75, 1.5, 3, 6, 12, 24, 48, 96]
auto_pulse_by_scale: { 0.25: SP, 0.5: SP, 0.75: SP, 1.5: SP, 3: MP, 6: MP, 12: LP, 24: LP, 48: LP, 96: LP }
receiver:
  snr_ref_db: 25               # σ = 10 м² на 10 милях при LP
  noise_figure_db: 6
  gain_curve: { k: 2.4, t0_db: 3 }
  stc: { max_range_nm: 6, exponent: 3.5 }
  ftc: { max_time_constant_us: 1.0 }
video:
  spokes_per_revolution: 2048
  range_bins: 1024
tracking:
  max_tracks: 40
  confirm_m_of_n: [3, 5]
  lost_after_scans: 5
  gate_probability: 0.99
clutter:
  sea_state_intensity_db: [ -99, 5, 10, 15, 19, 22, 25 ]   # балл 0…6
  critical_range_nm: 4
```

## 6.3. Сценарий

```yaml
schema_version: 1
id: strait_crossing_01
title: "Пролив, три встречных судна"
seed: 42
start_time: "2026-06-01T10:00:00+10:00"
origin: { lat: 43.000, lon: 131.800 }   # начало локальной проекции
duration_s: 3600
radar: generic_x_band_25kw
coastline: primorye.geojson

environment:
  wind: { from_deg: 270, speed_kn: 15 }
  sea_state: 3
  current: { set_deg: 90, drift_kn: 0.5 }
  rain_cells:
    - { center: { lat: 43.05, lon: 131.95 }, radius_nm: 1.5, rate_mmh: 8, drift_with_wind: true }

own_ship:
  type: bulk_carrier_20k
  position: { lat: 43.010, lon: 131.850 }
  course_deg: 45
  speed_kn: 12
  control: trainee                       # trainee | scripted

targets:
  - id: T1
    type: container_5k
    position: { bearing_deg: 30, range_nm: 8 }   # относительно own_ship в момент старта
    motion: { model: constant, course_deg: 210, speed_kn: 15 }
  - id: T2
    type: fishing_vessel
    position: { lat: 43.080, lon: 131.900 }
    motion:
      model: scripted
      course_deg: 120
      speed_kn: 8
      maneuvers:
        - { at_s: 600, course_deg: 200, turn_rate_dps: 0.5 }
  - id: B1
    type: buoy_reflector
    position: { lat: 43.040, lon: 131.880 }
    motion: { model: static }

events:                                   # события обстановки (не оценивание)
  - { at_s: 1200, action: sensor_fail, sensor: gyro }
  - { at_s: 1500, action: sensor_restore, sensor: gyro }
```

## 6.4. Тип судна

```yaml
schema_version: 1
id: container_5k
name: "Контейнеровоз 5000 т"
length_m: 110
beam_m: 18
height_m: 30
rcs_m2: 20000            # если не задано — по формуле Скольника из displacement_kt
rcs_fluctuation: swerling1
dynamics: { accel_time_s: 180, max_turn_rate_dps: 1.2 }
```

## 6.5. Учебное задание

Подробно о критериях — [07-training-and-assessment.md](07-training-and-assessment.md).

```yaml
schema_version: 1
id: basic_03_acquire_dangerous
level: basic
title: "Определение опасной цели"
scenario: strait_crossing_01
mode: exam                       # training | exam
time_limit_s: 900
time_acceleration_allowed: false
briefing: |
  РЛС в режиме «Готов». Включите излучение, настройте изображение,
  возьмите на сопровождение все цели в пределах 8 миль и определите опасную цель.
initial_state:
  radar_power: standby
  range_scale_nm: 6
  gain: 20                        # заведомо недоусилено — обучаемый должен настроить
criteria:
  - id: tx_on
    type: state
    description: "Включено излучение"
    condition: "radar.power == transmit"
    weight: 1
  - id: acquired_all
    type: state
    description: "Все цели в пределах 8 миль на сопровождении"
    condition: "all_truth_targets(range_nm < 8) are tracked"
    within_s: 600
    weight: 3
  - id: dangerous_identified
    type: answer
    description: "Указана опасная цель"
    expected: { target: T1 }
    weight: 3
  - id: no_gain_saturation
    type: forbidden
    description: "Изображение не перенасыщено шумом более 30 с"
    condition: "radar.noise_fill_ratio > 0.4"
    max_duration_s: 30
    penalty: 1
passing_score: 0.7
```

Язык условий (`condition`) в MVP — **не** интерпретатор выражений, а фиксированный набор
именованных предикатов с параметрами (см. 07, §7.3). Строковая форма выше — для читаемости; загрузчик
сопоставляет её с зарегистрированным предикатом и падает с понятной ошибкой на неизвестном.

## 6.6. Результат (JSON)

```json
{
  "schema_version": 1,
  "app_version": "0.3.0",
  "task_id": "basic_03_acquire_dangerous",
  "scenario_id": "strait_crossing_01",
  "seed": 42,
  "trainee": "Иванов И.И.",
  "started_at": "2026-10-18T09:12:03+03:00",
  "duration_s": 712.4,
  "score": 0.86,
  "passed": true,
  "criteria": [
    { "id": "tx_on", "status": "passed", "at_s": 12.0 },
    { "id": "acquired_all", "status": "passed", "at_s": 304.5 },
    { "id": "dangerous_identified", "status": "passed", "at_s": 690.1 },
    { "id": "no_gain_saturation", "status": "violated", "duration_s": 41.0, "penalty": 1 }
  ],
  "events": [
    { "t": 12.0, "type": "command", "name": "PowerCommand", "args": { "state": "transmit" } },
    { "t": 102.0, "type": "system", "name": "TransmitStarted" }
  ],
  "recording": "results/2026-10-18_091203_basic_03.mrsrec"
}
```

## 6.7. Валидация

* Загрузчик: YAML → промежуточные структуры → проверка (диапазоны, ссылки на `id`, единицы) → доменные объекты.
* Ошибка: `scenarios/strait_crossing_01.yaml:27: targets[1].motion.speed_kn: ожидается число 0…60, получено "fast"`.
* `mrs_headless --validate data/` проверяет все файлы — запускается в CI.
