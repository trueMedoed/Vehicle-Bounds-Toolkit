//! Generates, reload-validates, and compares the standalone per-prefab vehicle-bounds snapshot.

//------------------------------------------------------------------------------------------------
//! One flattened snapshot entry with its parent membership context.
class ME_VBT_VehicleBoundsFlatRecord
{
	string m_sFactionKey;
	string m_sVehicleType;
	ResourceName m_sPrefab;
	ME_VBT_VehicleBoundsPerPrefabSnapshotEntry m_Entry;
}

//------------------------------------------------------------------------------------------------
//! Measures every catalog-backed fixture root and writes only the registered Candidate resource.
[WorkbenchPluginAttribute(name: "VBT: Generate bounds snapshot", description: "Measures the fixture, writes the Candidate, reload-validates it, and compares it with the accepted Baseline.", wbModules: { "WorldEditor" }, category: "ME Vehicle Bounds Toolkit")]
class ME_VBT_GenerateVehicleBoundsSnapshotPlugin : WorldEditorPlugin
{
	protected const string CANDIDATE_PATH = "Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf";
	protected const int CANDIDATE_REBUILD_TIMEOUT_MS = 5000;
	protected static const ResourceName CANDIDATE_RESOURCE = "{0F8D7A7D004E2D06}Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf";
	protected static const ResourceName BASELINE_RESOURCE = "{272D22AD3B7E45D1}Configs/Generated/ME_VBT_VehicleBoundsPerPrefabBaseline.conf";

	//------------------------------------------------------------------------------------------------
	//! Generates and compares a complete snapshot from the open fixture.
	override void Run()
	{
		Game game = GetGame();
		string gameVersion;
		if (game)
			gameVersion = game.GetBuildVersion();
		if (gameVersion.IsEmpty())
		{
			Print("[ME_VBT_WB] snapshot_compare status=FAIL reason=game_version_unavailable", LogLevel.ERROR);
			return;
		}

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_VBT_WB] snapshot_compare status=FAIL reason=world_editor_unavailable", LogLevel.ERROR);
			return;
		}

		ME_VBT_VehicleBoundsFixtureInventoryResult inventory;
		string reason;
		if (!ME_VBT_VehicleBoundsFixtureInventory.Collect(worldEditor.GetApi(), inventory, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason), LogLevel.ERROR);
			return;
		}

		array<ref ME_VBT_VehicleBoundsCatalogRecord> catalogRecords;
		if (!ME_VBT_VehicleBoundsCatalogResolver.Resolve(inventory, catalogRecords, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason), LogLevel.ERROR);
			return;
		}

		if (!ME_VBT_VehicleBoundsCatalogResolver.ValidateCoverage(inventory, catalogRecords, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason), LogLevel.ERROR);
			return;
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshot candidate = new ME_VBT_VehicleBoundsPerPrefabSnapshot();
		candidate.m_iSchemaVersion = ME_VBT_VehicleBoundsFixtureInventory.SCHEMA_VERSION;
		candidate.m_sGeneratorVersion = ME_VBT_VehicleBoundsFixtureInventory.GENERATOR_VERSION;
		candidate.m_sFixtureIdentity = ME_VBT_VehicleBoundsFixtureInventory.FIXTURE_IDENTITY;
		candidate.m_sGameVersion = gameVersion;
		candidate.m_aFactions = {};

		for (int index = 0; index < catalogRecords.Count(); index++)
		{
			ME_VBT_VehicleBoundsCatalogRecord catalogRecord = catalogRecords[index];
			ME_VBT_VehicleBoundsFixtureMarkerRecord marker = inventory.m_aMarkers[index];
			vector worldMins;
			vector worldMaxs;
			SCR_Global.GetWorldBoundsWithChildren(marker.m_Entity, worldMins, worldMaxs);

			vector localMins = worldMins - marker.m_Entity.GetOrigin();
			vector localMaxs = worldMaxs - marker.m_Entity.GetOrigin();
			if (!AreFiniteOrderedBounds(localMins, localMaxs))
			{
				Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=invalid_local_bounds path=%1", catalogRecord.m_sPrefab), LogLevel.ERROR);
				return;
			}
			AddMeasuredEntry(candidate, catalogRecord, localMins, localMaxs);
		}

		SortSnapshot(candidate);
		if (!ValidateSnapshot(candidate, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason), LogLevel.ERROR);
			return;
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshot reloadedCandidate;
		if (!SaveAndReloadCandidate(candidate, reloadedCandidate, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1 candidate=%2", reason, CANDIDATE_PATH), LogLevel.ERROR);
			return;
		}

		PrintFormat("[ME_VBT_WB] snapshot_candidate status=PASS candidate=%1 game_version=%2 count=%3", CANDIDATE_PATH, reloadedCandidate.m_sGameVersion, CountEntries(reloadedCandidate));
		ME_VBT_VehicleBoundsPerPrefabSnapshot baseline;
		if (!LoadBaseline(baseline, reason))
		{
			Print(string.Format("[ME_VBT_WB] snapshot_compare status=FAIL reason=baseline_%1 candidate_written=1", reason), LogLevel.ERROR);
			return;
		}

		CompareSnapshots(baseline, reloadedCandidate);
	}

	//------------------------------------------------------------------------------------------------
	//! Inserts one measured prefab under its single faction and basic vehicle type.
	static void AddMeasuredEntry(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, ME_VBT_VehicleBoundsCatalogRecord catalogRecord, vector localMins, vector localMaxs)
	{
		ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction = FindFaction(snapshot, catalogRecord.m_sFactionKey);
		if (!faction)
		{
			faction = new ME_VBT_VehicleBoundsPerPrefabSnapshotFaction();
			faction.m_sFactionKey = catalogRecord.m_sFactionKey;
			faction.m_aVehicleTypes = {};
			snapshot.m_aFactions.Insert(faction);
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType = FindVehicleType(faction, catalogRecord.m_sVehicleType);
		if (!vehicleType)
		{
			vehicleType = new ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType();
			vehicleType.m_sVehicleType = catalogRecord.m_sVehicleType;
			vehicleType.m_aEntries = {};
			faction.m_aVehicleTypes.Insert(vehicleType);
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry = new ME_VBT_VehicleBoundsPerPrefabSnapshotEntry();
		entry.m_sPrefab = catalogRecord.m_sPrefab;
		entry.m_vLocalMins = localMins;
		entry.m_vLocalMaxs = localMaxs;
		vehicleType.m_aEntries.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one exact faction group.
	static ME_VBT_VehicleBoundsPerPrefabSnapshotFaction FindFaction(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, string factionKey)
	{
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : snapshot.m_aFactions)
			if (faction.m_sFactionKey == factionKey)
				return faction;
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one exact basic vehicle-type group inside a faction.
	static ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType FindVehicleType(ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction, string vehicleType)
	{
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType group : faction.m_aVehicleTypes)
			if (group.m_sVehicleType == vehicleType)
				return group;
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts factions, nested vehicle types, and prefab entries deterministically.
	static void SortSnapshot(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot)
	{
		for (int factionIndex = 1; factionIndex < snapshot.m_aFactions.Count(); factionIndex++)
		{
			ME_VBT_VehicleBoundsPerPrefabSnapshotFaction factionValue = snapshot.m_aFactions[factionIndex];
			int previousFaction = factionIndex - 1;
			while (previousFaction >= 0 && snapshot.m_aFactions[previousFaction].m_sFactionKey > factionValue.m_sFactionKey)
			{
				snapshot.m_aFactions[previousFaction + 1] = snapshot.m_aFactions[previousFaction];
				previousFaction--;
			}
			snapshot.m_aFactions[previousFaction + 1] = factionValue;
		}

		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : snapshot.m_aFactions)
		{
			for (int typeIndex = 1; typeIndex < faction.m_aVehicleTypes.Count(); typeIndex++)
			{
				ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType typeValue = faction.m_aVehicleTypes[typeIndex];
				int previousType = typeIndex - 1;
				while (previousType >= 0 && faction.m_aVehicleTypes[previousType].m_sVehicleType > typeValue.m_sVehicleType)
				{
					faction.m_aVehicleTypes[previousType + 1] = faction.m_aVehicleTypes[previousType];
					previousType--;
				}
				faction.m_aVehicleTypes[previousType + 1] = typeValue;
			}

			foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType : faction.m_aVehicleTypes)
			{
				for (int entryIndex = 1; entryIndex < vehicleType.m_aEntries.Count(); entryIndex++)
				{
					ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entryValue = vehicleType.m_aEntries[entryIndex];
					int previousEntry = entryIndex - 1;
					while (previousEntry >= 0 && vehicleType.m_aEntries[previousEntry].m_sPrefab > entryValue.m_sPrefab)
					{
						vehicleType.m_aEntries[previousEntry + 1] = vehicleType.m_aEntries[previousEntry];
						previousEntry--;
					}
					vehicleType.m_aEntries[previousEntry + 1] = entryValue;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the Candidate, rebuilds it, reloads it cache-safely, and compares the full model.
	bool SaveAndReloadCandidate(ME_VBT_VehicleBoundsPerPrefabSnapshot candidate, out ME_VBT_VehicleBoundsPerPrefabSnapshot reloaded, out string reason)
	{
		reloaded = null;
		reason = "";
		Resource containerResource = BaseContainerTools.CreateContainerFromInstance(candidate);
		string absolutePath;
		if (!containerResource || !Workbench.GetAbsolutePath(CANDIDATE_PATH, absolutePath, false))
		{
			reason = "candidate_create_or_path_failed";
			return false;
		}

		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container || !SetDeterministicContainerNames(container, candidate, reason))
			return false;
		if (!BaseContainerTools.SaveContainer(container, CANDIDATE_RESOURCE, absolutePath))
		{
			reason = "candidate_save_container_failed";
			return false;
		}

		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager)
		{
			reason = "candidate_resource_manager_unavailable";
			return false;
		}
		resourceManager.RebuildResourceFile(absolutePath, "", false);
		if (!resourceManager.WaitForFile(absolutePath, CANDIDATE_REBUILD_TIMEOUT_MS))
		{
			reason = string.Format("candidate_resource_wait_failed timeout_ms=%1", CANDIDATE_REBUILD_TIMEOUT_MS);
			return false;
		}

		Resource loaded = BaseContainerTools.LoadContainer(CANDIDATE_RESOURCE);
		BaseContainer loadedContainer;
		if (loaded)
			loadedContainer = loaded.GetResource().ToBaseContainer();
		if (!loadedContainer)
		{
			reason = "candidate_reload_container_failed";
			return false;
		}

		reloaded = ME_VBT_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloaded)
		{
			reason = "candidate_reload_deserialization_failed";
			return false;
		}
		if (!ValidateSnapshot(reloaded, reason) || !ValidateRawContainerHierarchy(loadedContainer, reloaded, reason))
		{
			reloaded = null;
			return false;
		}
		if (!AreSnapshotsEqual(candidate, reloaded, true))
		{
			reloaded = null;
			reason = "candidate_reload_model_mismatch";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the registered Baseline and permits only the explicit grouped-v2 bootstrap state.
	static bool LoadBaseline(out ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		snapshot = null;
		reason = "";
		Resource loaded = BaseContainerTools.LoadContainer(BASELINE_RESOURCE);
		BaseContainer container;
		if (loaded)
			container = loaded.GetResource().ToBaseContainer();
		if (container)
			snapshot = ME_VBT_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!snapshot)
		{
			reason = "snapshot_deserialization_failed";
			return false;
		}

		if (!IsUninitializedBaseline(snapshot) && !ValidateSnapshot(snapshot, reason))
		{
			snapshot = null;
			return false;
		}
		if (!ValidateRawContainerHierarchy(container, snapshot, reason))
		{
			snapshot = null;
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Recognizes the only bootstrap Baseline state allowed before first manual acceptance.
	static bool IsUninitializedBaseline(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot)
	{
		return snapshot
			&& snapshot.m_iSchemaVersion == ME_VBT_VehicleBoundsFixtureInventory.SCHEMA_VERSION
			&& snapshot.m_sGeneratorVersion == ME_VBT_VehicleBoundsFixtureInventory.GENERATOR_VERSION
			&& snapshot.m_sFixtureIdentity == ME_VBT_VehicleBoundsFixtureInventory.FIXTURE_IDENTITY
			&& snapshot.m_sGameVersion == "UNINITIALIZED"
			&& snapshot.m_aFactions
			&& snapshot.m_aFactions.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates metadata, the grouped hierarchy, global prefab uniqueness, bounds, and exact count.
	static bool ValidateSnapshot(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		if (!snapshot || snapshot.m_iSchemaVersion != ME_VBT_VehicleBoundsFixtureInventory.SCHEMA_VERSION || snapshot.m_sGeneratorVersion != ME_VBT_VehicleBoundsFixtureInventory.GENERATOR_VERSION || snapshot.m_sFixtureIdentity != ME_VBT_VehicleBoundsFixtureInventory.FIXTURE_IDENTITY || snapshot.m_sGameVersion.IsEmpty())
		{
			reason = "snapshot_metadata_invalid";
			return false;
		}
		if (!snapshot.m_aFactions || snapshot.m_aFactions.IsEmpty())
		{
			reason = "snapshot_factions_empty";
			return false;
		}

		array<string> prefabPaths = {};
		string previousFactionKey;
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : snapshot.m_aFactions)
		{
			if (!faction || faction.m_sFactionKey.IsEmpty() || !faction.m_aVehicleTypes || faction.m_aVehicleTypes.IsEmpty() || !previousFactionKey.IsEmpty() && faction.m_sFactionKey <= previousFactionKey)
			{
				reason = "snapshot_faction_invalid_or_unsorted";
				return false;
			}

			string previousVehicleType;
			foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType : faction.m_aVehicleTypes)
			{
				if (!vehicleType || !ME_VBT_VehicleBoundsCatalogResolver.IsVehicleTypeName(vehicleType.m_sVehicleType) || !vehicleType.m_aEntries || vehicleType.m_aEntries.IsEmpty() || !previousVehicleType.IsEmpty() && vehicleType.m_sVehicleType <= previousVehicleType)
				{
					reason = string.Format("snapshot_vehicle_type_invalid_or_unsorted faction=%1", faction.m_sFactionKey);
					return false;
				}

				ResourceName previousPrefab;
				foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry : vehicleType.m_aEntries)
				{
					if (!entry || entry.m_sPrefab.IsEmpty() || !previousPrefab.IsEmpty() && entry.m_sPrefab <= previousPrefab)
					{
						reason = string.Format("snapshot_entry_order_or_identity_invalid faction=%1 type=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
						return false;
					}
					if (prefabPaths.Contains(entry.m_sPrefab))
					{
						reason = string.Format("snapshot_prefab_duplicate path=%1", entry.m_sPrefab);
						return false;
					}
					if (!AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
					{
						reason = string.Format("snapshot_bounds_invalid path=%1", entry.m_sPrefab);
						return false;
					}
					prefabPaths.Insert(entry.m_sPrefab);
					previousPrefab = entry.m_sPrefab;
				}
				previousVehicleType = vehicleType.m_sVehicleType;
			}
			previousFactionKey = faction.m_sFactionKey;
		}

		if (prefabPaths.Count() != ME_VBT_VehicleBoundsFixtureInventory.EXPECTED_MARKER_COUNT)
		{
			reason = string.Format("snapshot_entry_count_invalid actual=%1 expected=%2", prefabPaths.Count(), ME_VBT_VehicleBoundsFixtureInventory.EXPECTED_MARKER_COUNT);
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns identifier-safe, collision-free names to factions, types, and prefab entries.
	static bool SetDeterministicContainerNames(BaseContainer container, ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList factionContainers = container.GetObjectArray("m_aFactions");
		if (!factionContainers || !snapshot || !snapshot.m_aFactions || factionContainers.Count() != snapshot.m_aFactions.Count())
		{
			reason = "serialized_faction_container_count_mismatch";
			return false;
		}

		array<string> factionNames = {};
		for (int factionIndex = 0; factionIndex < factionContainers.Count(); factionIndex++)
		{
			BaseContainer factionContainer = factionContainers.Get(factionIndex);
			ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction = snapshot.m_aFactions[factionIndex];
			if (!factionContainer || !faction || !IsSafeContainerName(faction.m_sFactionKey) || factionNames.Contains(faction.m_sFactionKey))
			{
				reason = string.Format("serialized_faction_container_name_invalid key=%1", faction.m_sFactionKey);
				return false;
			}
			factionContainer.SetName(faction.m_sFactionKey);
			factionNames.Insert(faction.m_sFactionKey);

			BaseContainerList typeContainers = factionContainer.GetObjectArray("m_aVehicleTypes");
			if (!typeContainers || !faction.m_aVehicleTypes || typeContainers.Count() != faction.m_aVehicleTypes.Count())
			{
				reason = string.Format("serialized_type_container_count_mismatch faction=%1", faction.m_sFactionKey);
				return false;
			}

			array<string> typeNames = {};
			for (int typeIndex = 0; typeIndex < typeContainers.Count(); typeIndex++)
			{
				BaseContainer typeContainer = typeContainers.Get(typeIndex);
				ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType = faction.m_aVehicleTypes[typeIndex];
				if (!typeContainer || !vehicleType || !IsSafeContainerName(vehicleType.m_sVehicleType) || typeNames.Contains(vehicleType.m_sVehicleType))
				{
					reason = string.Format("serialized_type_container_name_invalid faction=%1 type=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
					return false;
				}
				typeContainer.SetName(vehicleType.m_sVehicleType);
				typeNames.Insert(vehicleType.m_sVehicleType);

				BaseContainerList entryContainers = typeContainer.GetObjectArray("m_aEntries");
				if (!entryContainers || !vehicleType.m_aEntries || entryContainers.Count() != vehicleType.m_aEntries.Count())
				{
					reason = string.Format("serialized_entry_container_count_mismatch faction=%1 type=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
					return false;
				}

				array<string> entryNames = {};
				for (int entryIndex = 0; entryIndex < entryContainers.Count(); entryIndex++)
				{
					BaseContainer entryContainer = entryContainers.Get(entryIndex);
					string entryName;
					if (!entryContainer || !ME_VBT_VehicleBoundsFixtureInventory.GetPrefabStem(vehicleType.m_aEntries[entryIndex].m_sPrefab, entryName) || !IsSafeContainerName(entryName) || entryNames.Contains(entryName))
					{
						reason = string.Format("serialized_entry_container_name_invalid path=%1", vehicleType.m_aEntries[entryIndex].m_sPrefab);
						return false;
					}
					entryContainer.SetName(entryName);
					entryNames.Insert(entryName);
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies raw hierarchy counts, order, and names against a validated typed snapshot.
	static bool ValidateRawContainerHierarchy(BaseContainer container, ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList factionContainers = container.GetObjectArray("m_aFactions");
		if (!factionContainers || !snapshot || !snapshot.m_aFactions || factionContainers.Count() != snapshot.m_aFactions.Count())
		{
			reason = "reload_faction_container_count_mismatch";
			return false;
		}

		array<string> factionNames = {};
		for (int factionIndex = 0; factionIndex < factionContainers.Count(); factionIndex++)
		{
			BaseContainer factionContainer = factionContainers.Get(factionIndex);
			ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction = snapshot.m_aFactions[factionIndex];
			if (!factionContainer || !faction || factionContainer.GetName() != faction.m_sFactionKey || !IsSafeContainerName(faction.m_sFactionKey) || factionNames.Contains(faction.m_sFactionKey))
			{
				reason = string.Format("reload_faction_container_name_mismatch expected=%1", faction.m_sFactionKey);
				return false;
			}
			factionNames.Insert(faction.m_sFactionKey);

			BaseContainerList typeContainers = factionContainer.GetObjectArray("m_aVehicleTypes");
			if (!typeContainers || !faction.m_aVehicleTypes || typeContainers.Count() != faction.m_aVehicleTypes.Count())
			{
				reason = string.Format("reload_type_container_count_mismatch faction=%1", faction.m_sFactionKey);
				return false;
			}

			array<string> typeNames = {};
			for (int typeIndex = 0; typeIndex < typeContainers.Count(); typeIndex++)
			{
				BaseContainer typeContainer = typeContainers.Get(typeIndex);
				ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType = faction.m_aVehicleTypes[typeIndex];
				if (!typeContainer || !vehicleType || typeContainer.GetName() != vehicleType.m_sVehicleType || !IsSafeContainerName(vehicleType.m_sVehicleType) || typeNames.Contains(vehicleType.m_sVehicleType))
				{
					reason = string.Format("reload_type_container_name_mismatch faction=%1 expected=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
					return false;
				}
				typeNames.Insert(vehicleType.m_sVehicleType);

				BaseContainerList entryContainers = typeContainer.GetObjectArray("m_aEntries");
				if (!entryContainers || !vehicleType.m_aEntries || entryContainers.Count() != vehicleType.m_aEntries.Count())
				{
					reason = string.Format("reload_entry_container_count_mismatch faction=%1 type=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
					return false;
				}

				array<string> entryNames = {};
				for (int entryIndex = 0; entryIndex < entryContainers.Count(); entryIndex++)
				{
					BaseContainer entryContainer = entryContainers.Get(entryIndex);
					string expectedName;
					if (!ME_VBT_VehicleBoundsFixtureInventory.GetPrefabStem(vehicleType.m_aEntries[entryIndex].m_sPrefab, expectedName) || !entryContainer || entryContainer.GetName() != expectedName || !IsSafeContainerName(expectedName) || entryNames.Contains(expectedName))
					{
						reason = string.Format("reload_entry_container_name_mismatch path=%1 expected=%2", vehicleType.m_aEntries[entryIndex].m_sPrefab, expectedName);
						return false;
					}
					entryNames.Insert(expectedName);
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Accepts only identifier-safe ASCII letters, digits, and underscores in a container name.
	static bool IsSafeContainerName(string value)
	{
		if (value.IsEmpty())
			return false;
		string firstCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
		if (!firstCharacters.Contains(value.Substring(0, 1)))
			return false;
		string allowedCharacters = firstCharacters + "0123456789";
		for (int index = 0; index < value.Length(); index++)
			if (!allowedCharacters.Contains(value.Substring(index, 1)))
				return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Flattens a grouped snapshot and sorts records globally by canonical prefab path.
	static void FlattenSnapshot(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out array<ref ME_VBT_VehicleBoundsFlatRecord> records)
	{
		records = {};
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : snapshot.m_aFactions)
		{
			foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType : faction.m_aVehicleTypes)
			{
				foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry : vehicleType.m_aEntries)
				{
					ME_VBT_VehicleBoundsFlatRecord record = new ME_VBT_VehicleBoundsFlatRecord();
					record.m_sFactionKey = faction.m_sFactionKey;
					record.m_sVehicleType = vehicleType.m_sVehicleType;
					record.m_sPrefab = entry.m_sPrefab;
					record.m_Entry = entry;
					records.Insert(record);
				}
			}
		}

		for (int index = 1; index < records.Count(); index++)
		{
			ME_VBT_VehicleBoundsFlatRecord value = records[index];
			int previous = index - 1;
			while (previous >= 0 && records[previous].m_sPrefab > value.m_sPrefab)
			{
				records[previous + 1] = records[previous];
				previous--;
			}
			records[previous + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Performs a deterministic prefab-keyed comparison and reports bounds or membership changes.
	static void CompareSnapshots(ME_VBT_VehicleBoundsPerPrefabSnapshot baseline, ME_VBT_VehicleBoundsPerPrefabSnapshot candidate)
	{
		PrintFormat("[ME_VBT_WB] snapshot_compare phase=context baseline_game_version=%1 candidate_game_version=%2", baseline.m_sGameVersion, candidate.m_sGameVersion);
		array<ref ME_VBT_VehicleBoundsFlatRecord> baselineRecords;
		array<ref ME_VBT_VehicleBoundsFlatRecord> candidateRecords;
		FlattenSnapshot(baseline, baselineRecords);
		FlattenSnapshot(candidate, candidateRecords);
		int baselineIndex;
		int candidateIndex;
		int added;
		int removed;
		int changed;
		while (baselineIndex < baselineRecords.Count() || candidateIndex < candidateRecords.Count())
		{
			ME_VBT_VehicleBoundsFlatRecord baselineRecord;
			ME_VBT_VehicleBoundsFlatRecord candidateRecord;
			if (baselineIndex < baselineRecords.Count())
				baselineRecord = baselineRecords[baselineIndex];
			if (candidateIndex < candidateRecords.Count())
				candidateRecord = candidateRecords[candidateIndex];

			if (!baselineRecord || candidateRecord && candidateRecord.m_sPrefab < baselineRecord.m_sPrefab)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=ADDED prefab=%1 faction=%2 vehicle_type=%3", candidateRecord.m_sPrefab, candidateRecord.m_sFactionKey, candidateRecord.m_sVehicleType);
				added++;
				candidateIndex++;
				continue;
			}
			if (!candidateRecord || baselineRecord.m_sPrefab < candidateRecord.m_sPrefab)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=REMOVED prefab=%1 faction=%2 vehicle_type=%3", baselineRecord.m_sPrefab, baselineRecord.m_sFactionKey, baselineRecord.m_sVehicleType);
				removed++;
				baselineIndex++;
				continue;
			}

			bool boundsChanged = !AreVectorsClose(baselineRecord.m_Entry.m_vLocalMins, candidateRecord.m_Entry.m_vLocalMins) || !AreVectorsClose(baselineRecord.m_Entry.m_vLocalMaxs, candidateRecord.m_Entry.m_vLocalMaxs);
			bool factionChanged = baselineRecord.m_sFactionKey != candidateRecord.m_sFactionKey;
			bool vehicleTypeChanged = baselineRecord.m_sVehicleType != candidateRecord.m_sVehicleType;
			if (boundsChanged || factionChanged || vehicleTypeChanged)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=CHANGED prefab=%1 bounds=%2 faction=%3 vehicle_type=%4 baseline_faction=%5 baseline_vehicle_type=%6 candidate_faction=%7 candidate_vehicle_type=%8", baselineRecord.m_sPrefab, boundsChanged, factionChanged, vehicleTypeChanged, baselineRecord.m_sFactionKey, baselineRecord.m_sVehicleType, candidateRecord.m_sFactionKey, candidateRecord.m_sVehicleType);
				PrintFormat("[ME_VBT_WB] snapshot_diff_bounds prefab=%1 baseline_mins=%2 baseline_maxs=%3 candidate_mins=%4 candidate_maxs=%5", baselineRecord.m_sPrefab, baselineRecord.m_Entry.m_vLocalMins, baselineRecord.m_Entry.m_vLocalMaxs, candidateRecord.m_Entry.m_vLocalMins, candidateRecord.m_Entry.m_vLocalMaxs);
				changed++;
			}
			baselineIndex++;
			candidateIndex++;
		}

		string status = "PASS";
		if (added > 0 || removed > 0 || changed > 0)
			status = "DIFF";
		PrintFormat("[ME_VBT_WB] snapshot_compare status=%1 baseline_game_version=%2 candidate_game_version=%3 baseline_count=%4 candidate_count=%5 added=%6 removed=%7 changed=%8 candidate_written=1", status, baseline.m_sGameVersion, candidate.m_sGameVersion, baselineRecords.Count(), candidateRecords.Count(), added, removed, changed);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares complete grouped snapshot models, optionally including game build identity.
	static bool AreSnapshotsEqual(ME_VBT_VehicleBoundsPerPrefabSnapshot left, ME_VBT_VehicleBoundsPerPrefabSnapshot right, bool compareGameVersion)
	{
		if (!left || !right || left.m_iSchemaVersion != right.m_iSchemaVersion || left.m_sGeneratorVersion != right.m_sGeneratorVersion || left.m_sFixtureIdentity != right.m_sFixtureIdentity || compareGameVersion && left.m_sGameVersion != right.m_sGameVersion || !left.m_aFactions || !right.m_aFactions || left.m_aFactions.Count() != right.m_aFactions.Count())
			return false;
		for (int factionIndex = 0; factionIndex < left.m_aFactions.Count(); factionIndex++)
		{
			ME_VBT_VehicleBoundsPerPrefabSnapshotFaction leftFaction = left.m_aFactions[factionIndex];
			ME_VBT_VehicleBoundsPerPrefabSnapshotFaction rightFaction = right.m_aFactions[factionIndex];
			if (!leftFaction || !rightFaction || leftFaction.m_sFactionKey != rightFaction.m_sFactionKey || !leftFaction.m_aVehicleTypes || !rightFaction.m_aVehicleTypes || leftFaction.m_aVehicleTypes.Count() != rightFaction.m_aVehicleTypes.Count())
				return false;
			for (int typeIndex = 0; typeIndex < leftFaction.m_aVehicleTypes.Count(); typeIndex++)
			{
				ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType leftType = leftFaction.m_aVehicleTypes[typeIndex];
				ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType rightType = rightFaction.m_aVehicleTypes[typeIndex];
				if (!leftType || !rightType || leftType.m_sVehicleType != rightType.m_sVehicleType || !leftType.m_aEntries || !rightType.m_aEntries || leftType.m_aEntries.Count() != rightType.m_aEntries.Count())
					return false;
				for (int entryIndex = 0; entryIndex < leftType.m_aEntries.Count(); entryIndex++)
				{
					ME_VBT_VehicleBoundsPerPrefabSnapshotEntry leftEntry = leftType.m_aEntries[entryIndex];
					ME_VBT_VehicleBoundsPerPrefabSnapshotEntry rightEntry = rightType.m_aEntries[entryIndex];
					if (!leftEntry || !rightEntry || leftEntry.m_sPrefab != rightEntry.m_sPrefab || !AreVectorsClose(leftEntry.m_vLocalMins, rightEntry.m_vLocalMins) || !AreVectorsClose(leftEntry.m_vLocalMaxs, rightEntry.m_vLocalMaxs))
						return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the total number of prefab entries across all groups.
	static int CountEntries(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot)
	{
		int count;
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : snapshot.m_aFactions)
			foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType : faction.m_aVehicleTypes)
				count += vehicleType.m_aEntries.Count();
		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, and unreasonable bounds.
	static bool AreFiniteOrderedBounds(vector mins, vector maxs)
	{
		for (int axis = 0; axis < 3; axis++)
			if (mins[axis] != mins[axis] || maxs[axis] != maxs[axis] || Math.AbsFloat(mins[axis]) > 1000000 || Math.AbsFloat(maxs[axis]) > 1000000 || mins[axis] > maxs[axis])
				return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares vectors within config serialization precision.
	static bool AreVectorsClose(vector left, vector right)
	{
		for (int axis = 0; axis < 3; axis++)
			if (Math.AbsFloat(left[axis] - right[axis]) > 0.0011)
				return false;
		return true;
	}
}
