# ME Vehicle Bounds Toolkit

Standalone Arma Reforger Workbench tooling for measuring per-prefab vehicle bounds and detecting changes between game builds.

## Purpose

The toolkit answers two questions:

1. Does the reference scene cover every enabled vehicle prefab from the supported faction catalogs?
2. Did the bounds of any covered vehicle change after a game update or scene change?

The project uses the term `fixture` in code, resource names, and diagnostics. In this documentation, **reference scene** means the same thing: a controlled World Editor scene containing one marked root for every vehicle being measured.

The toolkit reads the faction-specific `VEHICLE` catalogs for `CIV`, `FIA`, `US`, and `USSR`. It validates the placed vehicle roots, measures every root together with its children, and stores deterministic local bounds with faction and vehicle-type information.

This data can be used for editor previews, placement validation, spacing calculations, and regression checks.

## Quick start

1. Open `ME_Vehicle_Bounds_Toolkit/addon.gproj` in Workbench.
2. Open `Worlds/ME_VBT_VehicleBoundsFixture.ent` in the World Editor.
3. Wait until the scene and vehicle resources finish loading.
4. Run these commands from `Plugins > ME Vehicle Bounds Toolkit`:
   1. `VBT: Preflight fixture names`
   2. `VBT: Validate fixture coverage`
   3. `VBT: Generate bounds snapshot`
5. Check the Workbench console or `script.log` for `[ME_VBT_WB]` messages.
6. Review all Candidate/Baseline differences before accepting a new Baseline.

Play mode is not required. These commands are World Editor tools.

For detailed command descriptions, see [Workbench plugins](Docs/PLUGINS_EN.md).

## Snapshots

Each snapshot stores:

- schema, generator, reference-scene, and game-build versions;
- the canonical prefab resource path;
- local minimum and maximum bounds;
- sorted faction keys;
- sorted vehicle-type labels.

Entries are sorted by canonical prefab path. Their serialized container names are derived from prefab filenames so that text diffs remain readable.

### Candidate

`Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf` is generated from the current game build and reference scene. Running the generator can replace its contents.

Do not treat manual Candidate edits as authoritative.

### Baseline

`Configs/Generated/ME_VBT_VehicleBoundsPerPrefabBaseline.conf` is the manually accepted reference. The generator reads it but never overwrites it.

To accept an intentional change:

1. generate a Candidate;
2. confirm that Candidate save and reload validation passed;
3. review the semantic comparison and text diff;
4. verify that every added, removed, or changed prefab is expected;
5. manually replace the Baseline payload with the reviewed Candidate payload;
6. preserve the Baseline filename and its existing `.meta` file;
7. reload resources or restart Workbench if the previous Baseline remains cached;
8. run the generator again and confirm `snapshot_compare status=PASS`;
9. run it once more to confirm deterministic output.

Never copy the Candidate `.meta` file over the Baseline `.meta` file. They are independent registered resources with different GUIDs.

## Result meanings

- `PASS` — the operation completed successfully. For snapshot comparison, Candidate and Baseline are semantically identical.
- `DIFF` — Candidate is valid but differs from Baseline. Review all reported changes before accepting them.
- `FAIL` — the toolkit could not produce a trustworthy result. Use the reported `reason=...` value to identify the scene, catalog, resource, or validation problem.

A cold bounds measurement can take longer than a Workbench automation bridge timeout. A timeout alone does not prove failure; check the final `[ME_VBT_WB]` messages in `script.log`.

## Updating the reference scene

If catalog coverage changes after a game update:

1. identify added or removed prefab paths in the diagnostics;
2. add or remove the corresponding vehicle root in the appropriate faction layer;
3. assign the exact canonical catalog path through `ME_VBT_VehicleBoundsFixtureMarkerComponent`;
4. keep every measured root unrotated;
5. update the expected root count and reference-scene identity only when intentionally creating a new version;
6. run the name preflight and coverage validation;
7. generate and review a new Candidate;
8. accept a new Baseline only after every difference is understood.

Required faction, manager, scope, and catalog data is validated strictly. Missing data must be fixed rather than hidden with a global catalog fallback.

## Prefixes

- Script and class prefix: `ME_VBT_`
- Diagnostic prefix: `[ME_VBT_WB]`

Search `script.log` for the diagnostic prefix. The main result families are:

```text
fixture_names
fixture_coverage
snapshot_candidate
snapshot_compare
```
