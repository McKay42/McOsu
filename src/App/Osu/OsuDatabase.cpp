//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu!.db + collection.db + raw loader + scores etc.
//
// $NoKeywords: $osubdb
//===============================================================================//

#include "OsuDatabase.h"

#include "Engine.h"
#include "ConVar.h"
#include "Timer.h"
#include "File.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuFile.h"
#include "OsuReplay.h"
#include "OsuScore.h"
#include "OsuNotificationOverlay.h"

#include "OsuDatabaseBeatmap.h"

#include <fstream>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

ConVar osu_folder("osu_folder", "C:/Program Files (x86)/osu!/", FCVAR_NONE);

#elif defined __linux__

ConVar osu_folder("osu_folder", "/home/pg/Desktop/osu!/", FCVAR_NONE);

#elif defined __APPLE__

ConVar osu_folder("osu_folder", "/osu!/", FCVAR_NONE);

#elif defined __SWITCH__

ConVar osu_folder("osu_folder", "sdmc:/switch/McOsu/", FCVAR_NONE);

#else

#error "put correct default folder convar here"

#endif

ConVar osu_folder_sub_songs("osu_folder_sub_songs", "Songs/", FCVAR_NONE);
ConVar osu_folder_sub_skins("osu_folder_sub_skins", "Skins/", FCVAR_NONE);

ConVar osu_database_enabled("osu_database_enabled", true, FCVAR_NONE);
ConVar osu_database_version("osu_database_version", 20191114, FCVAR_NONE, "maximum supported osu!.db version, above this will use fallback loader");
ConVar osu_database_ignore_version_warnings("osu_database_ignore_version_warnings", false, FCVAR_NONE);
ConVar osu_database_ignore_version("osu_database_ignore_version", false, FCVAR_NONE, "ignore upper version limit and force load the db file (may crash)");
ConVar osu_database_stars_cache_enabled("osu_database_stars_cache_enabled", false, FCVAR_NONE);
ConVar osu_scores_enabled("osu_scores_enabled", true, FCVAR_NONE);
ConVar osu_scores_legacy_enabled("osu_scores_legacy_enabled", true, FCVAR_NONE, "load osu!'s scores.db");
ConVar osu_scores_custom_enabled("osu_scores_custom_enabled", true, FCVAR_NONE, "load custom scores.db");
ConVar osu_scores_custom_version("osu_scores_custom_version", 20210110, FCVAR_NONE, "maximum supported custom scores.db/scoresvr.db version");
ConVar osu_scores_save_immediately("osu_scores_save_immediately", true, FCVAR_NONE, "write scores.db as soon as a new score is added");
ConVar osu_scores_sort_by_pp("osu_scores_sort_by_pp", true, FCVAR_NONE, "display pp in score browser instead of score");
ConVar osu_scores_bonus_pp("osu_scores_bonus_pp", true, FCVAR_NONE, "whether to add bonus pp to total (real) pp or not");
ConVar osu_scores_rename("osu_scores_rename");
ConVar osu_scores_export("osu_scores_export");
ConVar osu_collections_legacy_enabled("osu_collections_legacy_enabled", true, FCVAR_NONE, "load osu!'s collection.db");
ConVar osu_collections_custom_enabled("osu_collections_custom_enabled", true, FCVAR_NONE, "load custom collections.db");
ConVar osu_collections_custom_version("osu_collections_custom_version", 20220110, FCVAR_NONE, "maximum supported custom collections.db version");
ConVar osu_collections_save_immediately("osu_collections_save_immediately", true, FCVAR_NONE, "write collections.db as soon as anything is changed");
ConVar osu_user_beatmap_pp_sanity_limit_for_stats("osu_user_beatmap_pp_sanity_limit_for_stats", 10000.0f, FCVAR_NONE, "ignore scores with a higher pp value than this for the total profile pp calc");
ConVar osu_user_include_relax_and_autopilot_for_stats("osu_user_include_relax_and_autopilot_for_stats", false, FCVAR_NONE);
ConVar osu_user_switcher_include_legacy_scores_for_names("osu_user_switcher_include_legacy_scores_for_names", true, FCVAR_NONE);



struct SortScoreByScore : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByScore() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: score
		unsigned long long score1 = a.score;
		unsigned long long score2 = b.score;

		// second: time
		if (score1 == score2)
		{
			score1 = a.unixTimestamp;
			score2 = b.unixTimestamp;
		}

		// strict weak ordering!
		if (score1 == score2)
			return a.sortHack > b.sortHack;

		return score1 > score2;
	}
};

struct SortScoreByCombo : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByCombo() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: combo
		unsigned long long score1 = a.comboMax;
		unsigned long long score2 = b.comboMax;

		// second: score
		if (score1 == score2)
		{
			score1 = a.score;
			score2 = b.score;
		}

		// third: time
		if (score1 == score2)
		{
			score1 = a.unixTimestamp;
			score2 = b.unixTimestamp;
		}

		// strict weak ordering!
		if (score1 == score2)
			return a.sortHack > b.sortHack;

		return score1 > score2;
	}
};

struct SortScoreByDate : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByDate() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: time
		unsigned long long score1 = a.unixTimestamp;
		unsigned long long score2 = b.unixTimestamp;

		// strict weak ordering!
		if (score1 == score2)
			return a.sortHack > b.sortHack;

		return score1 > score2;
	}
};

struct SortScoreByMisses : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByMisses() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: misses
		unsigned long long score1 = b.numMisses; // swapped (lower numMisses is better)
		unsigned long long score2 = a.numMisses;

		// second: score
		if (score1 == score2)
		{
			score1 = a.score;
			score2 = b.score;
		}

		// third: time
		if (score1 == score2)
		{
			score1 = a.unixTimestamp;
			score2 = b.unixTimestamp;
		}

		// strict weak ordering!
		if (score1 == score2)
			return a.sortHack > b.sortHack;

		return score1 > score2;
	}
};

struct SortScoreByAccuracy : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByAccuracy() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: accuracy
		unsigned long long score1 = (unsigned long long)(OsuScore::calculateAccuracy(a.num300s, a.num100s, a.num50s, a.numMisses) * 10000.0f);
		unsigned long long score2 = (unsigned long long)(OsuScore::calculateAccuracy(b.num300s, b.num100s, b.num50s, b.numMisses) * 10000.0f);

		// second: score
		if (score1 == score2)
		{
			score1 = a.score;
			score2 = b.score;
		}

		// third: time
		if (score1 == score2)
		{
			score1 = a.unixTimestamp;
			score2 = b.unixTimestamp;
		}

		// strict weak ordering!
		if (score1 == score2)
			return a.sortHack > b.sortHack;

		return score1 > score2;
	}
};

struct SortScoreByPP : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByPP() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: pp
		unsigned long long score1 = (unsigned long long)std::max((a.isLegacyScore ? -b.score : a.pp) * 100.0f, 0.0f);
		unsigned long long score2 = (unsigned long long)std::max((b.isLegacyScore ? -a.score : b.pp) * 100.0f, 0.0f);


		// second: score
		if (score1 == score2)
		{
			score1 = a.score;
			score2 = b.score;
		}

		// third: time
		if (score1 == score2)
		{
			score1 = a.unixTimestamp;
			score2 = b.unixTimestamp;
		}

		// strict weak ordering!
		if (score1 == score2)
		{
			return a.sortHack > b.sortHack;
		}

		return score1 > score2;
	}
};

struct SortScoreByUnstableRate : public OsuDatabase::SCORE_SORTING_COMPARATOR
{
	virtual ~SortScoreByUnstableRate() {;}
	bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
	{
		// first: UR (reversed, lower is better)
		unsigned long long ur1 = (unsigned long long)(std::abs(a.isLegacyScore ? -a.sortHack : a.unstableRate) * 100000.0f);
		unsigned long long ur2 = (unsigned long long)(std::abs(b.isLegacyScore ? -b.sortHack : b.unstableRate) * 100000.0f);

		// strict weak ordering!
		if (ur1 == ur2)
		{
			return a.sortHack > b.sortHack;
		}

		return -ur1 > -ur2;
	}
};



struct SortCollectionByName
{
	bool operator () (OsuDatabase::Collection const &a, OsuDatabase::Collection const &b)
	{
		// strict weak ordering!
		if (a.name == b.name)
			return &a < &b;

		return a.name.lessThanIgnoreCase(b.name);
	}
};



class OsuDatabaseLoader : public Resource
{
public:
	OsuDatabaseLoader(OsuDatabase *db) : Resource()
	{
		m_db = db;
		m_bNeedRawLoad = false;
		m_bNeedCleanup = false;

		m_bAsyncReady = false;
		m_bReady = false;
	};

protected:
	virtual void init()
	{
		// legacy loading, if db is not found or by convar
		if (m_bNeedRawLoad)
			m_db->scheduleLoadRaw();
		else
		{
			// delete all previously loaded beatmaps here
			if (m_bNeedCleanup)
			{
				m_bNeedCleanup = false;
				for (int i=0; i<m_toCleanup.size(); i++)
				{
					delete m_toCleanup[i];
				}
				m_toCleanup.clear();
			}
		}

		m_bReady = true;

		delete this; // commit sudoku
	}

	virtual void initAsync()
	{
		debugLog("OsuDatabaseLoader::initAsync()\n");

		// load scores
		m_db->loadScores();

		// load stars.cache
		m_db->loadStars();

		// check if osu database exists, load file completely
		UString osuDbFilePath = osu_folder.getString();
		osuDbFilePath.append("osu!.db");
		OsuFile db = OsuFile(osuDbFilePath);

		// load database
		if (db.isReady() && osu_database_enabled.getBool())
		{
			m_bNeedCleanup = true;
			m_toCleanup.swap(m_db->m_databaseBeatmaps);
			m_db->m_databaseBeatmaps.clear();

			m_db->m_fLoadingProgress = 0.25f;
			m_db->loadDB(&db, m_bNeedRawLoad);
		}
		else
			m_bNeedRawLoad = true;

		m_bAsyncReady = true;
	}

	virtual void destroy() {;}

private:
	OsuDatabase *m_db;
	bool m_bNeedRawLoad;

	bool m_bNeedCleanup;
	std::vector<OsuDatabaseBeatmap*> m_toCleanup;
};



ConVar *OsuDatabase::m_name_ref = NULL;
ConVar *OsuDatabase::m_osu_songbrowser_scores_sortingtype_ref = NULL;

OsuDatabase::OsuDatabase(Osu *osu)
{
	// convar refs
	if (m_name_ref == NULL)
		m_name_ref = convar->getConVarByName("name");
	if (m_osu_songbrowser_scores_sortingtype_ref == NULL)
		m_osu_songbrowser_scores_sortingtype_ref = convar->getConVarByName("osu_songbrowser_scores_sortingtype");

	osu_scores_rename.setCallback( fastdelegate::MakeDelegate(this, &OsuDatabase::onScoresRename) );
	osu_scores_export.setCallback( fastdelegate::MakeDelegate(this, &OsuDatabase::onScoresExport) );

	// vars
	m_importTimer = new Timer();
	m_bIsFirstLoad = true;
	m_bFoundChanges = true;

	m_iNumBeatmapsToLoad = 0;
	m_fLoadingProgress = 0.0f;
	m_bInterruptLoad = false;

	m_osu = osu;
	m_iVersion = 0;
	m_iFolderCount = 0;

	m_bDidCollectionsChangeForSave = false;

	m_bScoresLoaded = false;
	m_bDidScoresChangeForSave = false;
	m_bDidScoresChangeForStats = true;
	m_iSortHackCounter = 0;

	m_iCurRawBeatmapLoadIndex = 0;
	m_bRawBeatmapLoadScheduled = false;

	m_prevPlayerStats.pp = 0.0f;
	m_prevPlayerStats.accuracy = 0.0f;
	m_prevPlayerStats.numScoresWithPP = 0;
	m_prevPlayerStats.level = 0;
	m_prevPlayerStats.percentToNextLevel = 0.0f;
	m_prevPlayerStats.totalScore = 0;

	m_scoreSortingMethods.push_back({"Sort By Accuracy", new SortScoreByAccuracy()});
	m_scoreSortingMethods.push_back({"Sort By Combo", new SortScoreByCombo()});
	m_scoreSortingMethods.push_back({"Sort By Date", new SortScoreByDate()});
	m_scoreSortingMethods.push_back({"Sort By Misses", new SortScoreByMisses()});
	m_scoreSortingMethods.push_back({"Sort By pp (Mc)", new SortScoreByPP()});
	m_scoreSortingMethods.push_back({"Sort By Score", new SortScoreByScore()});
	m_scoreSortingMethods.push_back({"Sort By Unstable Rate (Mc)", new SortScoreByUnstableRate()});
}

OsuDatabase::~OsuDatabase()
{
	SAFE_DELETE(m_importTimer);

	for (int i=0; i<m_databaseBeatmaps.size(); i++)
	{
		delete m_databaseBeatmaps[i];
	}

	for (int i=0; i<m_scoreSortingMethods.size(); i++)
	{
		delete m_scoreSortingMethods[i].comparator;
	}
}

void OsuDatabase::update()
{
	// loadRaw() logic
	if (m_bRawBeatmapLoadScheduled)
	{
		Timer t;
		t.start();

		while (t.getElapsedTime() < 0.033f)
		{
			if (m_bInterruptLoad.load()) break; // cancellation point

			if (m_rawLoadBeatmapFolders.size() > 0 && m_iCurRawBeatmapLoadIndex < m_rawLoadBeatmapFolders.size())
			{
				UString curBeatmap = m_rawLoadBeatmapFolders[m_iCurRawBeatmapLoadIndex++];
				m_rawBeatmapFolders.push_back(curBeatmap); // for future incremental loads, so that we know what's been loaded already

				UString fullBeatmapPath = m_sRawBeatmapLoadOsuSongFolder;
				fullBeatmapPath.append(curBeatmap);
				fullBeatmapPath.append("/");

				addBeatmap(fullBeatmapPath);
			}

			// update progress
			m_fLoadingProgress = (float)m_iCurRawBeatmapLoadIndex / (float)m_iNumBeatmapsToLoad;

			// check if we are finished
			if (m_iCurRawBeatmapLoadIndex >= m_iNumBeatmapsToLoad || m_iCurRawBeatmapLoadIndex > (int)(m_rawLoadBeatmapFolders.size()-1))
			{
				m_rawLoadBeatmapFolders.clear();
				m_bRawBeatmapLoadScheduled = false;
				m_importTimer->update();

				debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_databaseBeatmaps.size(), m_importTimer->getElapsedTime());

				// TODO: improve loading progress feedback here, currently we just freeze everything if this takes too long
				// load custom collections after we have all beatmaps available (and m_rawHashToDiff2 + m_rawHashToBeatmap populated)
				{
					loadCollections("collections.db", false, m_rawHashToDiff2, m_rawHashToBeatmap);

					std::sort(m_collections.begin(), m_collections.end(), SortCollectionByName());
				}

				m_fLoadingProgress = 1.0f;

				break;
			}

			t.update();
		}
	}
}

void OsuDatabase::load()
{
	m_bDidCollectionsChangeForSave = false;

	m_bInterruptLoad = false;
	m_fLoadingProgress = 0.0f;

	OsuDatabaseLoader *loader = new OsuDatabaseLoader(this); // (deletes itself after finishing)

	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->loadResource(loader);
}

void OsuDatabase::cancel()
{
	m_bInterruptLoad = true;
	m_bRawBeatmapLoadScheduled = false;
	m_fLoadingProgress = 1.0f; // force finished
	m_bFoundChanges = true;
}

void OsuDatabase::save()
{
	saveScores();
	saveCollections();
	saveStars();
}

OsuDatabaseBeatmap *OsuDatabase::addBeatmap(UString beatmapFolderPath)
{
	OsuDatabaseBeatmap *beatmap = loadRawBeatmap(beatmapFolderPath);

	if (beatmap != NULL)
		m_databaseBeatmaps.push_back(beatmap);

	return beatmap;
}

int OsuDatabase::addScore(std::string beatmapMD5Hash, OsuDatabase::Score score)
{
	if (beatmapMD5Hash.length() != 32)
	{
		debugLog("ERROR: OsuDatabase::addScore() has invalid md5hash.length() = %i!\n", beatmapMD5Hash.length());
		return -1;
	}

	addScoreRaw(beatmapMD5Hash, score);
	sortScores(beatmapMD5Hash);

	m_bDidScoresChangeForSave = true;
	m_bDidScoresChangeForStats = true;

	if (osu_scores_save_immediately.getBool())
		saveScores();

	// return sorted index
	for (int i=0; i<m_scores[beatmapMD5Hash].size(); i++)
	{
		if (m_scores[beatmapMD5Hash][i].unixTimestamp == score.unixTimestamp)
			return i;
	}

	return -1;
}

void OsuDatabase::addScoreRaw(const std::string &beatmapMD5Hash, const OsuDatabase::Score &score)
{
	m_scores[beatmapMD5Hash].push_back(score);

	// cheap dynamic recalculations for mcosu scores
	if (!score.isLegacyScore)
	{
		// as soon as we have >= 1 score with maxPossibleCombo info, all scores of that beatmap (even older ones without the info) can get the 'perfect' flag set
		// all scores >= 20180722 already have this populated during load, so this only affects the brief period where pp was stored without maxPossibleCombo info
		{
			// find score with maxPossibleCombo info
			int maxPossibleCombo = -1;
			for (const OsuDatabase::Score &s : m_scores[beatmapMD5Hash])
			{
				if (s.version > 20180722)
				{
					if (s.maxPossibleCombo > 0)
					{
						maxPossibleCombo = s.maxPossibleCombo;
						break;
					}
				}
			}

			// set 'perfect' flag on all relevant old scores of that same beatmap
			if (maxPossibleCombo > 0)
			{
				for (OsuDatabase::Score &s : m_scores[beatmapMD5Hash])
				{
					if (s.version <= 20180722 || s.maxPossibleCombo < 1) // also set on scores which have broken maxPossibleCombo values for whatever reason
						s.perfect = (s.comboMax > 0 && s.comboMax >= maxPossibleCombo);
				}
			}
		}
	}
}

void OsuDatabase::deleteScore(std::string beatmapMD5Hash, uint64_t scoreUnixTimestamp)
{
	if (beatmapMD5Hash.length() != 32)
	{
		debugLog("WARNING: OsuDatabase::deleteScore() called with invalid md5hash.length() = %i\n", beatmapMD5Hash.length());
		return;
	}

	for (int i=0; i<m_scores[beatmapMD5Hash].size(); i++)
	{
		if (m_scores[beatmapMD5Hash][i].unixTimestamp == scoreUnixTimestamp)
		{
			m_scores[beatmapMD5Hash].erase(m_scores[beatmapMD5Hash].begin() + i);

			m_bDidScoresChangeForSave = true;
			m_bDidScoresChangeForStats = true;

			//debugLog("Deleted score for %s at %llu\n", beatmapMD5Hash.c_str(), scoreUnixTimestamp);

			break;
		}
	}
}

void OsuDatabase::sortScores(std::string beatmapMD5Hash)
{
	if (beatmapMD5Hash.length() != 32 || m_scores[beatmapMD5Hash].size() < 2) return;

	for (int i=0; i<m_scoreSortingMethods.size(); i++)
	{
		if (m_osu_songbrowser_scores_sortingtype_ref->getString() == m_scoreSortingMethods[i].name)
		{
			struct COMPARATOR_WRAPPER
			{
				SCORE_SORTING_COMPARATOR *comp;
				bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const
				{
					return comp->operator()(a, b);
				}
			};
			COMPARATOR_WRAPPER comparatorWrapper;
			comparatorWrapper.comp = m_scoreSortingMethods[i].comparator;

			std::sort(m_scores[beatmapMD5Hash].begin(), m_scores[beatmapMD5Hash].end(), comparatorWrapper);
			return;
		}
	}

	debugLog("ERROR: Invalid score sortingtype \"%s\"\n", m_osu_songbrowser_scores_sortingtype_ref->getString().toUtf8());
}

bool OsuDatabase::addCollection(UString collectionName)
{
	if (collectionName.length() < 1) return false;

	// don't want duplicates
	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == collectionName)
			return false;
	}

	Collection c;
	{
		c.isLegacyCollection = false;

		c.name = collectionName;
	}
	m_collections.push_back(c);

	std::sort(m_collections.begin(), m_collections.end(), SortCollectionByName());

	m_bDidCollectionsChangeForSave = true;

	if (osu_collections_save_immediately.getBool())
		saveCollections();

	return true;
}

bool OsuDatabase::renameCollection(UString oldCollectionName, UString newCollectionName)
{
	if (newCollectionName.length() < 1) return false;
	if (oldCollectionName == newCollectionName) return false;

	// don't want duplicates
	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == newCollectionName)
			return false;
	}

	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == oldCollectionName)
		{
			// can't rename loaded osu! collections
			if (!m_collections[i].isLegacyCollection)
			{
				m_collections[i].name = newCollectionName;

				std::sort(m_collections.begin(), m_collections.end(), SortCollectionByName());

				m_bDidCollectionsChangeForSave = true;

				if (osu_collections_save_immediately.getBool())
					saveCollections();

				return true;
			}
			else
				return false;
		}
	}

	return false;
}

void OsuDatabase::deleteCollection(UString collectionName)
{
	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == collectionName)
		{
			// can't delete loaded osu! collections
			if (!m_collections[i].isLegacyCollection)
			{
				m_collections.erase(m_collections.begin() + i);

				m_bDidCollectionsChangeForSave = true;

				if (osu_collections_save_immediately.getBool())
					saveCollections();
			}

			break;
		}
	}
}

void OsuDatabase::addBeatmapToCollection(UString collectionName, std::string beatmapMD5Hash, bool doSaveImmediatelyIfEnabled)
{
	if (beatmapMD5Hash.length() != 32) return;

	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == collectionName)
		{
			bool containedAlready = false;
			for (size_t h=0; h<m_collections[i].hashes.size(); h++)
			{
				if (m_collections[i].hashes[h].hash == beatmapMD5Hash)
				{
					containedAlready = true;
					break;
				}
			}

			if (!containedAlready)
			{
				CollectionEntry entry;
				{
					entry.isLegacyEntry = false;

					entry.hash = beatmapMD5Hash;
				}
				m_collections[i].hashes.push_back(entry);

				m_bDidCollectionsChangeForSave = true;

				if (doSaveImmediatelyIfEnabled && osu_collections_save_immediately.getBool())
					saveCollections();

				// also update .beatmaps for convenience (songbrowser will use that to rebuild the UI)
				{
					OsuDatabaseBeatmap *beatmap = getBeatmap(beatmapMD5Hash);
					OsuDatabaseBeatmap *diff2 = getBeatmapDifficulty(beatmapMD5Hash);

					if (beatmap != NULL && diff2 != NULL)
					{
						bool beatmapContainedAlready = false;
						for (size_t b=0; b<m_collections[i].beatmaps.size(); b++)
						{
							if (m_collections[i].beatmaps[b].first == beatmap)
							{
								beatmapContainedAlready = true;

								bool diffContainedAlready = false;
								for (size_t d=0; d<m_collections[i].beatmaps[b].second.size(); d++)
								{
									if (m_collections[i].beatmaps[b].second[d] == diff2)
									{
										diffContainedAlready = true;
										break;
									}
								}

								if (!diffContainedAlready)
									m_collections[i].beatmaps[b].second.push_back(diff2);

								break;
							}
						}

						if (!beatmapContainedAlready)
						{
							std::vector<OsuDatabaseBeatmap*> diffs2;
							{
								diffs2.push_back(diff2);
							}
							m_collections[i].beatmaps.push_back(std::pair<OsuDatabaseBeatmap*, std::vector<OsuDatabaseBeatmap*>>(beatmap, diffs2));
						}
					}
				}
			}

			break;
		}
	}
}

void OsuDatabase::removeBeatmapFromCollection(UString collectionName, std::string beatmapMD5Hash, bool doSaveImmediatelyIfEnabled)
{
	if (beatmapMD5Hash.length() != 32) return;

	for (size_t i=0; i<m_collections.size(); i++)
	{
		if (m_collections[i].name == collectionName)
		{
			bool didRemove = false;
			for (size_t h=0; h<m_collections[i].hashes.size(); h++)
			{
				if (m_collections[i].hashes[h].hash == beatmapMD5Hash)
				{
					// can't delete loaded osu! collection entries
					if (!m_collections[i].hashes[h].isLegacyEntry)
					{
						m_collections[i].hashes.erase(m_collections[i].hashes.begin() + h);

						didRemove = true;

						m_bDidCollectionsChangeForSave = true;

						if (doSaveImmediatelyIfEnabled && osu_collections_save_immediately.getBool())
							saveCollections();
					}

					break;
				}
			}

			// also update .beatmaps for convenience (songbrowser will use that to rebuild the UI)
			if (didRemove)
			{
				for (size_t b=0; b<m_collections[i].beatmaps.size(); b++)
				{
					bool found = false;
					for (size_t d=0; d<m_collections[i].beatmaps[b].second.size(); d++)
					{
						if (m_collections[i].beatmaps[b].second[d]->getMD5Hash() == beatmapMD5Hash)
						{
							found = true;

							m_collections[i].beatmaps[b].second.erase(m_collections[i].beatmaps[b].second.begin() + d);

							break;
						}
					}

					if (found)
						break;
				}
			}
		}
	}
}

std::vector<UString> OsuDatabase::getPlayerNamesWithPPScores()
{
	std::vector<std::string> keys;
	keys.reserve(m_scores.size());

	for (auto kv : m_scores)
	{
		keys.push_back(kv.first);
	}

	// bit of a useless double string conversion going on here, but whatever

	std::unordered_set<std::string> tempNames;
	for (auto &key : keys)
	{
		for (Score &score : m_scores[key])
		{
			if (!score.isLegacyScore)
				tempNames.insert(std::string(score.playerName.toUtf8()));
		}
	}

	// always add local user, even if there were no scores
	tempNames.insert(std::string(m_name_ref->getString().toUtf8()));

	std::vector<UString> names;
	names.reserve(tempNames.size());
	for (auto k : tempNames)
	{
		if (k.length() > 0)
			names.push_back(UString(k.c_str()));
	}

	return names;
}

std::vector<UString> OsuDatabase::getPlayerNamesWithScoresForUserSwitcher()
{
	const bool includeLegacyNames = osu_user_switcher_include_legacy_scores_for_names.getBool();

	// bit of a useless double string conversion going on here, but whatever

	std::unordered_set<std::string> tempNames;
	for (auto kv : m_scores)
	{
		const std::string &key = kv.first;
		for (Score &score : m_scores[key])
		{
			if (!score.isLegacyScore || includeLegacyNames)
				tempNames.insert(std::string(score.playerName.toUtf8()));
		}
	}

	// always add local user, even if there were no scores
	tempNames.insert(std::string(m_name_ref->getString().toUtf8()));

	std::vector<std::string> stdNames;
	stdNames.reserve(tempNames.size());
	for (auto k : tempNames)
	{
		if (k.length() > 0)
			stdNames.push_back(k);
	}
	struct CaseInsensitiveStringComparator
	{
		bool operator() (const std::string &a, const std::string &b) const noexcept
		{
			return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [] (unsigned char c1, unsigned char c2) {
                	return std::tolower(c1) < std::tolower(c2);
				}
			);
		}
	};
	std::sort(stdNames.begin(), stdNames.end(), CaseInsensitiveStringComparator());

	std::vector<UString> names;
	names.reserve(stdNames.size());
	for (const std::string &stdName : stdNames)
	{
		names.push_back(UString(stdName.c_str(), stdName.size()));
	}
	return names;
}

OsuDatabase::PlayerPPScores OsuDatabase::getPlayerPPScores(UString playerName)
{
	std::vector<Score*> scores;

	// collect all scores with pp data
	std::vector<std::string> keys;
	keys.reserve(m_scores.size());

	for (auto kv : m_scores)
	{
		keys.push_back(kv.first);
	}

	struct ScoreSortComparator
	{
	    bool operator() (Score const *a, Score const *b) const
	    {
	    	// sort by pp
	    	// strict weak ordering!
	    	if (a->pp == b->pp)
	    		return a->sortHack < b->sortHack;
	    	else
	    		return a->pp < b->pp;
	    }
	};

	unsigned long long totalScore = 0;
	const float userBeatmapPpSanityLimitForStats = osu_user_beatmap_pp_sanity_limit_for_stats.getFloat();
	for (auto &key : keys)
	{
		if (m_scores[key].size() > 0)
		{
			Score *tempScore = &m_scores[key][0];

			// only add highest pp score per diff
			bool foundValidScore = false;
			float prevPP = -1.0f;
			for (Score &score : m_scores[key])
			{
				if (!score.isLegacyScore && (osu_user_include_relax_and_autopilot_for_stats.getBool() ? true : !((score.modsLegacy & OsuReplay::Mods::Relax) || (score.modsLegacy & OsuReplay::Mods::Relax2))) && score.playerName == playerName)
				{
					const bool isSaneScore = (userBeatmapPpSanityLimitForStats <= 0.0f || score.pp <= userBeatmapPpSanityLimitForStats);
					if (isSaneScore)
					{
						foundValidScore = true;

						totalScore += score.score;

						score.sortHack = m_iSortHackCounter++;

						if ((score.pp > prevPP || prevPP < 0.0f))
						{
							prevPP = score.pp;
							tempScore = &score;
						}
					}
				}
			}

			if (foundValidScore)
				scores.push_back(tempScore);
		}
	}

	// sort by pp
	std::sort(scores.begin(), scores.end(), ScoreSortComparator());

	PlayerPPScores ppScores;
	ppScores.ppScores = std::move(scores);
	ppScores.totalScore = totalScore;

	return ppScores;
}

OsuDatabase::PlayerStats OsuDatabase::calculatePlayerStats(UString playerName)
{
	if (!m_bDidScoresChangeForStats && playerName == m_prevPlayerStats.name) return m_prevPlayerStats;

	const PlayerPPScores ps = getPlayerPPScores(playerName);

	// delay caching until we actually have scores loaded
	if (ps.ppScores.size() > 0)
		m_bDidScoresChangeForStats = false;

	// "If n is the amount of scores giving more pp than a given score, then the score's weight is 0.95^n"
	// "Total pp = PP[1] * 0.95^0 + PP[2] * 0.95^1 + PP[3] * 0.95^2 + ... + PP[n] * 0.95^(n-1)"
	// also, total accuracy is apparently weighted the same as pp

	// https://expectancyviolation.github.io/osu-acc/

	float pp = 0.0f;
	float acc = 0.0f;
	for (size_t i=0; i<ps.ppScores.size(); i++)
	{
		const float weight = getWeightForIndex(ps.ppScores.size() - 1 - i);

		pp += ps.ppScores[i]->pp * weight;
		acc += OsuScore::calculateAccuracy(ps.ppScores[i]->num300s, ps.ppScores[i]->num100s, ps.ppScores[i]->num50s, ps.ppScores[i]->numMisses) * weight;
	}

	// bonus pp
	// https://osu.ppy.sh/wiki/en/Performance_points
	if (osu_scores_bonus_pp.getBool())
		pp += getBonusPPForNumScores(ps.ppScores.size());

	// normalize accuracy
	if (ps.ppScores.size() > 0)
		acc /= (20.0f * (1.0f - getWeightForIndex(ps.ppScores.size())));

	// fill stats
	m_prevPlayerStats.name = playerName;
	m_prevPlayerStats.pp = pp;
	m_prevPlayerStats.accuracy = acc;
	m_prevPlayerStats.numScoresWithPP = ps.ppScores.size();

	if (ps.totalScore != m_prevPlayerStats.totalScore)
	{
		m_prevPlayerStats.level = getLevelForScore(ps.totalScore);

		const unsigned long long requiredScoreForCurrentLevel = getRequiredScoreForLevel(m_prevPlayerStats.level);
		const unsigned long long requiredScoreForNextLevel = getRequiredScoreForLevel(m_prevPlayerStats.level + 1);

		if (requiredScoreForNextLevel > requiredScoreForCurrentLevel)
			m_prevPlayerStats.percentToNextLevel = (double)(ps.totalScore - requiredScoreForCurrentLevel) / (double)(requiredScoreForNextLevel - requiredScoreForCurrentLevel);
	}

	m_prevPlayerStats.totalScore = ps.totalScore;

	return m_prevPlayerStats;
}

float OsuDatabase::getWeightForIndex(int i)
{
	return std::pow(0.95, (double)i);
}

float OsuDatabase::getBonusPPForNumScores(size_t numScores)
{
	// see https://github.com/ppy/osu-queue-score-statistics/blob/master/osu.Server.Queues.ScoreStatisticsProcessor/Helpers/UserTotalPerformanceAggregateHelper.cs
	
	// old
	//return (416.6667 * (1.0 - std::pow(0.9994, (double)numScores)));
	
	// new
	return (417.0 - 1.0 / 3.0) * (1.0 - std::pow(0.995, std::min(1000.0, (double)numScores)));
}

unsigned long long OsuDatabase::getRequiredScoreForLevel(int level)
{
	// https://zxq.co/ripple/ocl/src/branch/master/level.go
	if (level <= 100)
	{
		if (level > 1)
			return (uint64_t)std::floor( 5000/3*(4 * std::pow(level, 3) - 3 * std::pow(level, 2) - level) + std::floor(1.25 * std::pow(1.8, (double)(level - 60))) );

		return 1;
	}

	return (uint64_t)26931190829 + (uint64_t)100000000000 * (uint64_t)(level - 100);
}

int OsuDatabase::getLevelForScore(unsigned long long score, int maxLevel)
{
	// https://zxq.co/ripple/ocl/src/branch/master/level.go
	int i = 0;
	while (true)
	{
		if (maxLevel > 0 && i >= maxLevel)
			return i;

		const unsigned long long lScore = getRequiredScoreForLevel(i);

		if (score < lScore)
			return (i - 1);

		i++;
	}
}

OsuDatabaseBeatmap *OsuDatabase::getBeatmap(const std::string &md5hash)
{
	const size_t md5hashLength = md5hash.length();

	if (md5hashLength != 32) return NULL;

	for (size_t i=0; i<m_databaseBeatmaps.size(); i++)
	{
		OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
		const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
		for (size_t d=0; d<diffs.size(); d++)
		{
			const OsuDatabaseBeatmap *diff = diffs[d];

			const size_t diffmd5hashLength = diff->getMD5Hash().length();
			bool uuidMatches = (diffmd5hashLength > 0 && diffmd5hashLength == md5hashLength);
			for (size_t u=0; u<32 && u<diffmd5hashLength && u<md5hashLength; u++)
			{
				if (diff->getMD5Hash()[u] != md5hash[u])
				{
					uuidMatches = false;
					break;
				}
			}

			if (uuidMatches)
				return beatmap;
		}
	}

	return NULL;
}

OsuDatabaseBeatmap *OsuDatabase::getBeatmapDifficulty(const std::string &md5hash)
{
	const size_t md5hashLength = md5hash.length();

	if (md5hashLength != 32) return NULL;

	// TODO: optimize db accesses by caching a hashmap from md5hash -> OsuBeatmap*, currently it just does a loop over all diffs of all beatmaps (for every call)
	for (size_t i=0; i<m_databaseBeatmaps.size(); i++)
	{
		OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
		const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
		for (size_t d=0; d<diffs.size(); d++)
		{
			OsuDatabaseBeatmap *diff = diffs[d];

			const size_t diffmd5hashLength = diff->getMD5Hash().length();
			bool uuidMatches = (diffmd5hashLength > 0);
			for (size_t u=0; u<32 && u<diffmd5hashLength && u<md5hashLength; u++)
			{
				if (diff->getMD5Hash()[u] != md5hash[u])
				{
					uuidMatches = false;
					break;
				}
			}

			if (uuidMatches)
				return diff;
		}
	}

	return NULL;
}

UString OsuDatabase::parseLegacyCfgBeatmapDirectoryParameter()
{
	// get BeatmapDirectory parameter from osu!.<OS_USERNAME>.cfg
	debugLog("OsuDatabase::parseLegacyCfgBeatmapDirectoryParameter() : username = %s\n", env->getUsername().toUtf8());
	if (env->getUsername().length() > 0)
	{
		UString osuUserConfigFilePath = osu_folder.getString();
		osuUserConfigFilePath.append("osu!.");
		osuUserConfigFilePath.append(env->getUsername());
		osuUserConfigFilePath.append(".cfg");

		File file(osuUserConfigFilePath);
		char stringBuffer[1024];
		while (file.canRead())
		{
			UString uCurLine = file.readLine();
			const char *curLineChar = uCurLine.toUtf8();
			std::string curLine(curLineChar);

			memset(stringBuffer, '\0', 1024);
			if (sscanf(curLineChar, " BeatmapDirectory = %1023[^\n]", stringBuffer) == 1)
			{
				UString beatmapDirectory = UString(stringBuffer);
				beatmapDirectory = beatmapDirectory.trim();

				if (beatmapDirectory.length() > 2)
				{
					// if we have an absolute path, use it in its entirety.
					// otherwise, append the beatmapDirectory to the songFolder (which uses the osu_folder as the starting point)
					UString songsFolder = osu_folder.getString();

					if (beatmapDirectory.find(":") != -1)
						songsFolder = beatmapDirectory;
					else
					{
						// ensure that beatmapDirectory doesn't start with a slash
						if (beatmapDirectory[0] == L'/' || beatmapDirectory[0] == L'\\')
							beatmapDirectory.erase(0, 1);

						songsFolder.append(beatmapDirectory);
					}

					// ensure that the songFolder ends with a slash
					if (songsFolder.length() > 0)
					{
						if (songsFolder[songsFolder.length()-1] != L'/' && songsFolder[songsFolder.length()-1] != L'\\')
							songsFolder.append("/");
					}

					return songsFolder;
				}

				break;
			}
		}
	}

	return "";
}

void OsuDatabase::scheduleLoadRaw()
{
	m_sRawBeatmapLoadOsuSongFolder = osu_folder.getString();
	{
		const UString customBeatmapDirectory = parseLegacyCfgBeatmapDirectoryParameter();
		if (customBeatmapDirectory.length() < 1)
			m_sRawBeatmapLoadOsuSongFolder.append(osu_folder_sub_songs.getString());
		else
			m_sRawBeatmapLoadOsuSongFolder = customBeatmapDirectory;
	}

	debugLog("Database: m_sRawBeatmapLoadOsuSongFolder = %s\n", m_sRawBeatmapLoadOsuSongFolder.toUtf8());

	m_rawLoadBeatmapFolders = env->getFoldersInFolder(m_sRawBeatmapLoadOsuSongFolder);
	m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

	// if this isn't the first load, only load the differences
	if (!m_bIsFirstLoad)
	{
		std::vector<UString> toLoad;
		for (int i=0; i<m_iNumBeatmapsToLoad; i++)
		{
			bool alreadyLoaded = false;
			for (int j=0; j<m_rawBeatmapFolders.size(); j++)
			{
				if (m_rawLoadBeatmapFolders[i] == m_rawBeatmapFolders[j])
				{
					alreadyLoaded = true;
					break;
				}
			}

			if (!alreadyLoaded)
				toLoad.push_back(m_rawLoadBeatmapFolders[i]);
		}

		// only load differences
		m_rawLoadBeatmapFolders = toLoad;
		m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

		debugLog("Database: Found %i new/changed beatmaps.\n", m_iNumBeatmapsToLoad);

		m_bFoundChanges = m_iNumBeatmapsToLoad > 0;
		if (m_bFoundChanges)
			m_osu->getNotificationOverlay()->addNotification(UString::format(m_iNumBeatmapsToLoad == 1 ? "Adding %i new beatmap." : "Adding %i new beatmaps.", m_iNumBeatmapsToLoad), 0xff00ff00);
		else
			m_osu->getNotificationOverlay()->addNotification(UString::format("No new beatmaps detected.", m_iNumBeatmapsToLoad), 0xff00ff00);
	}

	debugLog("Database: Building beatmap database ...\n");
	debugLog("Database: Found %i folders to load.\n", m_rawLoadBeatmapFolders.size());

	// only start loading if we have something to load
	if (m_rawLoadBeatmapFolders.size() > 0)
	{
		m_fLoadingProgress = 0.0f;
		m_iCurRawBeatmapLoadIndex = 0;

		m_bRawBeatmapLoadScheduled = true;
		m_importTimer->start();

		if (m_bIsFirstLoad)
		{
			// reset
			m_rawHashToDiff2.clear();
			m_rawHashToBeatmap.clear();
		}
	}
	else
		m_fLoadingProgress = 1.0f;

	m_bIsFirstLoad = false;
}

void OsuDatabase::loadDB(OsuFile *db, bool &fallbackToRawLoad)
{
	// reset
	m_collections.clear();

	if (m_databaseBeatmaps.size() > 0)
		debugLog("WARNING: OsuDatabase::loadDB() called without cleared m_beatmaps!!!\n");

	m_databaseBeatmaps.clear();

	if (!db->isReady())
	{
		debugLog("Database: Couldn't read, database not ready!\n");
		return;
	}

	// get BeatmapDirectory parameter from osu!.<OS_USERNAME>.cfg
	// fallback to /Songs/ if it doesn't exist
	UString songFolder = osu_folder.getString();
	{
		const UString customBeatmapDirectory = parseLegacyCfgBeatmapDirectoryParameter();
		if (customBeatmapDirectory.length() < 1)
			songFolder.append(osu_folder_sub_songs.getString());
		else
			songFolder = customBeatmapDirectory;
	}

	debugLog("Database: songFolder = %s\n", songFolder.toUtf8());

	m_importTimer->start();

	// read header
	m_iVersion = db->readInt();
	m_iFolderCount = db->readInt();
	db->readBool();
	db->readDateTime();
	UString playerName = db->readString();
	m_iNumBeatmapsToLoad = db->readInt();

	debugLog("Database: version = %i, folderCount = %i, playerName = %s, numDiffs = %i\n", m_iVersion, m_iFolderCount, playerName.toUtf8(), m_iNumBeatmapsToLoad);

	if (m_iVersion < 20140609)
	{
		debugLog("Database: Version is below 20140609, not supported.\n");
		m_osu->getNotificationOverlay()->addNotification("osu!.db version too old, update osu! and try again!", 0xffff0000);
		m_fLoadingProgress = 1.0f;
		return;
	}

	if (m_iVersion < 20170222)
	{
		debugLog("Database: Version is quite old, below 20170222 ...\n");
		m_osu->getNotificationOverlay()->addNotification("osu!.db version too old, update osu! and try again!", 0xffff0000);
		m_fLoadingProgress = 1.0f;
		return;
	}

	if (!osu_database_ignore_version_warnings.getBool())
	{
		if (m_iVersion < 20190207) // xexxar angles star recalc
		{
			m_osu->getNotificationOverlay()->addNotification("osu!.db version is old,  let osu! update when convenient.", 0xffffff00, false, 3.0f);
		}
	}

	// hard cap upper db version
	if (m_iVersion > osu_database_version.getInt() && !osu_database_ignore_version.getBool())
	{
		m_osu->getNotificationOverlay()->addNotification(UString::format("osu!.db version unknown (%i),  using fallback loader.", m_iVersion), 0xffffff00, false, 5.0f);

		fallbackToRawLoad = true;
		m_fLoadingProgress = 0.0f;

		return;
	}

	// read beatmapInfos, and also build two hashmaps (diff hash -> OsuBeatmapDifficulty, diff hash -> OsuBeatmap)
	struct BeatmapSet
	{
		int setID;
		UString path;
		std::vector<OsuDatabaseBeatmap*> diffs2;
	};
	std::vector<BeatmapSet> beatmapSets;
	std::unordered_map<int, size_t> setIDToIndex;
	std::unordered_map<std::string, OsuDatabaseBeatmap*> hashToDiff2;
	std::unordered_map<std::string, OsuDatabaseBeatmap*> hashToBeatmap;
	for (int i=0; i<m_iNumBeatmapsToLoad; i++)
	{
		if (m_bInterruptLoad.load()) break; // cancellation point

		if (Osu::debug->getBool())
			debugLog("Database: Reading beatmap %i/%i ...\n", (i+1), m_iNumBeatmapsToLoad);

		m_fLoadingProgress = 0.24f + 0.5f*((float)(i+1)/(float)m_iNumBeatmapsToLoad);

		if (m_iVersion < 20191107) // see https://osu.ppy.sh/home/changelog/stable40/20191107.2
		{
			// also see https://github.com/ppy/osu-wiki/commit/b90f312e06b4f86e509b397565f1fe014bb15943
			// no idea why peppy decided to change the wiki version from 20191107 to 20191106, because that's not what stable is doing.
			// the correct version is still 20191107

			/*unsigned int size = */db->readInt(); // size in bytes of the beatmap entry
		}

		UString artistName = db->readString().trim();
		UString artistNameUnicode = db->readString();
		UString songTitle = db->readString().trim();
		UString songTitleUnicode = db->readString();
		UString creatorName = db->readString().trim();
		UString difficultyName = db->readString().trim();
		UString audioFileName = db->readString();
		std::string md5hash = db->readStdString();
		UString osuFileName = db->readString();
		/*unsigned char rankedStatus = */db->readByte();
		unsigned short numCircles = db->readShort();
		unsigned short numSliders = db->readShort();
		unsigned short numSpinners = db->readShort();
		long long lastModificationTime = db->readLongLong();
		float AR = db->readFloat();
		float CS = db->readFloat();
		float HP = db->readFloat();
		float OD = db->readFloat();
		double sliderMultiplier = db->readDouble();

		//debugLog("Database: Entry #%i: artist = %s, songtitle = %s, creator = %s, diff = %s, audiofilename = %s, md5hash = %s, osufilename = %s\n", i, artistName.toUtf8(), songTitle.toUtf8(), creatorName.toUtf8(), difficultyName.toUtf8(), audioFileName.toUtf8(), md5hash.c_str(), osuFileName.toUtf8());
		//debugLog("rankedStatus = %i, numCircles = %i, numSliders = %i, numSpinners = %i, lastModificationTime = %ld\n", (int)rankedStatus, numCircles, numSliders, numSpinners, lastModificationTime);
		//debugLog("AR = %f, CS = %f, HP = %f, OD = %f, sliderMultiplier = %f\n", AR, CS, HP, OD, sliderMultiplier);

		unsigned int numOsuStandardStarRatings = db->readInt();
		//debugLog("%i star ratings for osu!standard\n", numOsuStandardStarRatings);
		float numOsuStandardStars = 0.0f;
		for (int s=0; s<numOsuStandardStarRatings; s++)
		{
			db->readByte(); // ObjType
			unsigned int mods = db->readInt();
			db->readByte(); // ObjType
			double starRating = (m_iVersion >= 20250108 ? (double)db->readFloat() : db->readDouble()); // see https://osu.ppy.sh/home/changelog/stable40/20250108.3
			//debugLog("%f stars for %u\n", starRating, mods);

			if (mods == 0)
				numOsuStandardStars = starRating;
		}
		// NOTE: if we have our own stars cached then prefer that
		{
			if (osu_database_stars_cache_enabled.getBool())
				numOsuStandardStars = 0.0f; // NOTE: force don't use stable stars

			const auto result = m_starsCache.find(md5hash);
			if (result != m_starsCache.end())
				numOsuStandardStars = result->second.starsNomod;
		}

		unsigned int numTaikoStarRatings = db->readInt();
		//debugLog("%i star ratings for taiko\n", numTaikoStarRatings);
		for (int s=0; s<numTaikoStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			if (m_iVersion >= 20250108) // see https://osu.ppy.sh/home/changelog/stable40/20250108.3
				db->readFloat();
			else
				db->readDouble();
		}

		unsigned int numCtbStarRatings = db->readInt();
		//debugLog("%i star ratings for ctb\n", numCtbStarRatings);
		for (int s=0; s<numCtbStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			if (m_iVersion >= 20250108) // see https://osu.ppy.sh/home/changelog/stable40/20250108.3
				db->readFloat();
			else
				db->readDouble();
		}

		unsigned int numManiaStarRatings = db->readInt();
		//debugLog("%i star ratings for mania\n", numManiaStarRatings);
		for (int s=0; s<numManiaStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			if (m_iVersion >= 20250108) // see https://osu.ppy.sh/home/changelog/stable40/20250108.3
				db->readFloat();
			else
				db->readDouble();
		}

		/*unsigned int drainTime = */db->readInt(); // seconds
		int duration = db->readInt(); // milliseconds
		duration = duration >= 0 ? duration : 0; // sanity clamp
		int previewTime = db->readInt();

		//debugLog("drainTime = %i sec, duration = %i ms, previewTime = %i ms\n", drainTime, duration, previewTime);

		unsigned int numTimingPoints = db->readInt();
		//debugLog("%i timingpoints\n", numTimingPoints);
		std::vector<OsuFile::TIMINGPOINT> timingPoints;
		for (int t=0; t<numTimingPoints; t++)
		{
			timingPoints.push_back(db->readTimingPoint());
		}

		int beatmapID = db->readInt(); // fucking bullshit, this is NOT an unsigned integer as is described on the wiki, it can and is -1 sometimes
		int beatmapSetID = db->readInt(); // same here
		/*unsigned int threadID = */db->readInt();

		/*unsigned char osuStandardGrade = */db->readByte();
		/*unsigned char taikoGrade = */db->readByte();
		/*unsigned char ctbGrade = */db->readByte();
		/*unsigned char maniaGrade = */db->readByte();
		//debugLog("beatmapID = %i, beatmapSetID = %i, threadID = %i, osuStandardGrade = %i, taikoGrade = %i, ctbGrade = %i, maniaGrade = %i\n", beatmapID, beatmapSetID, threadID, osuStandardGrade, taikoGrade, ctbGrade, maniaGrade);

		short localOffset = db->readShort();
		float stackLeniency = db->readFloat();
		unsigned char mode = db->readByte();
		//debugLog("localOffset = %i, stackLeniency = %f, mode = %i\n", localOffset, stackLeniency, mode);

		UString songSource = db->readString().trim();
		UString songTags = db->readString().trim();
		//debugLog("songSource = %s, songTags = %s\n", songSource.toUtf8(), songTags.toUtf8());

		short onlineOffset = db->readShort();
		UString songTitleFont = db->readString();
		/*bool unplayed = */db->readBool();
		/*long long lastTimePlayed = */db->readLongLong();
		/*bool isOsz2 = */db->readBool();
		UString path = db->readString().trim(); // somehow, some beatmaps may have spaces at the start/end of their path, breaking the Windows API (e.g. https://osu.ppy.sh/s/215347), therefore the trim
		/*long long lastOnlineCheck = */db->readLongLong();
		//debugLog("onlineOffset = %i, songTitleFont = %s, unplayed = %i, lastTimePlayed = %lu, isOsz2 = %i, path = %s, lastOnlineCheck = %lu\n", onlineOffset, songTitleFont.toUtf8(), (int)unplayed, lastTimePlayed, (int)isOsz2, path.toUtf8(), lastOnlineCheck);

		/*bool ignoreBeatmapSounds = */db->readBool();
		/*bool ignoreBeatmapSkin = */db->readBool();
		/*bool disableStoryboard = */db->readBool();
		/*bool disableVideo = */db->readBool();
		/*bool visualOverride = */db->readBool();
		/*int lastEditTime = */db->readInt();
		/*unsigned char maniaScrollSpeed = */db->readByte();
		//debugLog("ignoreBeatmapSounds = %i, ignoreBeatmapSkin = %i, disableStoryboard = %i, disableVideo = %i, visualOverride = %i, maniaScrollSpeed = %i\n", (int)ignoreBeatmapSounds, (int)ignoreBeatmapSkin, (int)disableStoryboard, (int)disableVideo, (int)visualOverride, maniaScrollSpeed);

		// HACKHACK: workaround for linux and macos: it can happen that nested beatmaps are stored in the database, and that osu! stores that filepath with a backslash (because windows)
		if (env->getOS() == Environment::OS::OS_LINUX || env->getOS() == Environment::OS::OS_MACOS)
		{
			for (int c=0; c<path.length(); c++)
			{
				if (path[c] == L'\\')
				{
					path.erase(c, 1);
					path.insert(c, L'/');
				}
			}
		}

		// build beatmap & diffs from all the data
		UString beatmapPath = songFolder;
		beatmapPath.append(path);
		beatmapPath.append("/");
		UString fullFilePath = beatmapPath;
		fullFilePath.append(osuFileName);

		// skip invalid/corrupt entries
		// the good way would be to check if the .osu file actually exists on disk, but that is slow af, ain't nobody got time for that
		// so, since I've seen some concrete examples of what happens in such cases, we just exclude those
		if (artistName.length() < 1 && songTitle.length() < 1 && creatorName.length() < 1 && difficultyName.length() < 1 && md5hash.length() < 1)
			continue;

		// fill diff with data
		if ((mode == 0 && m_osu->getGamemode() == Osu::GAMEMODE::STD) || (mode == 0x03 && m_osu->getGamemode() == Osu::GAMEMODE::MANIA)) // gamemode filter
		{
			OsuDatabaseBeatmap *diff2 = new OsuDatabaseBeatmap(m_osu, fullFilePath, beatmapPath);
			{
				diff2->m_sTitle = songTitle;
				diff2->m_sAudioFileName = audioFileName;
				diff2->m_iLengthMS = duration;

				diff2->m_fStackLeniency = stackLeniency;

				diff2->m_sArtist = artistName;
				diff2->m_sCreator = creatorName;
				diff2->m_sDifficultyName = difficultyName;
				diff2->m_sSource = songSource;
				diff2->m_sTags = songTags;
				diff2->m_sMD5Hash = md5hash;
				diff2->m_iID = beatmapID;
				diff2->m_iSetID = beatmapSetID;

				diff2->m_fAR = AR;
				diff2->m_fCS = CS;
				diff2->m_fHP = HP;
				diff2->m_fOD = OD;
				diff2->m_fSliderMultiplier = sliderMultiplier;

				//diff2->m_sBackgroundImageFileName = "";

				diff2->m_iPreviewTime = previewTime;
				diff2->m_iLastModificationTime = lastModificationTime;

				diff2->m_sFullSoundFilePath = beatmapPath;
				diff2->m_sFullSoundFilePath.append(diff2->m_sAudioFileName);
				diff2->m_iLocalOffset = localOffset;
				diff2->m_iOnlineOffset = (long)onlineOffset;
				diff2->m_iNumObjects = numCircles + numSliders + numSpinners;
				diff2->m_iNumCircles = numCircles;
				diff2->m_iNumSliders = numSliders;
				diff2->m_iNumSpinners = numSpinners;
				diff2->m_fStarsNomod = numOsuStandardStars;

				// calculate bpm range
				float minBeatLength = 0;
				float maxBeatLength = std::numeric_limits<float>::max();
				std::vector<OsuFile::TIMINGPOINT> uninheritedTimingpoints;
				for (int j=0; j<timingPoints.size(); j++)
				{
					const OsuFile::TIMINGPOINT &t = timingPoints[j];

					if (t.msPerBeat >= 0) // NOT inherited
					{
						uninheritedTimingpoints.push_back(t);

						if (t.msPerBeat > minBeatLength)
							minBeatLength = t.msPerBeat;
						if (t.msPerBeat < maxBeatLength)
							maxBeatLength = t.msPerBeat;
					}
				}

				// convert from msPerBeat to BPM
				const float msPerMinute = 1 * 60 * 1000;
				if (minBeatLength != 0)
					minBeatLength = msPerMinute / minBeatLength;
				if (maxBeatLength != 0)
					maxBeatLength = msPerMinute / maxBeatLength;

				diff2->m_iMinBPM = (int)std::round(minBeatLength);
				diff2->m_iMaxBPM = (int)std::round(maxBeatLength);

				struct MostCommonBPMHelper
				{
					static int calculateMostCommonBPM(const std::vector<OsuFile::TIMINGPOINT> &uninheritedTimingpoints, long lastTime)
					{
						if (uninheritedTimingpoints.size() < 1) return 0;

						struct Tuple
						{
							float beatLength;
							long duration;

							size_t sortHack;
						};

						// "Construct a set of (beatLength, duration) tuples for each individual timing point."
						std::vector<Tuple> tuples;
						tuples.reserve(uninheritedTimingpoints.size());
						for (size_t i=0; i<uninheritedTimingpoints.size(); i++)
						{
							const OsuFile::TIMINGPOINT &t = uninheritedTimingpoints[i];

							Tuple tuple;
							{
								if (t.offset > lastTime)
								{
									tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
									tuple.duration = 0;
								}
								else
								{
									// "osu-stable forced the first control point to start at 0."
									// "This is reproduced here to maintain compatibility around osu!mania scroll speed and song select display."
									const long currentTime = (i == 0 ? 0 : t.offset);
									const long nextTime = (i >= uninheritedTimingpoints.size() - 1 ? lastTime : uninheritedTimingpoints[i + 1].offset);

									tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
									tuple.duration = std::max(nextTime - currentTime, (long)0);
								}

								tuple.sortHack = i;
							}
							tuples.push_back(tuple);
						}

						// "Aggregate durations into a set of (beatLength, duration) tuples for each beat length"
						std::vector<Tuple> aggregations;
						aggregations.reserve(tuples.size());
						for (size_t i=0; i<tuples.size(); i++)
						{
							const Tuple &t = tuples[i];

							bool foundExistingAggregation = false;
							size_t aggregationIndex = 0;
							for (size_t j=0; j<aggregations.size(); j++)
							{
								if (aggregations[j].beatLength == t.beatLength)
								{
									foundExistingAggregation = true;
									aggregationIndex = j;
									break;
								}
							}

							if (!foundExistingAggregation)
								aggregations.push_back(t);
							else
								aggregations[aggregationIndex].duration += t.duration;
						}

						// "Get the most common one, or 0 as a suitable default"
						struct SortByDuration
						{
						    bool operator() (Tuple const &a, Tuple const &b) const
						    {
						    	// first condition: duration
						    	// second condition: if duration is the same, higher BPM goes before lower BPM

						    	// strict weak ordering!
						    	if (a.duration == b.duration && a.beatLength == b.beatLength)
						    		return a.sortHack > b.sortHack;
						    	else if (a.duration == b.duration)
						    		return (a.beatLength < b.beatLength);
						    	else
						    		return (a.duration > b.duration);
						    }
						};
						std::sort(aggregations.begin(), aggregations.end(), SortByDuration());

						float mostCommonBPM = aggregations[0].beatLength;
						{
							// convert from msPerBeat to BPM
							const float msPerMinute = 1.0f * 60.0f * 1000.0f;
							if (mostCommonBPM != 0.0f)
								mostCommonBPM = msPerMinute / mostCommonBPM;
						}
						return (int)std::round(mostCommonBPM);
					}
				};
				diff2->m_iMostCommonBPM = MostCommonBPMHelper::calculateMostCommonBPM(uninheritedTimingpoints, (timingPoints.size() > 0 ? timingPoints[timingPoints.size() - 1].offset : 0));

				// build temp partial timingpoints, only used for menu animations
				for (int t=0; t<timingPoints.size(); t++)
				{
					OsuDatabaseBeatmap::TIMINGPOINT tp;
					{
						tp.offset = timingPoints[t].offset;
						tp.msPerBeat = timingPoints[t].msPerBeat;
						tp.timingChange = timingPoints[t].timingChange;
						tp.kiai = false;
					}
					diff2->m_timingpoints.push_back(tp);
				}
			}

			// special case: legacy fallback behavior for invalid beatmapSetID, try to parse the ID from the path
			if (beatmapSetID < 1 && path.length() > 0)
			{
				const std::vector<UString> pathTokens = path.split("\\"); // NOTE: this is hardcoded to backslash since osu is windows only
				if (pathTokens.size() > 0 && pathTokens[0].length() > 0)
				{
					const std::vector<UString> spaceTokens = pathTokens[0].split(" ");
					if (spaceTokens.size() > 0 && spaceTokens[0].length() > 0)
					{
						try
						{
							beatmapSetID = spaceTokens[0].toInt();
						}
						catch (...)
						{
							beatmapSetID = -1;
						}
					}
				}
			}

			// (the diff is now fully built)

			// now, search if the current set (to which this diff would belong) already exists and add it there, or if it doesn't exist then create the set
			const auto result = setIDToIndex.find(beatmapSetID);
			const bool beatmapSetExists = (result != setIDToIndex.end());
			if (beatmapSetExists)
				beatmapSets[result->second].diffs2.push_back(diff2);
			else
			{
				setIDToIndex[beatmapSetID] = beatmapSets.size();

				BeatmapSet s;

				s.setID = beatmapSetID;
				s.path = beatmapPath;
				s.diffs2.push_back(diff2);

				beatmapSets.push_back(s);
			}

			// and add an entry in our hashmap
			if (diff2->getMD5Hash().length() == 32)
				hashToDiff2[diff2->getMD5Hash()] = diff2;
		}
	}

	// we now have a collection of BeatmapSets (where one set is equal to one beatmap and all of its diffs), build the actual OsuBeatmap objects
	// first, build all beatmaps which have a valid setID (trusting the values from the osu database)
	std::unordered_map<std::string, OsuDatabaseBeatmap*> titleArtistToBeatmap;
	for (int i=0; i<beatmapSets.size(); i++)
	{
		if (m_bInterruptLoad.load()) break; // cancellation point

		if (beatmapSets[i].diffs2.size() > 0) // sanity check
		{
			if (beatmapSets[i].setID > 0)
			{
				OsuDatabaseBeatmap *bm = new OsuDatabaseBeatmap(m_osu, beatmapSets[i].diffs2);

				m_databaseBeatmaps.push_back(bm);

				// and add an entry in our hashmap
				for (int d=0; d<beatmapSets[i].diffs2.size(); d++)
				{
					const std::string &md5hash = beatmapSets[i].diffs2[d]->getMD5Hash();
					if (md5hash.length() == 32)
						hashToBeatmap[md5hash] = bm;
				}

				// and in the other hashmap
				UString titleArtist = bm->getTitle();
				titleArtist.append(bm->getArtist());
				if (titleArtist.length() > 0)
					titleArtistToBeatmap[std::string(titleArtist.toUtf8())] = bm;
			}
		}
	}

	// second, handle all diffs which have an invalid setID, and group them exclusively by artist and title and creator (diffs with the same artist and title and creator will end up in the same beatmap object)
	// this goes through every individual diff in a "set" (not really a set because its ID is either 0 or -1) instead of trusting the ID values from the osu database
	for (int i=0; i<beatmapSets.size(); i++)
	{
		if (m_bInterruptLoad.load()) break; // cancellation point

		if (beatmapSets[i].diffs2.size() > 0) // sanity check
		{
			if (beatmapSets[i].setID < 1)
			{
				for (int b=0; b<beatmapSets[i].diffs2.size(); b++)
				{
					if (m_bInterruptLoad.load()) break; // cancellation point

					OsuDatabaseBeatmap *diff2 = beatmapSets[i].diffs2[b];

					// try finding an already existing beatmap with matching artist and title and creator (into which we could inject this lone diff)
					bool existsAlready = false;

					// new: use hashmap
					UString titleArtistCreator = diff2->getTitle();
					titleArtistCreator.append(diff2->getArtist());
					titleArtistCreator.append(diff2->getCreator());
					if (titleArtistCreator.length() > 0)
					{
						const auto result = titleArtistToBeatmap.find(std::string(titleArtistCreator.toUtf8()));
						if (result != titleArtistToBeatmap.end())
						{
							existsAlready = true;

							// we have found a matching beatmap, add ourself to its diffs
							const_cast<std::vector<OsuDatabaseBeatmap*>&>(result->second->getDifficulties()).push_back(diff2);

							// and add an entry in our hashmap
							if (diff2->getMD5Hash().length() == 32)
								hashToBeatmap[diff2->getMD5Hash()] = result->second;
						}
					}

					// if we couldn't find any beatmap with our title and artist, create a new one
					if (!existsAlready)
					{
						std::vector<OsuDatabaseBeatmap*> diffs2;
						diffs2.push_back(beatmapSets[i].diffs2[b]);

						OsuDatabaseBeatmap *bm = new OsuDatabaseBeatmap(m_osu, diffs2);

						m_databaseBeatmaps.push_back(bm);

						// and add an entry in our hashmap
						for (int d=0; d<diffs2.size(); d++)
						{
							const std::string &md5hash = diffs2[d]->getMD5Hash();
							if (md5hash.length() == 32)
								hashToBeatmap[md5hash] = bm;
						}
					}
				}
			}
		}
	}

	m_importTimer->update();
	debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_databaseBeatmaps.size(), m_importTimer->getElapsedTime());

	// signal that we are almost done
	m_fLoadingProgress = 0.75f;

	// load legacy collection.db
	if (osu_collections_legacy_enabled.getBool())
	{
		UString legacyCollectionFilePath = osu_folder.getString();
		legacyCollectionFilePath.append("collection.db");
		loadCollections(legacyCollectionFilePath, true, hashToDiff2, hashToBeatmap);
	}

	// load custom collections.db (after having loaded legacy!)
	if (osu_collections_custom_enabled.getBool())
		loadCollections("collections.db", false, hashToDiff2, hashToBeatmap);

	std::sort(m_collections.begin(), m_collections.end(), SortCollectionByName());

	// signal that we are done
	m_fLoadingProgress = 1.0f;
}

void OsuDatabase::loadStars()
{
	if (!osu_database_stars_cache_enabled.getBool()) return;

	debugLog("OsuDatabase::loadStars()\n");

	const UString starsFilePath = "stars.cache";
	const int starsCacheVersion = 20221108;

	OsuFile cache(starsFilePath, false);
	if (cache.isReady())
	{
		m_starsCache.clear();

		const int cacheVersion = cache.readInt();

		if (cacheVersion <= starsCacheVersion)
		{
			const std::string md5Hash = cache.readStdString();

			if (OsuFile::md5(cache.getReadPointer(), cache.getFileSize() - (size_t)(cache.getReadPointer() - cache.getBuffer())) == md5Hash)
			{
				const int64_t numStarsCacheEntries = cache.readLongLong();

				debugLog("Stars cache: version = %i, numStarsCacheEntries = %i\n", cacheVersion, numStarsCacheEntries);

				for (int64_t i=0; i<numStarsCacheEntries; i++)
				{
					const std::string beatmapMD5Hash = cache.readStdString();
					const float starsNomod = cache.readFloat();

					if (beatmapMD5Hash.length() == 32) // sanity
					{
						STARS_CACHE_ENTRY entry;
						{
							entry.starsNomod = starsNomod;
						}
						m_starsCache[beatmapMD5Hash] = entry;
					}
				}
			}
			else
				debugLog("Stars cache is corrupt, ignoring.\n");
		}
		else
			debugLog("Invalid stars cache version, ignoring.\n");
	}
	else
		debugLog("No stars cache found.\n");
}

void OsuDatabase::saveStars()
{
	if (!osu_database_stars_cache_enabled.getBool()) return;

	debugLog("Osu: Saving stars ...\n");

	const UString starsFilePath = "stars.cache";
	const int starsCacheVersion = 20221108;

	//const double startTime = engine->getTimeReal();
	{
		// count
		int64_t numStarsCacheEntries = 0;
		for (OsuDatabaseBeatmap *beatmap : m_databaseBeatmaps)
		{
			for (OsuDatabaseBeatmap *diff2 : beatmap->getDifficulties())
			{
				if (diff2->getMD5Hash().size() == 32 && diff2->getStarsNomod() > 0.0f && diff2->getStarsNomod() != 0.0001f)
					numStarsCacheEntries++;
			}
		}

		if (numStarsCacheEntries < 1)
		{
			debugLog("No stars cached, nothing to write.\n");
			return;
		}

		OsuFile cache(starsFilePath, true);
		if (cache.isReady())
		{
			// HACKHACK: code duplication is absolutely stupid, just so we can calculate the MD5 hash. but I'm too lazy to make it cleaner right now ffs
			OsuFile tempForHash("", false, true);
			{
				tempForHash.writeLongLong(numStarsCacheEntries);

				for (OsuDatabaseBeatmap *beatmap : m_databaseBeatmaps)
				{
					for (OsuDatabaseBeatmap *diff2 : beatmap->getDifficulties())
					{
						if (diff2->getMD5Hash().size() == 32 && diff2->getStarsNomod() > 0.0f && diff2->getStarsNomod() != 0.0001f)
						{
							tempForHash.writeStdString(diff2->getMD5Hash());
							tempForHash.writeFloat(diff2->getStarsNomod());
						}
					}
				}
			}
			const std::string md5Hash = (tempForHash.getWriteBuffer().size() > 0 ? OsuFile::md5((const unsigned char*)&tempForHash.getWriteBuffer()[0], tempForHash.getWriteBuffer().size()) : "");

			// write
			{
				cache.writeInt(starsCacheVersion);
				cache.writeStdString(md5Hash);
				cache.writeLongLong(numStarsCacheEntries);

				for (OsuDatabaseBeatmap *beatmap : m_databaseBeatmaps)
				{
					for (OsuDatabaseBeatmap *diff2 : beatmap->getDifficulties())
					{
						if (diff2->getMD5Hash().size() == 32 && diff2->getStarsNomod() > 0.0f && diff2->getStarsNomod() != 0.0001f)
						{
							cache.writeStdString(diff2->getMD5Hash());
							cache.writeFloat(diff2->getStarsNomod());
						}
					}
				}
			}
		}
		else
			debugLog("Couldn't write stars.cache!\n");
	}
	//debugLog("Took %f seconds.\n", (engine->getTimeReal() - startTime));
}

void OsuDatabase::loadScores()
{
	if (m_bScoresLoaded) return;

	debugLog("OsuDatabase::loadScores()\n");

	// reset
	m_scores.clear();

	// load custom scores
	// NOTE: custom scores are loaded before legacy scores (because we want to be able to skip loading legacy scores which were already previously imported at some point)
	size_t customScoresFileSize = 0;
	if (osu_scores_custom_enabled.getBool())
	{
		const int maxSupportedCustomDbVersion = osu_scores_custom_version.getInt();
		const unsigned char hackIsImportedLegacyScoreFlag = 0xA9; // TODO: remove this once all builds on steam (even previous-version) have loading version cap logic

		int makeBackupType = 0;
		const int backupLessThanVersion = 20210103;
		const int backupMoreThanVersion = 20210105;

		const UString scoresFilePath = (m_osu->isInVRMode() ? "scoresvr.db" : "scores.db");
		{
			OsuFile db(scoresFilePath, false);
			if (db.isReady())
			{
				customScoresFileSize = db.getFileSize();

				const int dbVersion = db.readInt();
				const int numBeatmaps = db.readInt();

				if (dbVersion > backupMoreThanVersion)
					makeBackupType = 2;
				else if (dbVersion < backupLessThanVersion)
					makeBackupType = 1;

				debugLog("Custom scores: version = %i, numBeatmaps = %i\n", dbVersion, numBeatmaps);

				if (dbVersion <= maxSupportedCustomDbVersion)
				{
					int scoreCounter = 0;
					for (int b=0; b<numBeatmaps; b++)
					{
						const std::string md5hash = db.readStdString();
						const int numScores = db.readInt();

						if (md5hash.length() < 32)
						{
							debugLog("WARNING: Invalid score with md5hash.length() = %i!\n", md5hash.length());
							continue;
						}
						else if (md5hash.length() > 32)
						{
							debugLog("ERROR: Corrupt score database/entry detected, stopping.\n");
							break;
						}

						if (Osu::debug->getBool())
							debugLog("Beatmap[%i]: md5hash = %s, numScores = %i\n", b, md5hash.c_str(), numScores);

						for (int s=0; s<numScores; s++)
						{
							const unsigned char gamemode = db.readByte(); // NOTE: abused as isImportedLegacyScore flag (because I forgot to add a version cap to old builds)
							const int scoreVersion = db.readInt();
							bool isImportedLegacyScore = false;
							if (dbVersion == 20210103 && scoreVersion > 20190103)
							{
								isImportedLegacyScore = db.readBool();
							}
							else if (dbVersion > 20210103 && scoreVersion > 20190103)
							{
								// HACKHACK: for explanation see hackIsImportedLegacyScoreFlag
								isImportedLegacyScore = (gamemode & hackIsImportedLegacyScoreFlag);
							}
							const uint64_t unixTimestamp = db.readLongLong();

							// default
							const UString playerName = db.readString();

							const short num300s = db.readShort();
							const short num100s = db.readShort();
							const short num50s = db.readShort();
							const short numGekis = db.readShort();
							const short numKatus = db.readShort();
							const short numMisses = db.readShort();

							const unsigned long long score = db.readLongLong();
							const short maxCombo = db.readShort();
							const int modsLegacy = db.readInt();

							// custom
							const short numSliderBreaks = db.readShort();
							const float pp = db.readFloat();
							const float unstableRate = db.readFloat();
							const float hitErrorAvgMin = db.readFloat();
							const float hitErrorAvgMax = db.readFloat();
							const float starsTomTotal = db.readFloat();
							const float starsTomAim = db.readFloat();
							const float starsTomSpeed = db.readFloat();
							const float speedMultiplier = db.readFloat();
							const float CS = db.readFloat();
							const float AR = db.readFloat();
							const float OD = db.readFloat();
							const float HP = db.readFloat();

							int maxPossibleCombo = -1;
							int numHitObjects = -1;
							int numCircles = -1;
							if (scoreVersion > 20180722)
							{
								maxPossibleCombo = db.readInt();
								numHitObjects = db.readInt();
								numCircles = db.readInt();
							}

							const UString experimentalMods = db.readString();

							if (gamemode == 0x0 || (dbVersion > 20210103 && scoreVersion > 20190103)) // gamemode filter (osu!standard) // HACKHACK: for explanation see hackIsImportedLegacyScoreFlag
							{
								Score sc;

								sc.isLegacyScore = false;
								sc.isImportedLegacyScore = isImportedLegacyScore;
								sc.version = scoreVersion;
								sc.unixTimestamp = unixTimestamp;

								// default
								sc.playerName = playerName;

								sc.num300s = num300s;
								sc.num100s = num100s;
								sc.num50s = num50s;
								sc.numGekis = numGekis;
								sc.numKatus = numKatus;
								sc.numMisses = numMisses;
								sc.score = score;
								sc.comboMax = maxCombo;
								sc.perfect = (maxPossibleCombo > 0 && sc.comboMax > 0 && sc.comboMax >= maxPossibleCombo);
								sc.modsLegacy = modsLegacy;

								// custom
								sc.numSliderBreaks = numSliderBreaks;
								sc.pp = pp;
								sc.unstableRate = unstableRate;
								sc.hitErrorAvgMin = hitErrorAvgMin;
								sc.hitErrorAvgMax = hitErrorAvgMax;
								sc.starsTomTotal = starsTomTotal;
								sc.starsTomAim = starsTomAim;
								sc.starsTomSpeed = starsTomSpeed;
								sc.speedMultiplier = speedMultiplier;
								sc.CS = CS;
								sc.AR = AR;
								sc.OD = OD;
								sc.HP = HP;
								sc.maxPossibleCombo = maxPossibleCombo;
								sc.numHitObjects = numHitObjects;
								sc.numCircles = numCircles;
								sc.experimentalModsConVars = experimentalMods;

								// runtime
								sc.sortHack = m_iSortHackCounter++;
								sc.md5hash = md5hash;

								addScoreRaw(md5hash, sc);
								scoreCounter++;
							}
						}
					}
					debugLog("Loaded %i individual scores.\n", scoreCounter);
				}
				else
					debugLog("Newer scores.db version is not backwards compatible with old clients.\n");
			}
			else
				debugLog("No custom scores found.\n");
		}

		// one-time-backup for special occasions (sanity)
		if (makeBackupType > 0)
		{
			File originalScoresFile(scoresFilePath);
			if (originalScoresFile.canRead())
			{
				UString backupScoresFilePath = scoresFilePath;
				const int forcedBackupCounter = 8;
				backupScoresFilePath.append(UString::format(".%i_%i.backup", (makeBackupType < 2 ? backupLessThanVersion : maxSupportedCustomDbVersion), forcedBackupCounter));

				if (!env->fileExists(backupScoresFilePath)) // NOTE: avoid overwriting when people switch betas
				{
					File backupScoresFile(backupScoresFilePath, File::TYPE::WRITE);
					if (backupScoresFile.canWrite())
					{
						const char *originalScoresFileBytes = originalScoresFile.readFile();
						if (originalScoresFileBytes != NULL)
							backupScoresFile.write(originalScoresFileBytes, originalScoresFile.getFileSize());
					}
				}
			}
		}
	}

	// load legacy osu scores
	if (osu_scores_legacy_enabled.getBool())
	{
		UString scoresPath = osu_folder.getString();
		scoresPath.append("scores.db");

		OsuFile db(scoresPath, false);
		if (db.isReady())
		{
			if (db.getFileSize() != customScoresFileSize) // HACKHACK: heuristic sanity check (some people have their osu!folder point directly to McOsu, which would break legacy score db loading here since there is no magic number)
			{
				const int dbVersion = db.readInt();
				const int numBeatmaps = db.readInt();

				debugLog("Legacy scores: version = %i, numBeatmaps = %i\n", dbVersion, numBeatmaps);

				int scoreCounter = 0;
				for (int b=0; b<numBeatmaps; b++)
				{
					const std::string md5hash = db.readStdString();

					if (md5hash.length() < 32)
					{
						debugLog("WARNING: Invalid score with md5hash.length() = %i!\n", md5hash.length());
						continue;
					}
					else if (md5hash.length() > 32)
					{
						debugLog("ERROR: Corrupt score database/entry detected, stopping.\n");
						break;
					}

					const int numScores = db.readInt();

					if (Osu::debug->getBool())
						debugLog("Beatmap[%i]: md5hash = %s, numScores = %i\n", b, md5hash.c_str(), numScores);

					for (int s=0; s<numScores; s++)
					{
						const unsigned char gamemode = db.readByte();
						const int scoreVersion = db.readInt();
						const UString beatmapHash = db.readString();

						const UString playerName = db.readString();
						const UString replayHash = db.readString();

						const short num300s = db.readShort();
						const short num100s = db.readShort();
						const short num50s = db.readShort();
						const short numGekis = db.readShort();
						const short numKatus = db.readShort();
						const short numMisses = db.readShort();

						const int score = db.readInt();
						const short maxCombo = db.readShort();
						const bool perfect = db.readBool();

						const int mods = db.readInt();
						const UString hpGraphString = db.readString();
						const long long ticksWindows = db.readLongLong();

						db.readByteArray(); // replayCompressed

						/*long long onlineScoreID = 0;*/
						if (scoreVersion >= 20131110)
							/*onlineScoreID = */db.readLongLong();
						else if (scoreVersion >= 20121008)
							/*onlineScoreID = */db.readInt();

						if (mods & OsuReplay::Mods::Target)
							/*double totalAccuracy = */db.readDouble();

						if (gamemode == 0x0) // gamemode filter (osu!standard)
						{
							Score sc;

							sc.isLegacyScore = true;
							sc.isImportedLegacyScore = false;
							sc.version = scoreVersion;
							sc.unixTimestamp = (ticksWindows - 621355968000000000) / 10000000;

							// default
							sc.playerName = playerName;

							sc.num300s = num300s;
							sc.num100s = num100s;
							sc.num50s = num50s;
							sc.numGekis = numGekis;
							sc.numKatus = numKatus;
							sc.numMisses = numMisses;
							sc.score = (score < 0 ? 0 : score);
							sc.comboMax = maxCombo;
							sc.perfect = perfect;
							sc.modsLegacy = mods;

							// custom
							sc.numSliderBreaks = 0;
							sc.pp = 0.0f;
							sc.unstableRate = 0.0f;
							sc.hitErrorAvgMin = 0.0f;
							sc.hitErrorAvgMax = 0.0f;
							sc.starsTomTotal = 0.0f;
							sc.starsTomAim = 0.0f;
							sc.starsTomSpeed = 0.0f;
							sc.speedMultiplier = (mods & OsuReplay::Mods::HalfTime ? 0.75f : (((mods & OsuReplay::Mods::DoubleTime) || (mods & OsuReplay::Mods::Nightcore)) ? 1.5f : 1.0f));
							sc.CS = 0.0f; sc.AR = 0.0f; sc.OD = 0.0f; sc.HP = 0.0f;
							sc.maxPossibleCombo = -1;
							sc.numHitObjects = -1;
							sc.numCircles = -1;
							//sc.experimentalModsConVars = "";

							// runtime
							sc.sortHack = m_iSortHackCounter++;
							sc.md5hash = md5hash;

							scoreCounter++;

							// NOTE: avoid adding an already imported legacy score (since that just spams the scorebrowser with useless information)
							bool isScoreAlreadyImported = false;
							{
								const std::vector<OsuDatabase::Score> &otherScores = m_scores[sc.md5hash];

								for (size_t s=0; s<otherScores.size(); s++)
								{
									if (sc.isLegacyScoreEqualToImportedLegacyScore(otherScores[s]))
									{
										isScoreAlreadyImported = true;
										break;
									}
								}
							}

							if (!isScoreAlreadyImported)
								addScoreRaw(md5hash, sc);
						}
					}
				}
				debugLog("Loaded %i individual scores.\n", scoreCounter);
			}
			else
				debugLog("Not loading legacy scores because filesize matches custom scores.\n");
		}
		else
			debugLog("No legacy scores found.\n");
	}

	if (m_scores.size() > 0)
		m_bScoresLoaded = true;
}

void OsuDatabase::saveScores()
{
	if (!m_bDidScoresChangeForSave) return;
	m_bDidScoresChangeForSave = false;

	const int dbVersion = osu_scores_custom_version.getInt();
	const unsigned char hackIsImportedLegacyScoreFlag = 0xA9; // TODO: remove this once all builds on steam (even previous-version) have loading version cap logic

	if (m_scores.size() > 0)
	{
		debugLog("Osu: Saving scores ...\n");

		OsuFile db((m_osu->isInVRMode() ? "scoresvr.db" : "scores.db"), true);
		if (db.isReady())
		{
			const double startTime = engine->getTimeReal();

			// count number of beatmaps with valid scores
			int numBeatmaps = 0;
			for (std::unordered_map<std::string, std::vector<Score>>::iterator it = m_scores.begin(); it != m_scores.end(); ++it)
			{
				for (int i=0; i<it->second.size(); i++)
				{
					if (!it->second[i].isLegacyScore)
					{
						numBeatmaps++;
						break;
					}
				}
			}

			// write header
			db.writeInt(dbVersion);
			db.writeInt(numBeatmaps);

			// write scores for each beatmap
			for (std::unordered_map<std::string, std::vector<Score>>::iterator it = m_scores.begin(); it != m_scores.end(); ++it)
			{
				int numNonLegacyScores = 0;
				for (int i=0; i<it->second.size(); i++)
				{
					if (!it->second[i].isLegacyScore)
						numNonLegacyScores++;
				}

				if (numNonLegacyScores > 0)
				{
					db.writeStdString(it->first);	// md5hash
					db.writeInt(numNonLegacyScores);// numScores

					for (int i=0; i<it->second.size(); i++)
					{
						if (!it->second[i].isLegacyScore)
						{
							db.writeByte((it->second[i].version > 20190103 ? (it->second[i].isImportedLegacyScore ? hackIsImportedLegacyScoreFlag : 0) : 0)); // gamemode (hardcoded atm) // NOTE: abused as isImportedLegacyScore flag (because I forgot to add a version cap to old builds)
							db.writeInt(it->second[i].version);
							// HACKHACK: for explanation see hackIsImportedLegacyScoreFlag
							/*
							if (it->second[i].version > 20190103)
							{
								db.writeBool(it->second[i].isImportedLegacyScore);
							}
							*/
							db.writeLongLong(it->second[i].unixTimestamp);

							// default
							db.writeString(it->second[i].playerName);

							db.writeShort(it->second[i].num300s);
							db.writeShort(it->second[i].num100s);
							db.writeShort(it->second[i].num50s);
							db.writeShort(it->second[i].numGekis);
							db.writeShort(it->second[i].numKatus);
							db.writeShort(it->second[i].numMisses);

							db.writeLongLong(it->second[i].score);
							db.writeShort(it->second[i].comboMax);
							db.writeInt(it->second[i].modsLegacy);

							// custom
							db.writeShort(it->second[i].numSliderBreaks);
							db.writeFloat(it->second[i].pp);
							db.writeFloat(it->second[i].unstableRate);
							db.writeFloat(it->second[i].hitErrorAvgMin);
							db.writeFloat(it->second[i].hitErrorAvgMax);
							db.writeFloat(it->second[i].starsTomTotal);
							db.writeFloat(it->second[i].starsTomAim);
							db.writeFloat(it->second[i].starsTomSpeed);
							db.writeFloat(it->second[i].speedMultiplier);
							db.writeFloat(it->second[i].CS);
							db.writeFloat(it->second[i].AR);
							db.writeFloat(it->second[i].OD);
							db.writeFloat(it->second[i].HP);

							if (it->second[i].version > 20180722)
							{
								db.writeInt(it->second[i].maxPossibleCombo);
								db.writeInt(it->second[i].numHitObjects);
								db.writeInt(it->second[i].numCircles);
							}

							db.writeString(it->second[i].experimentalModsConVars);
						}
					}
				}
			}

			db.write();

			debugLog("Took %f seconds.\n", (engine->getTimeReal() - startTime));
		}
		else
			debugLog("Couldn't write scores.db!\n");
	}
}

void OsuDatabase::loadCollections(UString collectionFilePath, bool isLegacy, const std::unordered_map<std::string, OsuDatabaseBeatmap*> &hashToDiff2, const std::unordered_map<std::string, OsuDatabaseBeatmap*> &hashToBeatmap)
{
	bool wasInterrupted = false;

	struct CollectionLoadingHelper
	{
		static void addBeatmapsEntryForBeatmapAndDiff2(Collection &c, OsuDatabaseBeatmap *beatmap, OsuDatabaseBeatmap *diff2, std::atomic<bool> &interruptLoad, bool &wasInterrupted)
		{
			if (beatmap == NULL || diff2 == NULL) return;

			// we now have one matching OsuBeatmap and OsuBeatmapDifficulty, add either of them if they don't exist yet
			bool beatmapIsAlreadyInCollection = false;
			{
				for (int m=0; m<c.beatmaps.size(); m++)
				{
					if (interruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

					if (c.beatmaps[m].first == beatmap)
					{
						beatmapIsAlreadyInCollection = true;

						// the beatmap already exists, check if we have to add the current diff
						bool diffIsAlreadyInCollection = false;
						{
							for (int d=0; d<c.beatmaps[m].second.size(); d++)
							{
								if (interruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

								if (c.beatmaps[m].second[d] == diff2)
								{
									diffIsAlreadyInCollection = true;
									break;
								}
							}
						}
						if (!diffIsAlreadyInCollection)
							c.beatmaps[m].second.push_back(diff2);

						break;
					}
				}
			}
			if (!beatmapIsAlreadyInCollection)
			{
				std::vector<OsuDatabaseBeatmap*> diffs2;
				{
					diffs2.push_back(diff2);
				}
				c.beatmaps.push_back(std::pair<OsuDatabaseBeatmap*, std::vector<OsuDatabaseBeatmap*>>(beatmap, diffs2));
			}
		}
	};

	OsuFile collectionFile(collectionFilePath);
	if (collectionFile.isReady())
	{
		const int version = collectionFile.readInt();
		const int numCollections = collectionFile.readInt();

		debugLog("Collection: version = %i, numCollections = %i\n", version, numCollections);

		const bool isLegacyAndVersionValid = (isLegacy && (version <= osu_database_version.getInt() || osu_database_ignore_version.getBool()));
		const bool isCustomAndVersionValid = (!isLegacy && (version <= osu_collections_custom_version.getInt()));

		if (isLegacyAndVersionValid || isCustomAndVersionValid)
		{
			for (int i=0; i<numCollections; i++)
			{
				if (m_bInterruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

				m_fLoadingProgress = 0.75f + 0.24f*((float)(i+1)/(float)numCollections);

				UString name = collectionFile.readString();
				const int numBeatmaps = collectionFile.readInt();

				if (Osu::debug->getBool())
					debugLog("Raw Collection #%i: name = %s, numBeatmaps = %i\n", i, name.toUtf8(), numBeatmaps);

				Collection c;
				c.isLegacyCollection = isLegacy;
				c.name = name;

				for (int b=0; b<numBeatmaps; b++)
				{
					if (m_bInterruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

					std::string md5hash = collectionFile.readStdString();

					CollectionEntry entry;
					{
						entry.isLegacyEntry = isLegacy;

						entry.hash = md5hash;
					}
					c.hashes.push_back(entry);
				}

				if (c.hashes.size() > 0)
				{
					// collect OsuBeatmaps corresponding to this collection

					// go through every hash of the collection
					std::vector<OsuDatabaseBeatmap*> matchingDiffs2;
					for (int h=0; h<c.hashes.size(); h++)
					{
						if (m_bInterruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

						// new: use hashmap
						if (c.hashes[h].hash.length() == 32)
						{
							const auto result = hashToDiff2.find(c.hashes[h].hash);
							if (result != hashToDiff2.end())
								matchingDiffs2.push_back(result->second);
						}
					}

					// we now have an array of all OsuBeatmapDifficulty objects within this collection

					// go through every found OsuBeatmapDifficulty
					for (int md=0; md<matchingDiffs2.size(); md++)
					{
						if (m_bInterruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

						OsuDatabaseBeatmap *diff2 = matchingDiffs2[md];

						if (diff2 == NULL) continue;

						// find the OsuBeatmap object corresponding to this diff
						OsuDatabaseBeatmap *beatmap = NULL;
						if (diff2->getMD5Hash().length() == 32)
						{
							// new: use hashmap
							const auto result = hashToBeatmap.find(diff2->getMD5Hash());
							if (result != hashToBeatmap.end())
								beatmap = result->second;
						}

						CollectionLoadingHelper::addBeatmapsEntryForBeatmapAndDiff2(c, beatmap, diff2, m_bInterruptLoad, wasInterrupted);
					}
				}

				// add the collection
				// check if we already have a collection with that name, if so then just add our new entries to it (necessary since this function will load both osu!'s collection.db as well as our own custom collections.db)
				// this handles all the merging between both legacy and custom collections
				{
					bool collectionAlreadyExists = false;
					for (size_t cc=0; cc<m_collections.size(); cc++)
					{
						if (m_bInterruptLoad.load()) {wasInterrupted = true; break;} // cancellation point

						Collection &existingCollection = m_collections[cc];
						if (existingCollection.name == c.name)
						{
							collectionAlreadyExists = true;

							// merge with existing collection
							{
								for (size_t e=0; e<c.hashes.size(); e++)
								{
									const std::string &toBeAddedEntryHash = c.hashes[e].hash;

									bool entryAlreadyExists = false;
									{
										for (size_t ee=0; ee<existingCollection.hashes.size(); ee++)
										{
											const std::string &existingEntryHash = existingCollection.hashes[ee].hash;
											if (existingEntryHash == toBeAddedEntryHash)
											{
												entryAlreadyExists = true;
												break;
											}
										}
									}

									// if the entry does not yet exist in the existing collection, then add that entry
									if (!entryAlreadyExists)
									{
										// add to .hashes
										{
											CollectionEntry toBeAddedEntry;
											{
												toBeAddedEntry.isLegacyEntry = isLegacy;

												toBeAddedEntry.hash = toBeAddedEntryHash;
											}
											existingCollection.hashes.push_back(toBeAddedEntry);
										}

										// add to .beatmaps
										{
											const auto result = hashToDiff2.find(toBeAddedEntryHash);
											if (result != hashToDiff2.end())
											{
												OsuDatabaseBeatmap *diff2 = result->second;

												if (diff2 != NULL)
												{
													// find the OsuBeatmap object corresponding to this diff
													OsuDatabaseBeatmap *beatmap = NULL;
													if (diff2->getMD5Hash().length() == 32)
													{
														// new: use hashmap
														const auto result = hashToBeatmap.find(diff2->getMD5Hash());
														if (result != hashToBeatmap.end())
															beatmap = result->second;
													}

													CollectionLoadingHelper::addBeatmapsEntryForBeatmapAndDiff2(existingCollection, beatmap, diff2, m_bInterruptLoad, wasInterrupted);
												}
											}
										}
									}
								}
							}

							break;
						}
					}
					if (!collectionAlreadyExists)
						m_collections.push_back(c);
				}
			}
		}
		else
			m_osu->getNotificationOverlay()->addNotification(UString::format("collection.db version unknown (%i),  skipping loading.", version), 0xffffff00, false, 5.0f);

		if (Osu::debug->getBool())
		{
			for (int i=0; i<m_collections.size(); i++)
			{
				debugLog("Collection #%i: name = %s, numBeatmaps = %i\n", i, m_collections[i].name.toUtf8(), m_collections[i].beatmaps.size());
			}
		}
	}
	else
		debugLog("OsuBeatmapDatabase::loadDB() : Couldn't load %s", collectionFilePath.toUtf8());

	// backup
	if (!isLegacy)
	{
		File originalCollectionsFile(collectionFilePath);
		if (originalCollectionsFile.canRead())
		{
			UString backupCollectionsFilePath = collectionFilePath;
			const int forcedBackupCounter = 7;
			backupCollectionsFilePath.append(UString::format(".%i_%i.backup", osu_collections_custom_version.getInt(), forcedBackupCounter));

			if (!env->fileExists(backupCollectionsFilePath)) // NOTE: avoid overwriting when people switch betas
			{
				File backupCollectionsFile(backupCollectionsFilePath, File::TYPE::WRITE);
				if (backupCollectionsFile.canWrite())
				{
					const char *originalCollectionsFileBytes = originalCollectionsFile.readFile();
					if (originalCollectionsFileBytes != NULL)
						backupCollectionsFile.write(originalCollectionsFileBytes, originalCollectionsFile.getFileSize());
				}
			}
		}
	}

	// don't keep partially loaded collections. the user should notice at that point that it's not a good idea to edit collections after having interrupted loading.
	if (wasInterrupted)
		m_collections.clear();
}

void OsuDatabase::saveCollections()
{
	if (!m_bDidCollectionsChangeForSave) return;
	m_bDidCollectionsChangeForSave = false;

	const int32_t dbVersion = osu_collections_custom_version.getInt();

	if (m_collections.size() > 0)
	{
		debugLog("Osu: Saving collections ...\n");

		OsuFile db("collections.db", true);
		if (db.isReady())
		{
			const double startTime = engine->getTimeReal();

			// check how much we actually have to save
			// note that we are only saving non-legacy collections and entries (i.e. things which were added/deleted inside mcosu)
			// reason being that it is more annoying to have osu!-side collections modifications be completely ignored (because we would make a full copy initially)
			// if a collection or entry is deleted in osu!, then you would expect it to also be deleted here
			// but, if a collection or entry is added in mcosu, then deleting the collection in osu! should only delete all osu!-side entries
			int32_t numNonLegacyCollectionsOrCollectionsWithNonLegacyEntries = 0;
			for (size_t c=0; c<m_collections.size(); c++)
			{
				if (!m_collections[c].isLegacyCollection)
					numNonLegacyCollectionsOrCollectionsWithNonLegacyEntries++;
				else
				{
					// does this legacy collection have any non-legacy entries?
					for (size_t h=0; h<m_collections[c].hashes.size(); h++)
					{
						if (!m_collections[c].hashes[h].isLegacyEntry)
						{
							numNonLegacyCollectionsOrCollectionsWithNonLegacyEntries++;
							break;
						}
					}
				}
			}

			db.writeInt(dbVersion);
			db.writeInt(numNonLegacyCollectionsOrCollectionsWithNonLegacyEntries);

			if (numNonLegacyCollectionsOrCollectionsWithNonLegacyEntries > 0)
			{
				for (size_t c=0; c<m_collections.size(); c++)
				{
					bool hasNonLegacyEntries = false;
					{
						for (size_t h=0; h<m_collections[c].hashes.size(); h++)
						{
							if (!m_collections[c].hashes[h].isLegacyEntry)
							{
								hasNonLegacyEntries = true;
								break;
							}
						}
					}

					if (!m_collections[c].isLegacyCollection || hasNonLegacyEntries)
					{
						int32_t numNonLegacyEntries = 0;
						for (size_t h=0; h<m_collections[c].hashes.size(); h++)
						{
							if (!m_collections[c].hashes[h].isLegacyEntry)
								numNonLegacyEntries++;
						}

						db.writeString(m_collections[c].name);
						db.writeInt(numNonLegacyEntries);

						for (size_t h=0; h<m_collections[c].hashes.size(); h++)
						{
							if (!m_collections[c].hashes[h].isLegacyEntry)
								db.writeStdString(m_collections[c].hashes[h].hash);
						}
					}
				}
			}

			db.write();

			debugLog("Took %f seconds.\n", (engine->getTimeReal() - startTime));
		}
		else
			debugLog("Couldn't write collections.db!\n");
	}
}

OsuDatabaseBeatmap *OsuDatabase::loadRawBeatmap(UString beatmapPath)
{
	if (Osu::debug->getBool())
		debugLog("OsuBeatmapDatabase::loadRawBeatmap() : %s\n", beatmapPath.toUtf8());

	// try loading all diffs
	std::vector<OsuDatabaseBeatmap*> diffs2;
	{
		std::vector<UString> beatmapFiles = env->getFilesInFolder(beatmapPath);
		for (int i=0; i<beatmapFiles.size(); i++)
		{
			UString ext = env->getFileExtensionFromFilePath(beatmapFiles[i]);

			UString fullFilePath = beatmapPath;
			fullFilePath.append(beatmapFiles[i]);

			// load diffs
			if (ext == "osu")
			{
				OsuDatabaseBeatmap *diff2 = new OsuDatabaseBeatmap(m_osu, fullFilePath, beatmapPath);

				// try to load it. if successful save it, else cleanup and continue to the next osu file
				if (!OsuDatabaseBeatmap::loadMetadata(diff2))
				{
					if (Osu::debug->getBool())
					{
						debugLog("OsuBeatmapDatabase::loadRawBeatmap() : Couldn't loadMetadata(), deleting object.\n");
						if (diff2->getGameMode() == 0)
							engine->showMessageWarning("OsuBeatmapDatabase::loadRawBeatmap()", "Couldn't loadMetadata()\n");
					}
					SAFE_DELETE(diff2);
					continue;
				}

				// (metadata loaded successfully)

				// NOTE: if we have our own stars cached then use that
				{
					const auto result = m_starsCache.find(diff2->getMD5Hash());
					if (result != m_starsCache.end())
						diff2->m_fStarsNomod = result->second.starsNomod;
				}

				diffs2.push_back(diff2);
			}
		}
	}

	// build beatmap from diffs
	OsuDatabaseBeatmap *beatmap = NULL;
	{
		if (diffs2.size() > 0)
		{
			beatmap = new OsuDatabaseBeatmap(m_osu, diffs2);

			// and add entries in our hashmaps
			for (size_t i=0; i<diffs2.size(); i++)
			{
				OsuDatabaseBeatmap *diff2 = diffs2[i];
				if (diff2->getMD5Hash().length() == 32)
				{
					m_rawHashToDiff2[diff2->getMD5Hash()] = diff2;
					m_rawHashToBeatmap[diff2->getMD5Hash()] = beatmap;
				}
			}
		}
	}

	return beatmap;
}

void OsuDatabase::onScoresRename(UString args)
{
	if (args.length() < 2)
	{
		m_osu->getNotificationOverlay()->addNotification(UString::format("Usage: %s MyNewName", osu_scores_rename.getName().toUtf8()));
		return;
	}

	const UString playerName = m_name_ref->getString();

	debugLog("Renaming scores \"%s\" to \"%s\"\n", playerName.toUtf8(), args.toUtf8());

	int numRenamedScores = 0;
	for (auto &kv : m_scores)
	{
		for (size_t i=0; i<kv.second.size(); i++)
		{
			Score &score = kv.second[i];

			if (!score.isLegacyScore && score.playerName == playerName)
			{
				numRenamedScores++;
				score.playerName = args;
			}
		}
	}

	if (numRenamedScores < 1)
		m_osu->getNotificationOverlay()->addNotification("No (pp) scores for active user.");
	else
	{
		m_osu->getNotificationOverlay()->addNotification(UString::format("Renamed %i scores.", numRenamedScores));

		m_bDidScoresChangeForSave = true;
		m_bDidScoresChangeForStats = true;
	}
}

void OsuDatabase::onScoresExport()
{
	const UString exportFilePath = "scores.csv";

	debugLog("Exporting currently loaded scores to \"%s\" (overwriting existing file) ...\n", exportFilePath.toUtf8());

	std::ofstream out(exportFilePath.toUtf8());
	if (!out.good())
	{
		debugLog("ERROR: Couldn't write %s\n", exportFilePath.toUtf8());
		return;
	}

	out << "#beatmapMD5hash,beatmapID,beatmapSetID,isImportedLegacyScore,version,unixTimestamp,playerName,num300s,num100s,num50s,numGekis,numKatus,numMisses,score,comboMax,perfect,modsLegacy,numSliderBreaks,pp,unstableRate,hitErrorAvgMin,hitErrorAvgMax,starsTomTotal,starsTomAim,starsTomSpeed,speedMultiplier,CS,AR,OD,HP,maxPossibleCombo,numHitObjects,numCircles,experimentalModsConVars\n";

	for (auto beatmapScores : m_scores)
	{
		bool triedGettingDatabaseBeatmapOnceForThisBeatmap = false;
		OsuDatabaseBeatmap *beatmap = NULL;

		for (const Score &score : beatmapScores.second)
		{
			if (!score.isLegacyScore)
			{
				long id = -1;
				long setId = -1;
				{
					if (beatmap == NULL && !triedGettingDatabaseBeatmapOnceForThisBeatmap)
					{
						triedGettingDatabaseBeatmapOnceForThisBeatmap = true;
						beatmap = getBeatmapDifficulty(beatmapScores.first);
					}

					if (beatmap != NULL)
					{
						id = beatmap->getID();
						setId = beatmap->getSetID();
					}
				}

				out << beatmapScores.first; // md5 hash
				out << ",";
				out << id;
				out << ",";
				out << setId;
				out << ",";

				out << score.isImportedLegacyScore;
				out << ",";
				out << score.version;
				out << ",";
				out << score.unixTimestamp;
				out << ",";

				out << std::string(score.playerName.toUtf8());
				out << ",";

				out << score.num300s;
				out << ",";
				out << score.num100s;
				out << ",";
				out << score.num50s;
				out << ",";
				out << score.numGekis;
				out << ",";
				out << score.numKatus;
				out << ",";
				out << score.numMisses;
				out << ",";

				out << score.score;
				out << ",";
				out << score.comboMax;
				out << ",";
				out << score.perfect;
				out << ",";
				out << score.modsLegacy;
				out << ",";

				out << score.numSliderBreaks;
				out << ",";
				out << score.pp;
				out << ",";
				out << score.unstableRate;
				out << ",";
				out << score.hitErrorAvgMin;
				out << ",";
				out << score.hitErrorAvgMax;
				out << ",";
				out << score.starsTomTotal;
				out << ",";
				out << score.starsTomAim;
				out << ",";
				out << score.starsTomSpeed;
				out << ",";
				out << score.speedMultiplier;
				out << ",";
				out << score.CS;
				out << ",";
				out << score.AR;
				out << ",";
				out << score.OD;
				out << ",";
				out << score.HP;
				out << ",";
				out << score.maxPossibleCombo;
				out << ",";
				out << score.numHitObjects;
				out << ",";
				out << score.numCircles;
				out << ",";
				out << std::string(score.experimentalModsConVars.toUtf8());

				out << "\n";
			}
		}

		triedGettingDatabaseBeatmapOnceForThisBeatmap = false;
		beatmap = NULL;
	}

	out.close();

	debugLog("Done.\n");
}
