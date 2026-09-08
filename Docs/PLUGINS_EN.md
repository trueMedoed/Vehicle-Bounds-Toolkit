# Workbench plugins

[Back to the English README](../README_EN.md)

All commands are available from:

```text
Plugins > ME Vehicle Bounds Toolkit
```

Open `Worlds/ME_VBT_VehicleBoundsFixture.ent` before running them. Play mode is not required.

## Recommended order

1. `VBT: Preflight fixture names`
2. `VBT: Validate fixture coverage`
3. `VBT: Generate bounds snapshot`

Run `VBT: Apply fixture names` only when the preflight reports that roots need renaming.

## `VBT: Preflight fixture names`

Performs a read-only validation of reference-scene root names.

The expected editor name for every marked vehicle root is derived from the final filename of its canonical prefab path. The command detects:

- invalid or empty prefab paths;
- duplicate target names;
- name conflicts with other editor entities;
- missing, duplicate, or otherwise invalid marked roots.

It prints every current-to-target mapping without modifying the scene.

Expected result for an already normalized scene:

```text
[ME_VBT_WB] fixture_names status=PASS phase=preflight markers=146 unchanged=146
```

## `VBT: Apply fixture names`

Runs the complete name preflight and applies all required canonical names as one undoable World Editor action.

The command makes no changes when validation fails or when every root already has its canonical name. Review the result and save the world manually.

## `VBT: Validate fixture coverage`

Compares the marked vehicle roots with the deterministic union of enabled entries from the declared faction-specific `VEHICLE` catalogs.

The command validates:

- all required faction scopes;
- availability of the faction manager, factions, and catalogs;
- non-empty enabled catalog entries;
- unique canonical prefab paths;
- the expected number of marked roots;
- unrotated measured roots;
- exact one-to-one coverage between catalog paths and roots.

There is no global or factionless catalog fallback. Missing required catalog data produces `FAIL`.

Expected result:

```text
[ME_VBT_WB] fixture_coverage status=PASS markers=146 scopes=4 catalog_union=146
```

## `VBT: Generate bounds snapshot`

Runs the complete measurement and comparison pipeline:

1. collects and validates the marked roots and faction scopes;
2. resolves all enabled entries from the faction-specific vehicle catalogs;
3. verifies exact catalog coverage;
4. generates a world-space axis-aligned bounding box (AABB) for each root and all of its children with `SCR_Global.GetWorldBoundsWithChildren`;
5. converts the AABB minimum and maximum corners to coordinates relative to the unrotated root;
6. records sorted faction and vehicle-type memberships;
7. validates and saves the Candidate;
8. rebuilds and reloads the Candidate resource;
9. verifies that the reloaded model matches the generated model;
10. compares Candidate with the accepted Baseline.

Successful generation produces:

```text
[ME_VBT_WB] snapshot_candidate status=PASS ...
```

The comparison then produces one of:

```text
[ME_VBT_WB] snapshot_compare status=PASS ...
[ME_VBT_WB] snapshot_compare status=DIFF ...
[ME_VBT_WB] snapshot_compare status=FAIL reason=...
```

The generator writes only the Candidate. Baseline acceptance is always a separate manual operation.

## Troubleshooting

### No toolkit messages

Confirm that the active project is `ME_Vehicle_Bounds_Toolkit`, the reference scene is open, and the command was selected from the correct plugin category.

### `world_editor_unavailable`

The command was not run from an active World Editor session.

### `faction_manager_unavailable`

The open scene does not contain the required faction manager, or the reference-scene manager layer is not loaded.

### Coverage failure

Use the reported `reason=...` and prefab paths to find missing, duplicate, unexpected, or rotated roots. Update the reference scene before generating a new snapshot.

### Automation timeout

Cold measurement of all vehicle roots can outlive an automation bridge timeout. Do not immediately run the command a second time. Wait and inspect `script.log` for the final `snapshot_candidate` and `snapshot_compare` messages.
