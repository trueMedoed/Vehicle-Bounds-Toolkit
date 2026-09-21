# Arma Reforger Workbench: запуск и fixture-миры

## Проверенные нюансы

### Пути с пробелами

При запуске Workbench через CLI путь к `.gproj` должен передаваться одним аргументом. Если путь содержит пробелы, его нужно заключать в кавычки:

```text
-gproj "C:\Users\Phil\Documents\GitHub\ArmaReforger\Mods\Vehicle Bounds Toolkit\ME_Vehicle_Bounds_Toolkit\addon.gproj"
```

Без кавычек Workbench может получить только часть пути, например до `...\Mods\Vehicle`, и завершить запуск с ошибкой загрузки `.gproj`.

### Точное имя fixture-ресурса

Перед открытием мира нужно проверять точное имя файла. В этом проекте правильный путь:

```text
Worlds/ME_VBT_VehicleBoundsFixture.ent
```

Автоматизация использовала ошибочный путь без префикса `VBT`:

```text
Worlds/ME_VehicleBoundsFixture.ent
```

Из-за этого Workbench сообщил `can't load world`, что ошибочно выглядело как проблема совместимости.

### Родительский мир `MpTest_Basic.ent`

Fixture-файл содержит ссылку на:

```text
{A701424D70022078}worlds/MP/MpTest/MpTest_Basic.ent
```

`MpTest_Basic.ent` совместим с Experimental Workbench. Ошибка при автоматическом тесте была вызвана неправильным quoting CLI и опечаткой в пути к fixture, а не несовместимостью родительского мира.

## Практика для следующих проектов

1. Проверять фактический путь к `.gproj` и передавать его как один quoted CLI-аргумент.
2. Перед открытием мира сверять имя ресурса через список файлов проекта или `project_browse`.
3. При ошибке загрузки мира сначала проверять путь и состояние Workbench, а не делать вывод о несовместимости его родительских ресурсов.
4. При использовании Experimental отдельно подтверждать, что запущен Experimental Workbench, а не Stable.
