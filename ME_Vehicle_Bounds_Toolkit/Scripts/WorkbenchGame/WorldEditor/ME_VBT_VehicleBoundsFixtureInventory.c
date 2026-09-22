//! Provides the shared, deterministic inventory and version contract for the vehicle-bounds fixture.

//------------------------------------------------------------------------------------------------
//! One marked fixture vehicle root and its canonical catalog prefab.
class ME_VBT_VehicleBoundsFixtureMarkerRecord
{
	IEntitySource m_Source;
	IEntity m_Entity;
	ResourceName m_sPrefab;
}

//------------------------------------------------------------------------------------------------
//! One declarative faction scope found in the open fixture.
class ME_VBT_VehicleBoundsFactionScopeRecord
{
	IEntitySource m_Source;
	IEntity m_Entity;
	FactionKey m_sFactionKey;
}

//------------------------------------------------------------------------------------------------
//! Complete validated marker and faction-scope inventory.
class ME_VBT_VehicleBoundsFixtureInventoryResult
{
	ref array<ref ME_VBT_VehicleBoundsFixtureMarkerRecord> m_aMarkers = {};
	ref array<ref ME_VBT_VehicleBoundsFactionScopeRecord> m_aScopes = {};
}

//------------------------------------------------------------------------------------------------
//! Collects and validates the versioned public fixture contract.
class ME_VBT_VehicleBoundsFixtureInventory
{
	static const int SCHEMA_VERSION = 2;
	static const int EXPECTED_MARKER_COUNT = 164;
	static const string FIXTURE_IDENTITY = "ME_VBT_VehicleBoundsFixture_v1";
	static const string GENERATOR_VERSION = "ME_VBT_per_prefab_generator_v2-faction-type-groups";

	//------------------------------------------------------------------------------------------------
	//! Returns the sorted faction keys required by this fixture version.
	//!
	//! \param[out] factionKeys Required faction keys in deterministic order
	static void GetRequiredFactionKeys(out array<string> factionKeys)
	{
		factionKeys = { "CIV", "FIA", "US", "USSR" };
	}

	//------------------------------------------------------------------------------------------------
	//! Scans and validates all marker and scope roots in the open World Editor.
	//!
	//! \param[in] api Active World Editor API
	//! \param[out] inventory Validated deterministic inventory
	//! \param[out] reason Stable failure reason
	//! \return True when the complete fixture contract is present
	static bool Collect(WorldEditorAPI api, out ME_VBT_VehicleBoundsFixtureInventoryResult inventory, out string reason)
	{
		inventory = new ME_VBT_VehicleBoundsFixtureInventoryResult();
		reason = "";
		if (!api)
		{
			reason = "world_editor_api_unavailable";
			return false;
		}

		for (int index = 0; index < api.GetEditorEntityCount(); index++)
		{
			IEntitySource source = api.GetEditorEntity(index);
			IEntity entity = api.SourceToEntity(source);
			if (!entity)
				continue;

			ME_VBT_VehicleBoundsFixtureMarkerComponent marker = ME_VBT_VehicleBoundsFixtureMarkerComponent.Cast(entity.FindComponent(ME_VBT_VehicleBoundsFixtureMarkerComponent));
			if (marker)
			{
				ResourceName prefab = marker.GetCatalogPrefab();
				if (prefab.IsEmpty())
				{
					reason = string.Format("empty_marker_prefab entity=%1", entity.GetName());
					return false;
				}

				if (FindMarker(inventory.m_aMarkers, prefab))
				{
					reason = string.Format("duplicate_marker_prefab path=%1", prefab);
					return false;
				}

				vector angles = entity.GetAngles();
				if (!IsZeroVector(angles))
				{
					reason = string.Format("rotated_marker_root path=%1 rotation=%2", prefab, angles);
					return false;
				}

				ME_VBT_VehicleBoundsFixtureMarkerRecord markerRecord = new ME_VBT_VehicleBoundsFixtureMarkerRecord();
				markerRecord.m_Source = source;
				markerRecord.m_Entity = entity;
				markerRecord.m_sPrefab = prefab;
				inventory.m_aMarkers.Insert(markerRecord);
			}

			ME_VBT_VehicleBoundsFactionScopeComponent scope = ME_VBT_VehicleBoundsFactionScopeComponent.Cast(entity.FindComponent(ME_VBT_VehicleBoundsFactionScopeComponent));
			if (!scope)
				continue;

			FactionKey factionKey = scope.GetFactionKey();
			if (factionKey.IsEmpty())
			{
				reason = string.Format("empty_scope_faction_key entity=%1", entity.GetName());
				return false;
			}

			if (FindScope(inventory.m_aScopes, factionKey))
			{
				reason = string.Format("duplicate_scope_faction_key key=%1", factionKey);
				return false;
			}

			ME_VBT_VehicleBoundsFactionScopeRecord scopeRecord = new ME_VBT_VehicleBoundsFactionScopeRecord();
			scopeRecord.m_Source = source;
			scopeRecord.m_Entity = entity;
			scopeRecord.m_sFactionKey = factionKey;
			inventory.m_aScopes.Insert(scopeRecord);
		}

		SortMarkers(inventory.m_aMarkers);
		SortScopes(inventory.m_aScopes);
		if (inventory.m_aMarkers.Count() != EXPECTED_MARKER_COUNT)
		{
			reason = string.Format("marker_count_mismatch markers=%1 expected=%2", inventory.m_aMarkers.Count(), EXPECTED_MARKER_COUNT);
			return false;
		}

		array<string> requiredFactionKeys;
		GetRequiredFactionKeys(requiredFactionKeys);
		if (inventory.m_aScopes.Count() != requiredFactionKeys.Count())
		{
			reason = string.Format("scope_count_mismatch scopes=%1 expected=%2", inventory.m_aScopes.Count(), requiredFactionKeys.Count());
			return false;
		}

		for (int scopeIndex = 0; scopeIndex < requiredFactionKeys.Count(); scopeIndex++)
		{
			if (inventory.m_aScopes[scopeIndex].m_sFactionKey != requiredFactionKeys[scopeIndex])
			{
				reason = string.Format("scope_key_mismatch actual=%1 expected=%2", inventory.m_aScopes[scopeIndex].m_sFactionKey, requiredFactionKeys[scopeIndex]);
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds a marker record by canonical prefab path.
	//!
	//! \param[in] markers Marker records to search
	//! \param[in] prefab Canonical prefab path
	//! \return Matching marker record or null
	static ME_VBT_VehicleBoundsFixtureMarkerRecord FindMarker(array<ref ME_VBT_VehicleBoundsFixtureMarkerRecord> markers, ResourceName prefab)
	{
		foreach (ME_VBT_VehicleBoundsFixtureMarkerRecord marker : markers)
			if (marker.m_sPrefab == prefab)
				return marker;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds a scope record by faction key.
	//!
	//! \param[in] scopes Scope records to search
	//! \param[in] factionKey Required faction key
	//! \return Matching scope record or null
	static ME_VBT_VehicleBoundsFactionScopeRecord FindScope(array<ref ME_VBT_VehicleBoundsFactionScopeRecord> scopes, FactionKey factionKey)
	{
		foreach (ME_VBT_VehicleBoundsFactionScopeRecord scope : scopes)
			if (scope.m_sFactionKey == factionKey)
				return scope;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts marker records by canonical prefab path.
	static void SortMarkers(array<ref ME_VBT_VehicleBoundsFixtureMarkerRecord> markers)
	{
		for (int i = 1; i < markers.Count(); i++)
		{
			ME_VBT_VehicleBoundsFixtureMarkerRecord value = markers[i];
			int previous = i - 1;
			while (previous >= 0 && markers[previous].m_sPrefab > value.m_sPrefab)
			{
				markers[previous + 1] = markers[previous];
				previous--;
			}
			markers[previous + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts scope records by faction key.
	static void SortScopes(array<ref ME_VBT_VehicleBoundsFactionScopeRecord> scopes)
	{
		for (int i = 1; i < scopes.Count(); i++)
		{
			ME_VBT_VehicleBoundsFactionScopeRecord value = scopes[i];
			int previous = i - 1;
			while (previous >= 0 && scopes[previous].m_sFactionKey > value.m_sFactionKey)
			{
				scopes[previous + 1] = scopes[previous];
				previous--;
			}
			scopes[previous + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Derives a safe editor and container name from the terminal prefab filename.
	//!
	//! \param[in] prefab Canonical .et prefab path
	//! \param[out] name Validated terminal filename stem
	//! \return True when the path has a safe non-empty terminal stem
	static bool GetPrefabStem(ResourceName prefab, out string name)
	{
		name = "";
		if (prefab.IsEmpty())
			return false;

		int slashIndex = prefab.LastIndexOf("/");
		int startIndex = slashIndex + 1;
		int filenameLength = prefab.Length() - startIndex;
		if (filenameLength <= 3)
			return false;

		string filename = prefab.Substring(startIndex, filenameLength);
		if (!filename.EndsWith(".et"))
			return false;

		name = filename.Substring(0, filename.Length() - 3);
		return !name.IsEmpty() && !name.Contains(" ") && !name.Contains("/") && !name.Contains("\\") && !name.Contains("\"") && !name.Contains("{") && !name.Contains("}");
	}

	//------------------------------------------------------------------------------------------------
	//! Checks whether all vector axes are effectively zero.
	static bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.0001 && Math.AbsFloat(value[1]) < 0.0001 && Math.AbsFloat(value[2]) < 0.0001;
	}
}
