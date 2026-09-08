# ME Vehicle Bounds Toolkit

Standalone Arma Reforger Workbench tooling for measuring deterministic, per-prefab vehicle bounds and detecting changes between game builds.

---

## English

### Purpose

`ME Vehicle Bounds Toolkit` provides a versioned World Editor fixture and regression workflow for base-game vehicle prefabs. It answers two questions:

1. Does the fixture still cover every enabled vehicle prefab from the supported faction catalogs?
2. Did the world-space bounds of any covered vehicle prefab change after a game update or fixture change?

The addon is intended for addon developers who need stable vehicle dimensions for editor tooling, placement validation, previews, spacing calculations, or regression diagnostics.

### Scope

The toolkit:

- depends only on the Arma Reforger base game;
- reads faction-specific `VEHICLE` catalogs for `CIV`, `FIA`, `US`, and `USSR`;
- contains one unrotated fixture root for every canonical prefab in the catalog union;
- currently validates exactly 146 unique vehicle prefabs;
- measures each root with its children and stores local minimum and maximum bounds;
- records every faction and vehicle-type membership associated with each prefab;
- writes deterministic Candidate snapshots;
- compares the Candidate with a manually accepted Baseline;
- provides fixture coverage and canonical-name validation tools.

The toolkit does **not**:

- spawn ambient vehicles;
- modify runtime vehicle behavior;
- provide a gameplay system or playable scenario;
- use a global or factionless vehicle-catalog fallback;
- automatically overwrite or accept the Baseline;
- depend on `ME_Vehicle_Spawn` or `ME_Vehicle_Spawn_Test`.

### Project identity

| Property | Value |
|---|---|
| Addon | `ME_Vehicle_Bounds_Toolkit` |
| Project ID | `MEVehicleBoundsToolkit` |
| Project GUID | `AEEBCCA53D2E81BA` |
| Dependency | Arma Reforger base game (`58D0FB3206B6F859`) |
| Script prefix | `ME_VBT_` |
| Diagnostic prefix | `[ME_VBT_WB]` |
| Workbench category | `ME Vehicle Bounds Toolkit` |

### Requirements

- Arma Reforger Tools with Workbench.
- The addon project opened from:

  ```text
  ME_Vehicle_Bounds_Toolkit/addon.gproj
  ```

- The fixture world opened in the World Editor:

  ```text
  Worlds/ME_VBT_VehicleBoundsFixture.ent
  ```

The tools are World Editor plugins. Entering Play mode is not required.

### Quick start

1. Open `ME_Vehicle_Bounds_Toolkit/addon.gproj` in Workbench.
2. Open `Worlds/ME_VBT_VehicleBoundsFixture.ent`.
3. Wait until the world and all vehicle resources finish loading.
4. Run the following plugins from `Plugins > ME Vehicle Bounds Toolkit`:
   1. `VBT: Preflight fixture names`
   2. `VBT: Validate fixture coverage`
   3. `VBT: Generate bounds snapshot`
5. Inspect `script.log` or the Workbench console for messages beginning with `[ME_VBT_WB]`.
6. Review any Candidate/Baseline differences before accepting a new Baseline.

### Workbench plugins

#### `VBT: Preflight fixture names`

Performs a read-only validation of fixture-root names.

For every marked vehicle root, the expected editor name is derived from the terminal prefab filename. The preflight detects invalid prefab paths, duplicate target names, and conflicts with other entities. It prints every current-to-target mapping and does not modify the world.

Expected success message:

```text
[ME_VBT_WB] fixture_names status=PASS phase=preflight markers=146 unchanged=146
```

#### `VBT: Apply fixture names`

Runs the same complete preflight and then applies all required canonical names as one undoable World Editor action.

Use this only when the preflight reports valid mappings and one or more roots need renaming. Save the world manually after reviewing the result.

#### `VBT: Validate fixture coverage`

Validates exact one-to-one coverage between the fixture roots and the union of enabled entries from the four declared faction-specific `VEHICLE` catalogs.

The check fails if a marker or faction scope is missing, duplicated, empty, or unexpected; if a fixture root is rotated; or if a catalog prefab has no matching fixture root.

Expected success message:

```text
[ME_VBT_WB] fixture_coverage status=PASS markers=146 scopes=4 catalog_union=146
```

#### `VBT: Generate bounds snapshot`

Runs the complete regression pipeline:

1. validates the fixture inventory;
2. resolves the four faction-specific vehicle catalogs;
3. validates exact catalog coverage;
4. measures world bounds with children for every fixture root;
5. converts the bounds to coordinates local to the unrotated root;
6. builds a deterministically sorted Candidate model;
7. writes and rebuilds the registered Candidate resource;
8. reloads the Candidate and verifies semantic equivalence;
9. loads the accepted Baseline;
10. reports `PASS`, `DIFF`, or `FAIL`.

The generator writes only:

```text
Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf
```

It never modifies:

```text
Configs/Generated/ME_VBT_VehicleBoundsPerPrefabBaseline.conf
```

A cold measurement can take significantly longer than a Workbench automation bridge timeout. A timeout does not by itself prove failure; check the final `[ME_VBT_WB]` messages in `script.log`.

### Snapshot contents

Each snapshot records:

- schema version;
- generator version;
- fixture identity;
- game build version;
- canonical prefab resource path;
- local minimum bounds;
- local maximum bounds;
- sorted faction keys;
- sorted vehicle-type labels.

Entries are sorted by canonical prefab path. Serialized entry containers use stable names derived from prefab filenames, which keeps text diffs readable.

### Candidate and Baseline workflow

#### Candidate

The Candidate is generator-owned output representing the current game build and fixture state. Running the generator may replace its contents.

Do not make manual authoritative changes to the Candidate.

#### Baseline

The Baseline is the manually accepted reference. The generator only reads it.

To accept an intentional change:

1. run the generator;
2. confirm that Candidate save and reload validation passed;
3. inspect the semantic comparison and the text diff;
4. verify that added, removed, or changed prefabs are expected;
5. manually replace the Baseline payload with the reviewed Candidate payload;
6. preserve the existing Baseline filename and `.meta` file;
7. restart or reload Workbench resources if the old Baseline is cached;
8. run the generator again and confirm `snapshot_compare status=PASS`;
9. run it a second time to verify deterministic output.

Never copy the Candidate `.meta` file over the Baseline `.meta` file. They are independent registered resources with different GUIDs.

### Result interpretation

#### `PASS`

The operation completed successfully. For snapshot comparison, the Candidate and Baseline are semantically identical.

#### `DIFF`

The Candidate is valid, but differs from the accepted Baseline. Review all added, removed, and changed records. A `DIFF` after a game update can be expected, but it must not be accepted automatically.

#### `FAIL`

The workflow could not produce a trustworthy comparison. The diagnostic includes a stable `reason=...` value. Fix the fixture, catalog, resource, or validation problem before considering snapshot changes.

### Fixture contract

The current fixture version requires:

| Faction key | Vehicle roots |
|---|---:|
| `CIV` | 58 |
| `FIA` | 20 |
| `US` | 46 |
| `USSR` | 22 |
| **Unique total** | **146** |

A prefab included by more than one faction catalog is measured once and retains all applicable faction and vehicle-type memberships in its snapshot entry.

Fixture roots must remain unrotated. Measurements are stored relative to each root origin, so rotation would make the local axis-aligned result invalid for this fixture contract.

### Fixture structure

```text
ME_Vehicle_Bounds_Toolkit/
├── addon.gproj
├── Configs/Generated/
│   ├── ME_VBT_VehicleBoundsPerPrefabBaseline.conf
│   └── ME_VBT_VehicleBoundsPerPrefabCandidate.conf
├── Scripts/Game/
│   ├── Components/
│   ├── Configs/
│   └── Faction/
├── Scripts/WorkbenchGame/WorldEditor/
└── Worlds/
    ├── ME_VBT_VehicleBoundsFixture.ent
    └── ME_VBT_VehicleBoundsFixture_Layers/
        ├── default.layer
        ├── managers.layer
        ├── Scopes.layer
        ├── CIV.layer
        ├── FIA.layer
        ├── US.layer
        └── USSR.layer
```

### What the main components are for

- `ME_VBT_VehicleBoundsFixtureMarkerComponent` associates a placed fixture root with its canonical catalog prefab path.
- `ME_VBT_VehicleBoundsFactionScopeComponent` declares one required faction key whose `VEHICLE` catalog must be resolved.
- `ME_VBT_EditorFactionCatalogInitialization` makes faction catalogs available to editor-only tooling when required.
- `ME_VBT_VehicleBoundsFixtureInventory` defines the fixture version contract and validates all marker and scope roots.
- `ME_VBT_VehicleBoundsCatalogResolver` builds the deterministic union of enabled faction-specific catalog entries and their memberships.
- `ME_VBT_VehicleBoundsPerPrefabSnapshot` defines the serialized snapshot schema.
- The World Editor plugins expose validation, naming, generation, reload validation, and comparison operations.

### Updating the fixture after a game change

If coverage validation reports catalog changes:

1. identify every added or removed canonical prefab in the diagnostics;
2. add or remove the corresponding fixture root in the correct faction layer;
3. attach a marker component with the exact canonical catalog prefab path;
4. keep the fixture root unrotated;
5. update the expected fixture count and fixture identity only when intentionally creating a new fixture version;
6. run canonical-name preflight;
7. run coverage validation until it passes;
8. generate and review a new Candidate;
9. accept a new Baseline only after the differences are understood.

Do not add a global catalog fallback to hide a missing faction, manager, scope, or catalog. Missing required inputs are deliberate failures.

### Diagnostics

Search Workbench `script.log` for:

```text
[ME_VBT_WB]
```

Useful result families include:

```text
fixture_names
fixture_coverage
snapshot_candidate
snapshot_compare
```

The toolkit scripts compile in the `Game` and `WorkbenchGame` modules, while the commands themselves are registered for the `WorldEditor` module.

---

## Русский

### Назначение

`ME Vehicle Bounds Toolkit` предоставляет версионируемый тестовый мир World Editor и процесс регрессионной проверки prefab-файлов техники из базовой игры. Toolkit отвечает на два вопроса:

1. Покрывает ли fixture все включённые prefab техники из поддерживаемых каталогов фракций?
2. Изменились ли пространственные границы какой-либо техники после обновления игры или изменения fixture?

Addon предназначен для разработчиков, которым нужны стабильные размеры техники для редакторских инструментов, проверки размещения, предпросмотра, расчёта интервалов или регрессионной диагностики.

### Область ответственности

Toolkit:

- зависит только от базовой игры Arma Reforger;
- читает faction-specific каталоги `VEHICLE` для `CIV`, `FIA`, `US` и `USSR`;
- содержит по одному неповёрнутому fixture root для каждого канонического prefab из объединения каталогов;
- сейчас проверяет ровно 146 уникальных prefab техники;
- измеряет каждый root вместе с дочерними сущностями и сохраняет локальные минимальные и максимальные границы;
- записывает все принадлежности prefab к фракциям и типам техники;
- создаёт детерминированные Candidate snapshots;
- сравнивает Candidate с вручную принятым Baseline;
- предоставляет инструменты проверки покрытия и канонических имён fixture.

Toolkit **не**:

- создаёт ambient vehicles;
- изменяет поведение техники во время игры;
- является игровой системой или игровым сценарием;
- использует global или factionless fallback для каталога техники;
- перезаписывает или принимает Baseline автоматически;
- зависит от `ME_Vehicle_Spawn` или `ME_Vehicle_Spawn_Test`.

### Идентификаторы проекта

| Свойство | Значение |
|---|---|
| Addon | `ME_Vehicle_Bounds_Toolkit` |
| Project ID | `MEVehicleBoundsToolkit` |
| Project GUID | `AEEBCCA53D2E81BA` |
| Зависимость | Базовая игра Arma Reforger (`58D0FB3206B6F859`) |
| Префикс скриптов | `ME_VBT_` |
| Префикс диагностики | `[ME_VBT_WB]` |
| Категория Workbench | `ME Vehicle Bounds Toolkit` |

### Требования

- Arma Reforger Tools с Workbench.
- Проект addon должен быть открыт из:

  ```text
  ME_Vehicle_Bounds_Toolkit/addon.gproj
  ```

- В World Editor должен быть открыт fixture-мир:

  ```text
  Worlds/ME_VBT_VehicleBoundsFixture.ent
  ```

Инструменты реализованы как плагины World Editor. Переходить в Play mode не требуется.

### Быстрый запуск

1. Откройте `ME_Vehicle_Bounds_Toolkit/addon.gproj` в Workbench.
2. Откройте `Worlds/ME_VBT_VehicleBoundsFixture.ent`.
3. Дождитесь полной загрузки мира и ресурсов техники.
4. Запустите следующие плагины через `Plugins > ME Vehicle Bounds Toolkit`:
   1. `VBT: Preflight fixture names`
   2. `VBT: Validate fixture coverage`
   3. `VBT: Generate bounds snapshot`
5. Проверьте `script.log` или консоль Workbench на сообщения с префиксом `[ME_VBT_WB]`.
6. Изучите все различия Candidate/Baseline до принятия нового Baseline.

### Плагины Workbench

#### `VBT: Preflight fixture names`

Выполняет проверку имён fixture roots без изменения мира.

Для каждого помеченного root ожидаемое имя в редакторе создаётся из имени файла prefab. Проверка находит некорректные пути prefab, повторяющиеся целевые имена и конфликты с другими сущностями. Она выводит каждое сопоставление текущего и целевого имени, но ничего не изменяет.

Ожидаемое сообщение об успехе:

```text
[ME_VBT_WB] fixture_names status=PASS phase=preflight markers=146 unchanged=146
```

#### `VBT: Apply fixture names`

Выполняет ту же полную предварительную проверку, а затем применяет все необходимые канонические имена как одно отменяемое действие World Editor.

Используйте этот инструмент только тогда, когда preflight подтвердил корректность сопоставлений и один или несколько roots требуют переименования. После проверки результата сохраните мир вручную.

#### `VBT: Validate fixture coverage`

Проверяет точное взаимно-однозначное соответствие между fixture roots и объединением включённых записей из четырёх объявленных faction-specific каталогов `VEHICLE`.

Проверка завершается ошибкой, если marker или faction scope отсутствует, повторяется, пуст или является неожиданным; если fixture root повёрнут; либо если для prefab из каталога нет соответствующего fixture root.

Ожидаемое сообщение об успехе:

```text
[ME_VBT_WB] fixture_coverage status=PASS markers=146 scopes=4 catalog_union=146
```

#### `VBT: Generate bounds snapshot`

Выполняет полный процесс регрессионной проверки:

1. проверяет inventory fixture;
2. разрешает четыре faction-specific каталога техники;
3. проверяет точное покрытие каталогов;
4. измеряет world bounds вместе с дочерними сущностями для каждого fixture root;
5. переводит границы в координаты относительно неповёрнутого root;
6. создаёт детерминированно отсортированную модель Candidate;
7. записывает и перестраивает зарегистрированный ресурс Candidate;
8. повторно загружает Candidate и проверяет семантическое равенство;
9. загружает принятый Baseline;
10. выводит результат `PASS`, `DIFF` или `FAIL`.

Генератор записывает только:

```text
Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf
```

Он никогда не изменяет:

```text
Configs/Generated/ME_VBT_VehicleBoundsPerPrefabBaseline.conf
```

Первое измерение после холодной загрузки может выполняться дольше тайм-аута автоматизации Workbench. Сам по себе тайм-аут не доказывает ошибку — проверяйте итоговые сообщения `[ME_VBT_WB]` в `script.log`.

### Содержимое snapshot

Каждый snapshot хранит:

- версию схемы;
- версию генератора;
- идентификатор fixture;
- версию сборки игры;
- канонический путь ресурса prefab;
- локальные минимальные границы;
- локальные максимальные границы;
- отсортированные ключи фракций;
- отсортированные labels типов техники.

Записи отсортированы по каноническому пути prefab. Контейнеры записей получают стабильные имена из имён файлов prefab, благодаря чему текстовые diff остаются читаемыми.

### Работа с Candidate и Baseline

#### Candidate

Candidate — результат генератора, отражающий текущую сборку игры и состояние fixture. Повторный запуск генератора может заменить его содержимое.

Не вносите в Candidate ручные изменения, которые должны считаться эталонными.

#### Baseline

Baseline — вручную принятый эталон. Генератор только читает его.

Чтобы принять ожидаемое изменение:

1. запустите генератор;
2. убедитесь, что сохранение и повторная загрузка Candidate завершились успешно;
3. изучите семантическое сравнение и текстовый diff;
4. убедитесь, что добавленные, удалённые или изменённые prefab ожидаемы;
5. вручную замените содержимое Baseline проверенным содержимым Candidate;
6. сохраните существующие имя файла Baseline и его `.meta`;
7. перезапустите Workbench или перезагрузите ресурсы, если старый Baseline остался в кэше;
8. снова запустите генератор и получите `snapshot_compare status=PASS`;
9. запустите генератор ещё раз, чтобы подтвердить детерминированность результата.

Никогда не копируйте `.meta` Candidate поверх `.meta` Baseline. Это независимые зарегистрированные ресурсы с разными GUID.

### Интерпретация результатов

#### `PASS`

Операция завершилась успешно. Для сравнения snapshots это означает, что Candidate и Baseline семантически идентичны.

#### `DIFF`

Candidate корректен, но отличается от принятого Baseline. Необходимо проверить все добавленные, удалённые и изменённые записи. После обновления игры `DIFF` может быть ожидаемым, но принимать его автоматически нельзя.

#### `FAIL`

Процесс не смог выполнить надёжное сравнение. Диагностика содержит стабильное значение `reason=...`. До рассмотрения изменений snapshot необходимо исправить проблему fixture, каталога, ресурса или проверки.

### Контракт fixture

Текущая версия fixture требует:

| Ключ фракции | Fixture roots техники |
|---|---:|
| `CIV` | 58 |
| `FIA` | 20 |
| `US` | 46 |
| `USSR` | 22 |
| **Всего уникальных** | **146** |

Prefab, который присутствует в каталогах нескольких фракций, измеряется один раз и сохраняет все применимые принадлежности к фракциям и типам техники в своей записи snapshot.

Fixture roots должны оставаться неповёрнутыми. Измерения сохраняются относительно origin каждого root, поэтому поворот сделает локальный axis-aligned результат некорректным для этого контракта fixture.

### Структура fixture

```text
ME_Vehicle_Bounds_Toolkit/
├── addon.gproj
├── Configs/Generated/
│   ├── ME_VBT_VehicleBoundsPerPrefabBaseline.conf
│   └── ME_VBT_VehicleBoundsPerPrefabCandidate.conf
├── Scripts/Game/
│   ├── Components/
│   ├── Configs/
│   └── Faction/
├── Scripts/WorkbenchGame/WorldEditor/
└── Worlds/
    ├── ME_VBT_VehicleBoundsFixture.ent
    └── ME_VBT_VehicleBoundsFixture_Layers/
        ├── default.layer
        ├── managers.layer
        ├── Scopes.layer
        ├── CIV.layer
        ├── FIA.layer
        ├── US.layer
        └── USSR.layer
```

### Для чего нужны основные компоненты

- `ME_VBT_VehicleBoundsFixtureMarkerComponent` связывает размещённый fixture root с каноническим путём prefab из каталога.
- `ME_VBT_VehicleBoundsFactionScopeComponent` объявляет обязательный ключ фракции, для которого должен быть разрешён каталог `VEHICLE`.
- `ME_VBT_EditorFactionCatalogInitialization` делает каталоги фракций доступными editor-only инструментам, когда это необходимо.
- `ME_VBT_VehicleBoundsFixtureInventory` задаёт контракт версии fixture и проверяет все marker и scope roots.
- `ME_VBT_VehicleBoundsCatalogResolver` создаёт детерминированное объединение включённых записей faction-specific каталогов и их memberships.
- `ME_VBT_VehicleBoundsPerPrefabSnapshot` задаёт сериализуемую схему snapshot.
- Плагины World Editor предоставляют операции проверки, именования, генерации, повторной загрузки и сравнения.

### Обновление fixture после изменения игры

Если проверка покрытия обнаружила изменения каталогов:

1. найдите в диагностике все добавленные и удалённые канонические prefab;
2. добавьте или удалите соответствующий fixture root в нужном faction layer;
3. добавьте marker component с точным каноническим путём prefab из каталога;
4. оставьте fixture root неповёрнутым;
5. изменяйте ожидаемое количество roots и идентификатор fixture только при намеренном создании новой версии fixture;
6. выполните preflight канонических имён;
7. запускайте проверку покрытия до получения `PASS`;
8. создайте и изучите новый Candidate;
9. принимайте новый Baseline только после полного понимания различий.

Не добавляйте global catalog fallback, чтобы скрыть отсутствие фракции, manager, scope или каталога. Отсутствие обязательных входных данных намеренно считается ошибкой.

### Диагностика

Ищите в `script.log` Workbench сообщения с префиксом:

```text
[ME_VBT_WB]
```

Основные семейства результатов:

```text
fixture_names
fixture_coverage
snapshot_candidate
snapshot_compare
```

Скрипты toolkit компилируются в модулях `Game` и `WorkbenchGame`, а сами команды зарегистрированы для модуля `WorldEditor`.
