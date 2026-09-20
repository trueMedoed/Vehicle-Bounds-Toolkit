//! Defines the standalone schema for deterministic per-prefab vehicle-bounds snapshots.

//------------------------------------------------------------------------------------------------
//! One canonical prefab with measured local bounds.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VBT_VehicleBoundsPerPrefabSnapshotEntry
{
	//! Canonical vehicle catalog prefab resource path.
	[Attribute("")]
	ResourceName m_sPrefab;

	//! Axis-aligned local minimum corner relative to the unrotated fixture root.
	[Attribute("0 0 0")]
	vector m_vLocalMins;

	//! Axis-aligned local maximum corner relative to the unrotated fixture root.
	[Attribute("0 0 0")]
	vector m_vLocalMaxs;
}

//------------------------------------------------------------------------------------------------
//! One basic vehicle classification containing canonical prefab measurements.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType
{
	//! Supported basic VEHICLE_* classification.
	[Attribute("")]
	string m_sVehicleType;

	//! Prefab entries sorted by canonical resource path.
	[Attribute()]
	ref array<ref ME_VBT_VehicleBoundsPerPrefabSnapshotEntry> m_aEntries;
}

//------------------------------------------------------------------------------------------------
//! One faction containing its basic vehicle-type groups.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VBT_VehicleBoundsPerPrefabSnapshotFaction
{
	//! Stable faction key.
	[Attribute("")]
	string m_sFactionKey;

	//! Vehicle-type groups sorted by classification name.
	[Attribute()]
	ref array<ref ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType> m_aVehicleTypes;
}

//------------------------------------------------------------------------------------------------
//! Root schema for a complete deterministic vehicle-bounds fixture snapshot.
[BaseContainerProps(configRoot: true)]
class ME_VBT_VehicleBoundsPerPrefabSnapshot
{
	//! Schema compatibility version.
	[Attribute("2")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Stable identity of the fixture used for all measurements.
	[Attribute("")]
	string m_sFixtureIdentity;

	//! Game build version reported while this snapshot was generated.
	[Attribute("")]
	string m_sGameVersion;

	//! Faction groups sorted by faction key.
	[Attribute()]
	ref array<ref ME_VBT_VehicleBoundsPerPrefabSnapshotFaction> m_aFactions;
}
