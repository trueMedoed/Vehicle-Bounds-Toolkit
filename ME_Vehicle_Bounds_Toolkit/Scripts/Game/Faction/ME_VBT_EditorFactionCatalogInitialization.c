//! Provides the explicit editor-only lazy initialization required to inspect faction catalogs.

//------------------------------------------------------------------------------------------------
//! Extends faction data with an editor-only catalog initialization step.
modded class SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	//! Initializes this faction's catalog map only while the World Editor is active.
	//!
	//! \return True when the faction catalog map is ready for read-only inspection
	bool ME_VBT_EnsureEditorCatalogsInitialized()
	{
		if (m_bCatalogInitDone)
			return true;

		if (!SCR_Global.IsEditMode() || !m_aEntityCatalogs)
			return false;

		SCR_EntityCatalogManagerComponent.InitCatalogs(m_aEntityCatalogs, m_mEntityCatalogs);
		m_bCatalogInitDone = true;
		m_aEntityCatalogs = null;
		return true;
	}
}
