//! Resolves the enabled faction-specific VEHICLE catalogs declared by the open fixture.

//------------------------------------------------------------------------------------------------
//! One canonical prefab and its single faction/type membership.
class ME_VBT_VehicleBoundsCatalogRecord
{
	ResourceName m_sPrefab;
	string m_sFactionKey;
	string m_sVehicleType;
}

//------------------------------------------------------------------------------------------------
//! Resolves a deterministic union without any global or factionless fallback.
class ME_VBT_VehicleBoundsCatalogResolver
{
	//------------------------------------------------------------------------------------------------
	//! Resolves every enabled VEHICLE entry from each declared faction scope.
	//!
	//! \param[in] inventory Validated fixture inventory
	//! \param[out] records Sorted union of canonical prefab records
	//! \param[out] reason Stable failure reason
	//! \return True when every required faction catalog was resolved
	static bool Resolve(ME_VBT_VehicleBoundsFixtureInventoryResult inventory, out array<ref ME_VBT_VehicleBoundsCatalogRecord> records, out string reason)
	{
		records = {};
		reason = "";
		if (!inventory || !inventory.m_aScopes || inventory.m_aScopes.IsEmpty())
		{
			reason = "fixture_scopes_unavailable";
			return false;
		}

		ChimeraGame game = ChimeraGame.Cast(GetGame());
		FactionManager factionManager;
		if (game)
			factionManager = game.GetFactionManager();
		if (!factionManager)
		{
			reason = "faction_manager_unavailable";
			return false;
		}

		foreach (ME_VBT_VehicleBoundsFactionScopeRecord scope : inventory.m_aScopes)
		{
			SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(scope.m_sFactionKey));
			if (!faction)
			{
				reason = string.Format("faction_unavailable key=%1", scope.m_sFactionKey);
				return false;
			}

			if (!faction.ME_VBT_EnsureEditorCatalogsInitialized())
			{
				reason = string.Format("faction_catalog_initialization_unavailable key=%1", scope.m_sFactionKey);
				return false;
			}

			SCR_EntityCatalog catalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (!catalog)
			{
				reason = string.Format("faction_vehicle_catalog_unavailable key=%1", scope.m_sFactionKey);
				return false;
			}

			array<SCR_EntityCatalogEntry> entries = {};
			catalog.GetEntityList(entries);
			if (entries.IsEmpty())
			{
				reason = string.Format("faction_vehicle_catalog_empty key=%1", scope.m_sFactionKey);
				return false;
			}

			foreach (SCR_EntityCatalogEntry catalogEntry : entries)
			{
				if (!catalogEntry || !catalogEntry.IsEnabled())
					continue;

				ResourceName prefab = catalogEntry.GetPrefab();
				if (prefab.IsEmpty())
				{
					reason = string.Format("empty_catalog_prefab key=%1", scope.m_sFactionKey);
					return false;
				}

				array<EEditableEntityLabel> labels = {};
				catalogEntry.GetEditableEntityLabels(labels);
				array<string> vehicleTypes = {};
				foreach (EEditableEntityLabel label : labels)
				{
					string labelName;
					if (TryGetVehicleTypeName(label, labelName) && !vehicleTypes.Contains(labelName))
						vehicleTypes.Insert(labelName);
				}
				if (vehicleTypes.Count() != 1)
				{
					string typeNames;
					foreach (string typeName : vehicleTypes)
					{
						if (!typeNames.IsEmpty())
							typeNames += ",";
						typeNames += typeName;
					}
					reason = string.Format("vehicle_type_membership_count_invalid path=%1 faction=%2 count=%3 types=%4", prefab, scope.m_sFactionKey, vehicleTypes.Count(), typeNames);
					return false;
				}
				string vehicleType = vehicleTypes[0];

				ME_VBT_VehicleBoundsCatalogRecord record = FindRecord(records, prefab);
				if (record)
				{
					if (record.m_sFactionKey != scope.m_sFactionKey || record.m_sVehicleType != vehicleType)
					{
						reason = string.Format("duplicate_prefab_membership_conflict path=%1 existing_faction=%2 existing_type=%3 duplicate_faction=%4 duplicate_type=%5", prefab, record.m_sFactionKey, record.m_sVehicleType, scope.m_sFactionKey, vehicleType);
						return false;
					}
					continue;
				}

				record = new ME_VBT_VehicleBoundsCatalogRecord();
				record.m_sPrefab = prefab;
				record.m_sFactionKey = scope.m_sFactionKey;
				record.m_sVehicleType = vehicleType;
				records.Insert(record);
			}
		}

		if (records.IsEmpty())
		{
			reason = "catalog_records_empty";
			return false;
		}

		SortRecords(records);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Requires exact sorted path coverage between catalog records and marker roots.
	//!
	//! \param[in] inventory Validated fixture inventory
	//! \param[in] records Sorted catalog union
	//! \param[out] reason Stable failure reason
	//! \return True when every catalog prefab has exactly one matching marker root
	static bool ValidateCoverage(ME_VBT_VehicleBoundsFixtureInventoryResult inventory, array<ref ME_VBT_VehicleBoundsCatalogRecord> records, out string reason)
	{
		reason = "";
		foreach (ME_VBT_VehicleBoundsCatalogRecord record : records)
		{
			if (ME_VBT_VehicleBoundsFixtureInventory.FindMarker(inventory.m_aMarkers, record.m_sPrefab))
				continue;

			PrintFormat("[ME_VBT_WB] fixture_coverage_diff kind=ADDED prefab=%1 faction=%2 vehicle_type=%3", record.m_sPrefab, record.m_sFactionKey, record.m_sVehicleType);
		}

		if (records.Count() != inventory.m_aMarkers.Count())
		{
			reason = string.Format("coverage_count_mismatch catalog=%1 markers=%2", records.Count(), inventory.m_aMarkers.Count());
			return false;
		}

		for (int index = 0; index < records.Count(); index++)
		{
			if (records[index].m_sPrefab != inventory.m_aMarkers[index].m_sPrefab)
			{
				reason = string.Format("coverage_path_mismatch catalog=%1 marker=%2", records[index].m_sPrefab, inventory.m_aMarkers[index].m_sPrefab);
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds a resolved record by canonical prefab path.
	static ME_VBT_VehicleBoundsCatalogRecord FindRecord(array<ref ME_VBT_VehicleBoundsCatalogRecord> records, ResourceName prefab)
	{
		foreach (ME_VBT_VehicleBoundsCatalogRecord record : records)
			if (record.m_sPrefab == prefab)
				return record;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Maps a supported vehicle-type enum label to its stable snapshot name.
	//!
	//! \param[in] label Catalog label to classify
	//! \param[out] labelName Serialized enum name when supported
	//! \return True when the label is a vehicle-type classification
	static bool TryGetVehicleTypeName(EEditableEntityLabel label, out string labelName)
	{
		labelName = "";
		if (label != EEditableEntityLabel.VEHICLE_CAR
			&& label != EEditableEntityLabel.VEHICLE_HELICOPTER
			&& label != EEditableEntityLabel.VEHICLE_AIRPLANE
			&& label != EEditableEntityLabel.VEHICLE_APC
			&& label != EEditableEntityLabel.VEHICLE_TRUCK
			&& label != EEditableEntityLabel.VEHICLE_TURRET)
			return false;

		labelName = typename.EnumToString(EEditableEntityLabel, label);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns whether a serialized label name is a vehicle-type classification.
	static bool IsVehicleTypeName(string labelName)
	{
		return labelName == "VEHICLE_CAR" || labelName == "VEHICLE_HELICOPTER" || labelName == "VEHICLE_AIRPLANE" || labelName == "VEHICLE_APC" || labelName == "VEHICLE_TRUCK" || labelName == "VEHICLE_TURRET";
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts resolved records by canonical prefab path.
	static void SortRecords(array<ref ME_VBT_VehicleBoundsCatalogRecord> records)
	{
		for (int i = 1; i < records.Count(); i++)
		{
			ME_VBT_VehicleBoundsCatalogRecord value = records[i];
			int previous = i - 1;
			while (previous >= 0 && records[previous].m_sPrefab > value.m_sPrefab)
			{
				records[previous + 1] = records[previous];
				previous--;
			}
			records[previous + 1] = value;
		}
	}
}
