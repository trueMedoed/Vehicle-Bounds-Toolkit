//! Applies validated canonical editor names to vehicle-bounds fixture roots.

//------------------------------------------------------------------------------------------------
//! Renames all validated marker roots in one undoable batch.
[WorkbenchPluginAttribute(name: "VBT: Apply fixture names", description: "Renames validated fixture roots in one undoable batch.", wbModules: { "WorldEditor" }, category: "ME Vehicle Bounds Toolkit")]
class ME_VBT_ApplyVehicleBoundsFixtureCanonicalNamesPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Runs the guarded canonical-name rename batch.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_VBT_WB] fixture_names status=FAIL phase=apply reason=world_editor_unavailable");
			return;
		}

		ME_VBT_VehicleBoundsFixtureCanonicalNames.Apply(worldEditor.GetApi());
	}
}
