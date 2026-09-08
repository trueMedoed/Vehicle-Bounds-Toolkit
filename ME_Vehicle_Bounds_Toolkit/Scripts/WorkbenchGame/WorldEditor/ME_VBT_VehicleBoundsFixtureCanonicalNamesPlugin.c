//! Provides canonical fixture-root name preflight and the shared guarded rename operation.

//------------------------------------------------------------------------------------------------
//! One validated marker root and its canonical editor name.
class ME_VBT_VehicleBoundsFixtureRenameEntry
{
	IEntitySource m_Source;
	IEntity m_Entity;
	ResourceName m_sPrefab;
	string m_sTargetName;
}

//------------------------------------------------------------------------------------------------
//! Implements deterministic canonical-name validation and renaming.
class ME_VBT_VehicleBoundsFixtureCanonicalNames
{
	//------------------------------------------------------------------------------------------------
	//! Collects and reports every canonical rename mapping without changing the world.
	//!
	//! \param[in] api Active World Editor API
	//! \param[out] entries Validated rename entries
	//! \return True when all target names are valid and conflict-free
	static bool Preflight(WorldEditorAPI api, out array<ref ME_VBT_VehicleBoundsFixtureRenameEntry> entries)
	{
		entries = {};
		ME_VBT_VehicleBoundsFixtureInventoryResult inventory;
		string reason;
		if (!ME_VBT_VehicleBoundsFixtureInventory.Collect(api, inventory, reason))
		{
			PrintFormat("[ME_VBT_WB] fixture_names status=FAIL phase=preflight reason=%1", reason);
			return false;
		}

		array<IEntitySource> allSources = {};
		for (int sourceIndex = 0; sourceIndex < api.GetEditorEntityCount(); sourceIndex++)
			allSources.Insert(api.GetEditorEntity(sourceIndex));

		foreach (ME_VBT_VehicleBoundsFixtureMarkerRecord marker : inventory.m_aMarkers)
		{
			string targetName;
			if (!ME_VBT_VehicleBoundsFixtureInventory.GetPrefabStem(marker.m_sPrefab, targetName))
			{
				PrintFormat("[ME_VBT_WB] fixture_names status=FAIL phase=preflight reason=invalid_catalog_prefab prefab=%1", marker.m_sPrefab);
				return false;
			}

			foreach (ME_VBT_VehicleBoundsFixtureRenameEntry existing : entries)
			{
				if (existing.m_sTargetName == targetName)
				{
					PrintFormat("[ME_VBT_WB] fixture_names status=FAIL phase=preflight reason=duplicate_target_name target=%1", targetName);
					return false;
				}
			}

			ME_VBT_VehicleBoundsFixtureRenameEntry entry = new ME_VBT_VehicleBoundsFixtureRenameEntry();
			entry.m_Source = marker.m_Source;
			entry.m_Entity = marker.m_Entity;
			entry.m_sPrefab = marker.m_sPrefab;
			entry.m_sTargetName = targetName;
			entries.Insert(entry);
		}

		foreach (ME_VBT_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			foreach (IEntitySource source : allSources)
			{
				if (source == entry.m_Source)
					continue;

				IEntity entity = api.SourceToEntity(source);
				if (entity && entity.GetName() == entry.m_sTargetName)
				{
					PrintFormat("[ME_VBT_WB] fixture_names status=FAIL phase=preflight reason=editor_name_conflict target=%1", entry.m_sTargetName);
					return false;
				}
			}
		}

		int unchanged;
		foreach (ME_VBT_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			if (entry.m_Entity.GetName() == entry.m_sTargetName)
				unchanged++;
			PrintFormat("[ME_VBT_WB] fixture_name_mapping prefab=%1 current=%2 target=%3 position=%4 rotation=%5", entry.m_sPrefab, entry.m_Entity.GetName(), entry.m_sTargetName, entry.m_Entity.GetOrigin(), entry.m_Entity.GetAngles());
		}

		PrintFormat("[ME_VBT_WB] fixture_names status=PASS phase=preflight markers=%1 unchanged=%2", entries.Count(), unchanged);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies a single undoable rename batch after a complete successful preflight.
	static void Apply(WorldEditorAPI api)
	{
		array<ref ME_VBT_VehicleBoundsFixtureRenameEntry> entries;
		if (!Preflight(api, entries))
			return;

		int renames;
		int unchanged;
		foreach (ME_VBT_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			if (entry.m_Entity.GetName() == entry.m_sTargetName)
				unchanged++;
			else
				renames++;
		}

		if (renames == 0)
		{
			PrintFormat("[ME_VBT_WB] fixture_names status=PASS phase=apply markers=%1 renames=0 unchanged=%2", entries.Count(), unchanged);
			return;
		}

		api.BeginEntityAction("Canonical vehicle-bounds fixture names");
		foreach (ME_VBT_VehicleBoundsFixtureRenameEntry entry : entries)
			if (entry.m_Entity.GetName() != entry.m_sTargetName)
				api.RenameEntity(entry.m_Source, entry.m_sTargetName);
		api.EndEntityAction();

		PrintFormat("[ME_VBT_WB] fixture_names status=PASS phase=apply markers=%1 renames=%2 unchanged=%3", entries.Count(), renames, unchanged);
	}
}

//------------------------------------------------------------------------------------------------
//! Reports canonical fixture-root names without mutating the world.
[WorkbenchPluginAttribute(name: "VBT: Preflight fixture names", description: "Reports and validates canonical fixture-root names without changing the world.", wbModules: { "WorldEditor" }, category: "ME Vehicle Bounds Toolkit")]
class ME_VBT_VehicleBoundsFixtureCanonicalNamesPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Runs the read-only canonical-name preflight.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_VBT_WB] fixture_names status=FAIL phase=preflight reason=world_editor_unavailable");
			return;
		}

		array<ref ME_VBT_VehicleBoundsFixtureRenameEntry> entries;
		ME_VBT_VehicleBoundsFixtureCanonicalNames.Preflight(worldEditor.GetApi(), entries);
	}
}
