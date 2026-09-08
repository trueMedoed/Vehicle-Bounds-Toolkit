//! Declares one required faction whose enabled VEHICLE catalog belongs to the fixture contract.

//------------------------------------------------------------------------------------------------
//! Component class for declarative vehicle-bounds faction scopes.
class ME_VBT_VehicleBoundsFactionScopeComponentClass : ScriptComponentClass
{
}

class ME_VBT_VehicleBoundsFactionScopeComponent : ScriptComponent
{
	//! Exact faction key resolved through the active FactionManager.
	[Attribute("")]
	protected FactionKey m_sFactionKey;

	//------------------------------------------------------------------------------------------------
	//! Returns the required faction key.
	//!
	//! \return Required faction key
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}
}
