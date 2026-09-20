# Плагины Workbench

[Вернуться к русскому README](../README_RU.md)

Все команды доступны через:

```text
Plugins > ME Vehicle Bounds Toolkit
```

Перед запуском откройте `Worlds/ME_VBT_VehicleBoundsFixture.ent`. Переходить в Play mode не требуется.

## Рекомендуемый порядок

1. `VBT: Preflight fixture names`
2. `VBT: Validate fixture coverage`
3. `VBT: Generate bounds snapshot`

Запускайте `VBT: Apply fixture names` только тогда, когда предварительная проверка сообщает о необходимости переименования roots.

## `VBT: Preflight fixture names`

Выполняет проверку имён roots эталонной сцены без изменения мира.

Ожидаемое имя каждого помеченного root техники создаётся из последнего имени файла в его каноническом пути prefab. Команда обнаруживает:

- некорректные или пустые пути prefab;
- повторяющиеся целевые имена;
- конфликты имён с другими сущностями редактора;
- отсутствующие, повторяющиеся или иначе некорректные помеченные roots.

Команда выводит все сопоставления текущих и целевых имён, не изменяя сцену.

Ожидаемый результат для уже нормализованной сцены:

```text
[ME_VBT_WB] fixture_names status=PASS phase=preflight markers=146 unchanged=146
```

## `VBT: Apply fixture names`

Выполняет полную предварительную проверку имён и применяет все необходимые канонические имена как одно отменяемое действие World Editor.

Команда ничего не изменяет, если проверка завершилась ошибкой или все roots уже имеют канонические имена. Проверьте результат и сохраните мир вручную.

## `VBT: Validate fixture coverage`

Сравнивает помеченные roots техники с детерминированным объединением включённых записей из объявленных faction-specific каталогов `VEHICLE`. Повторное представление одного канонического prefab допустимо только для той же пары faction/basic type; другая принадлежность приводит к `FAIL`.

Команда проверяет:

- наличие всех обязательных faction scopes;
- доступность faction manager, фракций и каталогов;
- непустые включённые записи каталогов;
- ровно одну поддерживаемую basic `VEHICLE_*` classification для каждого canonical prefab;
- отсутствие конфликтов faction/type при повторном canonical prefab;
- уникальность результирующих канонических путей prefab;
- ожидаемое количество помеченных roots;
- отсутствие поворота у измеряемых roots;
- точное взаимно-однозначное соответствие между путями каталога и roots.

Global или factionless fallback для каталога не используется. Отсутствие обязательных данных каталога приводит к `FAIL`.

Ожидаемый результат:

```text
[ME_VBT_WB] fixture_coverage status=PASS markers=146 scopes=4 catalog_union=146
```

## `VBT: Generate bounds snapshot`

Выполняет полный процесс измерения и сравнения:

1. собирает и проверяет помеченные roots и faction scopes;
2. разрешает все включённые записи faction-specific каталогов техники и требует для каждого prefab одну faction и одну basic classification;
3. проверяет точное покрытие каталогов;
4. создаёт world-space осево-ориентированный bounding box (AABB) для каждого root и всех его дочерних сущностей через `SCR_Global.GetWorldBoundsWithChildren`;
5. переводит минимальную и максимальную точки AABB в координаты относительно неповёрнутого root;
6. строит grouped schema v2 `faction → vehicle type → prefab` и сортирует все три уровня;
7. назначает identifier-safe имена контейнеров `<FactionKey>`, `<VehicleType>` и `<PrefabStem>` с проверкой collisions в каждом scope;
8. проверяет metadata, допустимые basic types, bounds, глобальную уникальность prefab и точное количество `146`, затем сохраняет Candidate;
9. перестраивает и cache-safe загружает ресурс Candidate через `BaseContainerTools.LoadContainer`;
10. проверяет raw hierarchy, counts, порядок и имена всех трёх уровней, затем полное field-by-field соответствие загруженной typed-модели созданной модели;
11. теми же raw и semantic проверками валидирует Baseline и сравнивает его с Candidate по canonical prefab path.

Успешная генерация выводит:

```text
[ME_VBT_WB] snapshot_candidate status=PASS ...
```

После этого сравнение выводит один из результатов:

```text
[ME_VBT_WB] snapshot_compare status=PASS ...
[ME_VBT_WB] snapshot_compare status=DIFF ...
[ME_VBT_WB] snapshot_compare status=FAIL reason=...
```

Генератор записывает только Candidate. Семантический diff разворачивает grouped hierarchy в детерминированный список по canonical prefab path и сообщает `ADDED`, `REMOVED` и `CHANGED`; смена parent faction или vehicle type считается membership change. Принятие Baseline всегда выполняется вручную как отдельная операция: копируйте только проверенный payload Candidate, сохраняя filename и отдельный `.meta` Baseline. Никогда не заменяйте его `.meta` файлом Candidate.

## Решение проблем

### Нет сообщений Toolkit

Убедитесь, что активен проект `ME_Vehicle_Bounds_Toolkit`, открыта эталонная сцена и команда выбрана в правильной категории плагинов.

### `world_editor_unavailable`

Команда была запущена без активной сессии World Editor.

### `faction_manager_unavailable`

Открытая сцена не содержит обязательный faction manager либо manager layer эталонной сцены не загружен.

### Ошибка покрытия

Используйте значение `reason=...` и пути prefab, чтобы найти отсутствующие, повторяющиеся, неожиданные или повёрнутые roots. Исправьте эталонную сцену до создания нового snapshot.

### Тайм-аут автоматизации

Холодное измерение всех roots техники может выполняться дольше тайм-аута automation bridge. Не запускайте команду повторно сразу. Дождитесь завершения и проверьте `script.log` на итоговые сообщения `snapshot_candidate` и `snapshot_compare`.
