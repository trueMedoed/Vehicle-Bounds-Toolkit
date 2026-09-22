[WorkbenchPluginAttribute(name: "VBT: Temporary measure bounds", description: "Temporary measurement helper.", wbModules: { "WorldEditor" }, category: "ME Vehicle Bounds Toolkit")]
class ME_VBT_TemporaryMeasureBoundsPlugin : WorldEditorPlugin
{
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI api = worldEditor.GetApi();
		for (int index = 0; index < api.GetEditorEntityCount(); index++)
		{
			IEntity entity = api.SourceToEntity(api.GetEditorEntity(index));
			if (!entity || entity.GetName() != "V2101_yellow")
				continue;

			vector mins;
			vector maxs;
			SCR_Global.GetWorldBoundsWithChildren(entity, mins, maxs);
			PrintFormat("[ME_VBT_TEMP] name=%1 origin=%2 mins=%3 maxs=%4", entity.GetName(), entity.GetOrigin(), mins, maxs);
		}
	}
}
