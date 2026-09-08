//! Generates, reload-validates, and compares the standalone per-prefab vehicle-bounds snapshot.

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
			Print("[ME_VBT_WB] snapshot_compare status=FAIL reason=game_version_unavailable");
			return;
		}

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_VBT_WB] snapshot_compare status=FAIL reason=world_editor_unavailable");
			return;
		}

		ME_VBT_VehicleBoundsFixtureInventoryResult inventory;
		string reason;
		if (!ME_VBT_VehicleBoundsFixtureInventory.Collect(worldEditor.GetApi(), inventory, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason);
			return;
		}

		array<ref ME_VBT_VehicleBoundsCatalogRecord> catalogRecords;
		if (!ME_VBT_VehicleBoundsCatalogResolver.Resolve(inventory, catalogRecords, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason);
			return;
		}

		if (!ME_VBT_VehicleBoundsCatalogResolver.ValidateCoverage(inventory, catalogRecords, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason);
			return;
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshot candidate = new ME_VBT_VehicleBoundsPerPrefabSnapshot();
		candidate.m_iSchemaVersion = ME_VBT_VehicleBoundsFixtureInventory.SCHEMA_VERSION;
		candidate.m_sGeneratorVersion = ME_VBT_VehicleBoundsFixtureInventory.GENERATOR_VERSION;
		candidate.m_sFixtureIdentity = ME_VBT_VehicleBoundsFixtureInventory.FIXTURE_IDENTITY;
		candidate.m_sGameVersion = gameVersion;
		candidate.m_aEntries = {};

		for (int index = 0; index < catalogRecords.Count(); index++)
		{
			ME_VBT_VehicleBoundsCatalogRecord catalogRecord = catalogRecords[index];
			ME_VBT_VehicleBoundsFixtureMarkerRecord marker = inventory.m_aMarkers[index];
			vector worldMins;
			vector worldMaxs;
			SCR_Global.GetWorldBoundsWithChildren(marker.m_Entity, worldMins, worldMaxs);

			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry = new ME_VBT_VehicleBoundsPerPrefabSnapshotEntry();
			entry.m_sPrefab = catalogRecord.m_sPrefab;
			entry.m_vLocalMins = worldMins - marker.m_Entity.GetOrigin();
			entry.m_vLocalMaxs = worldMaxs - marker.m_Entity.GetOrigin();
			entry.m_aFactionKeys = {};
			entry.m_aFactionKeys.Copy(catalogRecord.m_aFactionKeys);
			entry.m_aVehicleTypes = {};
			entry.m_aVehicleTypes.Copy(catalogRecord.m_aVehicleTypes);
			if (!AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
			{
				PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=invalid_local_bounds path=%1", entry.m_sPrefab);
				return;
			}
			candidate.m_aEntries.Insert(entry);
		}

		if (!ValidateSnapshot(candidate, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1", reason);
			return;
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshot reloadedCandidate;
		if (!SaveAndReloadCandidate(candidate, reloadedCandidate, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=%1 candidate=%2", reason, CANDIDATE_PATH);
			return;
		}

		PrintFormat("[ME_VBT_WB] snapshot_candidate status=PASS candidate=%1 game_version=%2 count=%3", CANDIDATE_PATH, reloadedCandidate.m_sGameVersion, reloadedCandidate.m_aEntries.Count());
		ME_VBT_VehicleBoundsPerPrefabSnapshot baseline;
		if (!LoadBaseline(baseline, reason))
		{
			PrintFormat("[ME_VBT_WB] snapshot_compare status=FAIL reason=baseline_%1 candidate_written=1", reason);
			return;
		}

		CompareSnapshots(baseline, reloadedCandidate);
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the Candidate, rebuilds it, reloads it, and compares the full semantic model.
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

		if (!LoadSnapshot(CANDIDATE_RESOURCE, reloaded, reason))
			return false;
		if (!AreSnapshotsEqual(candidate, reloaded, true))
		{
			reloaded = null;
			reason = "candidate_reload_model_mismatch";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the registered Baseline and permits only the explicit empty bootstrap state.
	//!
	//! \param[out] snapshot Loaded Baseline model
	//! \param[out] reason Stable failure reason
	//! \return True when the Baseline is valid or explicitly uninitialized
	static bool LoadBaseline(out ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		if (!LoadSnapshotInstance(BASELINE_RESOURCE, snapshot, reason))
			return false;
		if (IsUninitializedBaseline(snapshot))
			return true;
		if (ValidateSnapshot(snapshot, reason))
			return true;

		snapshot = null;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads and validates one registered snapshot resource.
	//!
	//! \param[in] resourceName Registered snapshot resource
	//! \param[out] snapshot Loaded validated snapshot model
	//! \param[out] reason Stable failure reason
	//! \return True when the resource contains a valid snapshot
	static bool LoadSnapshot(ResourceName resourceName, out ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		if (!LoadSnapshotInstance(resourceName, snapshot, reason))
			return false;
		if (ValidateSnapshot(snapshot, reason))
			return true;

		snapshot = null;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Deserializes one registered snapshot resource without applying semantic validation.
	//!
	//! \param[in] resourceName Registered snapshot resource
	//! \param[out] snapshot Deserialized snapshot model
	//! \param[out] reason Stable failure reason
	//! \return True when deserialization succeeds
	static bool LoadSnapshotInstance(ResourceName resourceName, out ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		snapshot = null;
		reason = "";
		Resource loaded = Resource.Load(resourceName);
		BaseContainer container;
		if (loaded)
			container = loaded.GetResource().ToBaseContainer();
		if (container)
			snapshot = ME_VBT_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (snapshot)
			return true;

		reason = "snapshot_deserialization_failed";
		return false;
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
			&& snapshot.m_aEntries
			&& snapshot.m_aEntries.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates metadata, ordering, bounds, memberships, and exact fixture count.
	static bool ValidateSnapshot(ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		if (!snapshot || snapshot.m_iSchemaVersion != ME_VBT_VehicleBoundsFixtureInventory.SCHEMA_VERSION || snapshot.m_sGeneratorVersion != ME_VBT_VehicleBoundsFixtureInventory.GENERATOR_VERSION || snapshot.m_sFixtureIdentity != ME_VBT_VehicleBoundsFixtureInventory.FIXTURE_IDENTITY || snapshot.m_sGameVersion.IsEmpty())
		{
			reason = "snapshot_metadata_invalid";
			return false;
		}
		if (!snapshot.m_aEntries || snapshot.m_aEntries.Count() != ME_VBT_VehicleBoundsFixtureInventory.EXPECTED_MARKER_COUNT)
		{
			reason = "snapshot_entry_count_invalid";
			return false;
		}

		ResourceName previousPrefab;
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (!entry || entry.m_sPrefab.IsEmpty() || !previousPrefab.IsEmpty() && entry.m_sPrefab <= previousPrefab)
			{
				reason = "snapshot_entry_order_or_identity_invalid";
				return false;
			}
			if (!AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
			{
				reason = string.Format("snapshot_bounds_invalid path=%1", entry.m_sPrefab);
				return false;
			}
			if (!IsStrictlySortedNonEmpty(entry.m_aFactionKeys) || !IsStrictlySortedNonEmpty(entry.m_aVehicleTypes))
			{
				reason = string.Format("snapshot_memberships_invalid path=%1", entry.m_sPrefab);
				return false;
			}
			foreach (string vehicleType : entry.m_aVehicleTypes)
			{
				if (!ME_VBT_VehicleBoundsCatalogResolver.IsVehicleTypeName(vehicleType))
				{
					reason = string.Format("snapshot_vehicle_type_invalid path=%1 type=%2", entry.m_sPrefab, vehicleType);
					return false;
				}
			}
			previousPrefab = entry.m_sPrefab;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns stable readable names to all serialized entry containers.
	static bool SetDeterministicContainerNames(BaseContainer container, ME_VBT_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList entryContainers = container.GetObjectArray("m_aEntries");
		if (!entryContainers || entryContainers.Count() != snapshot.m_aEntries.Count())
		{
			reason = "serialized_entry_container_count_mismatch";
			return false;
		}

		array<string> names = {};
		for (int index = 0; index < entryContainers.Count(); index++)
		{
			BaseContainer entryContainer = entryContainers.Get(index);
			string name;
			if (!entryContainer || !ME_VBT_VehicleBoundsFixtureInventory.GetPrefabStem(snapshot.m_aEntries[index].m_sPrefab, name) || names.Contains(name))
			{
				reason = string.Format("serialized_entry_container_name_invalid path=%1", snapshot.m_aEntries[index].m_sPrefab);
				return false;
			}
			entryContainer.SetName(name);
			names.Insert(name);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Performs a deterministic linear comparison and reports semantic differences.
	static void CompareSnapshots(ME_VBT_VehicleBoundsPerPrefabSnapshot baseline, ME_VBT_VehicleBoundsPerPrefabSnapshot candidate)
	{
		PrintFormat("[ME_VBT_WB] snapshot_compare phase=context baseline_game_version=%1 candidate_game_version=%2", baseline.m_sGameVersion, candidate.m_sGameVersion);
		int baselineIndex;
		int candidateIndex;
		int added;
		int removed;
		int changed;
		while (baselineIndex < baseline.m_aEntries.Count() || candidateIndex < candidate.m_aEntries.Count())
		{
			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry baselineEntry;
			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry candidateEntry;
			if (baselineIndex < baseline.m_aEntries.Count())
				baselineEntry = baseline.m_aEntries[baselineIndex];
			if (candidateIndex < candidate.m_aEntries.Count())
				candidateEntry = candidate.m_aEntries[candidateIndex];

			if (!baselineEntry || candidateEntry && candidateEntry.m_sPrefab < baselineEntry.m_sPrefab)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=ADDED prefab=%1", candidateEntry.m_sPrefab);
				added++;
				candidateIndex++;
				continue;
			}
			if (!candidateEntry || baselineEntry.m_sPrefab < candidateEntry.m_sPrefab)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=REMOVED prefab=%1", baselineEntry.m_sPrefab);
				removed++;
				baselineIndex++;
				continue;
			}

			bool boundsChanged = !AreVectorsClose(baselineEntry.m_vLocalMins, candidateEntry.m_vLocalMins) || !AreVectorsClose(baselineEntry.m_vLocalMaxs, candidateEntry.m_vLocalMaxs);
			bool factionsChanged = !AreStringArraysEqual(baselineEntry.m_aFactionKeys, candidateEntry.m_aFactionKeys);
			bool vehicleTypesChanged = !AreStringArraysEqual(baselineEntry.m_aVehicleTypes, candidateEntry.m_aVehicleTypes);
			if (boundsChanged || factionsChanged || vehicleTypesChanged)
			{
				PrintFormat("[ME_VBT_WB] snapshot_diff kind=CHANGED prefab=%1 bounds=%2 factions=%3 vehicle_types=%4 baseline_mins=%5 baseline_maxs=%6 candidate_mins=%7 candidate_maxs=%8", baselineEntry.m_sPrefab, boundsChanged, factionsChanged, vehicleTypesChanged, baselineEntry.m_vLocalMins, baselineEntry.m_vLocalMaxs, candidateEntry.m_vLocalMins, candidateEntry.m_vLocalMaxs);
				changed++;
			}
			baselineIndex++;
			candidateIndex++;
		}

		string status = "PASS";
		if (added > 0 || removed > 0 || changed > 0)
			status = "DIFF";
		PrintFormat("[ME_VBT_WB] snapshot_compare status=%1 baseline_game_version=%2 candidate_game_version=%3 baseline_count=%4 candidate_count=%5 added=%6 removed=%7 changed=%8 candidate_written=1", status, baseline.m_sGameVersion, candidate.m_sGameVersion, baseline.m_aEntries.Count(), candidate.m_aEntries.Count(), added, removed, changed);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares complete snapshot models, optionally including game build identity.
	static bool AreSnapshotsEqual(ME_VBT_VehicleBoundsPerPrefabSnapshot left, ME_VBT_VehicleBoundsPerPrefabSnapshot right, bool compareGameVersion)
	{
		if (!left || !right || left.m_iSchemaVersion != right.m_iSchemaVersion || left.m_sGeneratorVersion != right.m_sGeneratorVersion || left.m_sFixtureIdentity != right.m_sFixtureIdentity || compareGameVersion && left.m_sGameVersion != right.m_sGameVersion || left.m_aEntries.Count() != right.m_aEntries.Count())
			return false;
		for (int index = 0; index < left.m_aEntries.Count(); index++)
		{
			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry leftEntry = left.m_aEntries[index];
			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry rightEntry = right.m_aEntries[index];
			if (leftEntry.m_sPrefab != rightEntry.m_sPrefab || !AreVectorsClose(leftEntry.m_vLocalMins, rightEntry.m_vLocalMins) || !AreVectorsClose(leftEntry.m_vLocalMaxs, rightEntry.m_vLocalMaxs) || !AreStringArraysEqual(leftEntry.m_aFactionKeys, rightEntry.m_aFactionKeys) || !AreStringArraysEqual(leftEntry.m_aVehicleTypes, rightEntry.m_aVehicleTypes))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks that a string array is non-empty, sorted, and duplicate-free.
	static bool IsStrictlySortedNonEmpty(array<string> values)
	{
		if (!values || values.IsEmpty())
			return false;
		for (int index = 1; index < values.Count(); index++)
			if (values[index] <= values[index - 1])
				return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares two ordered string arrays.
	static bool AreStringArraysEqual(array<string> left, array<string> right)
	{
		if (!left || !right || left.Count() != right.Count())
			return false;
		for (int index = 0; index < left.Count(); index++)
			if (left[index] != right[index])
				return false;
		return true;
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
