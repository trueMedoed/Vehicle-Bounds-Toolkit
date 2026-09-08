//! Validates exact one-to-one coverage between the versioned fixture and declared faction catalogs.

//------------------------------------------------------------------------------------------------
//! Checks the sorted catalog union against the sorted marker roots without mutating the world.
[WorkbenchPluginAttribute(name: "VBT: Validate fixture coverage", description: "Checks exact faction-catalog-to-marker coverage for the vehicle-bounds fixture.", wbModules: { "WorldEditor" }, category: "ME Vehicle Bounds Toolkit")]
class ME_VBT_VehicleBoundsFixtureCoveragePlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Runs the read-only fixture coverage validation.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_VBT_WB] fixture_coverage status=FAIL reason=world_editor_unavailable");
			return;
		}

		ME_VBT_VehicleBoundsFixtureInventoryResult inventory;
		string reason;
		if (!ME_VBT_VehicleBoundsFixtureInventory.Collect(worldEditor.GetApi(), inventory, reason))
		{
			PrintFormat("[ME_VBT_WB] fixture_coverage status=FAIL reason=%1", reason);
			return;
		}

		array<ref ME_VBT_VehicleBoundsCatalogRecord> catalogRecords;
		if (!ME_VBT_VehicleBoundsCatalogResolver.Resolve(inventory, catalogRecords, reason))
		{
			PrintFormat("[ME_VBT_WB] fixture_coverage status=FAIL reason=%1", reason);
			return;
		}

		if (!ME_VBT_VehicleBoundsCatalogResolver.ValidateCoverage(inventory, catalogRecords, reason))
		{
			PrintFormat("[ME_VBT_WB] fixture_coverage status=FAIL reason=%1", reason);
			return;
		}

		PrintFormat("[ME_VBT_WB] fixture_coverage status=PASS markers=%1 scopes=%2 catalog_union=%3", inventory.m_aMarkers.Count(), inventory.m_aScopes.Count(), catalogRecords.Count());
	}
}
