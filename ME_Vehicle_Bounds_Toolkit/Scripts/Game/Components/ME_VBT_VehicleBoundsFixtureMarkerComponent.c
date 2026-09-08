//! Binds one placed fixture vehicle root to its canonical faction-catalog prefab path.

//------------------------------------------------------------------------------------------------
//! Declares the catalog prefab represented by the owning fixture vehicle root.
class ME_VBT_VehicleBoundsFixtureMarkerComponentClass : ScriptComponentClass
{
}

class ME_VBT_VehicleBoundsFixtureMarkerComponent : ScriptComponent
{
	//! Canonical ResourceName returned by SCR_EntityCatalogEntry.GetPrefab().
	[Attribute("")]
	protected ResourceName m_sCatalogPrefab;

	//------------------------------------------------------------------------------------------------
	//! Returns the declared canonical catalog prefab path.
	//!
	//! \return Canonical prefab resource name
	ResourceName GetCatalogPrefab()
	{
		return m_sCatalogPrefab;
	}
}
