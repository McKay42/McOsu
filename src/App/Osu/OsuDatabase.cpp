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
#include "OsuBackgroundStarCalcHandler.h"
#include "OsuNotificationOverlay.h"

#include "OsuDatabaseBeatmap.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

ConVar osu_folder("osu_folder", "C:/Program Files (x86)/osu!/");

#elif defined __linux__

ConVar osu_folder("osu_folder", "/media/pg/Win7/Program Files (x86)/osu!/");

#elif defined __APPLE__

ConVar osu_folder("osu_folder", "/osu!/");

#elif defined __SWITCH__

ConVar osu_folder("osu_folder", "sdmc:/switch/McOsu/");

#else

#error "put correct default folder convar here"

#endif

ConVar osu_folder_sub_songs("osu_folder_sub_songs", "Songs/");
ConVar osu_folder_sub_skins("osu_folder_sub_skins", "Skins/");

ConVar osu_database_enabled("osu_database_enabled", true);
ConVar osu_database_dynamic_star_calculation("osu_database_dynamic_star_calculation", true, "dynamically calculate star ratings in the background");
ConVar osu_database_version("osu_database_version", 20191114, "maximum supported version, above this will use fallback loader");
ConVar osu_database_ignore_version_warnings("osu_database_ignore_version_warnings", false);
ConVar osu_database_ignore_version("osu_database_ignore_version", false, "ignore upper version limit and force load the db file (may crash)");
ConVar osu_scores_enabled("osu_scores_enabled", true);
ConVar osu_scores_legacy_enabled("osu_scores_legacy_enabled", true, "load osu!'s scores.db");
ConVar osu_scores_custom_enabled("osu_scores_custom_enabled", true, "load custom scores.db");
ConVar osu_scores_save_immediately("osu_scores_save_immediately", true, "write scores.db as soon as a new score is added");
ConVar osu_scores_sort_by_pp("osu_scores_sort_by_pp", true, "display pp in score browser instead of score");
ConVar osu_user_include_relax_and_autopilot_for_stats("osu_user_include_relax_and_autopilot_for_stats", false);



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
		unsigned long long score1 = (unsigned long long)std::max(a.pp * 100.0f, 0.0f);
		unsigned long long score2 = (unsigned long long)std::max(b.pp * 100.0f, 0.0f);

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
		if (score1 == score2 || a.isLegacyScore != b.isLegacyScore) // force for type discrepancies (legacy scores don't contain pp data)
			return a.sortHack > b.sortHack;

		return score1 > score2;
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
			// HACKHACK: delete all previously loaded beatmaps here (must do this from the main thread due to textures etc.)
			// this is an absolutely disgusting fix
			// originally this was done in m_db->loadDB(db), but that was not run on the main thread and crashed on linux
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

		// check if osu database exists, load file completely
		UString filePath = osu_folder.getString();
		filePath.append("osu!.db");
		OsuFile *db = new OsuFile(filePath);

		// load database
		if (db->isReady() && osu_database_enabled.getBool())
		{
			m_bNeedCleanup = true;
			m_toCleanup = m_db->m_databaseBeatmaps;
			m_db->m_databaseBeatmaps.clear();

			m_db->m_fLoadingProgress = 0.25f;
			m_db->loadDB(db, m_bNeedRawLoad);
		}
		else
			m_bNeedRawLoad = true;

		// cleanup
		SAFE_DELETE(db);

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
	m_sPlayerName = "";

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

void OsuDatabase::reset()
{
	m_collections.clear();
	for (int i=0; i<m_databaseBeatmaps.size(); i++)
	{
		delete m_databaseBeatmaps[i];
	}
	m_databaseBeatmaps.clear();

	m_bIsFirstLoad = true;
	m_bFoundChanges = true;

	m_osu->getNotificationOverlay()->addNotification("Rebuilding.", 0xff00ff00);
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
				m_fLoadingProgress = 1.0f;
				m_importTimer->update();
				debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_databaseBeatmaps.size(), m_importTimer->getElapsedTime());
				break;
			}

			t.update();
		}
	}
}

void OsuDatabase::load()
{
	m_bInterruptLoad = false;
	m_fLoadingProgress = 0.0f;

	// NOTE: clear all active dependencies before deleting the current database
	m_osu->getBackgroundStarCalcHandler()->interruptAndEvict();

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
					if (s.version <= 20180722)
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
		names.push_back(UString(k.c_str()));
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
					foundValidScore = true;

					totalScore += score.score;

					score.sortHack = m_iSortHackCounter++;
					score.md5hash = key;

					if (score.pp > prevPP || prevPP < 0.0f)
					{
						prevPP = score.pp;
						tempScore = &score;
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
	for (int i=0; i<ps.ppScores.size(); i++)
	{
		const float weight = getWeightForIndex(ps.ppScores.size()-1-i);

		pp += ps.ppScores[i]->pp * weight;
		acc += OsuScore::calculateAccuracy(ps.ppScores[i]->num300s, ps.ppScores[i]->num100s, ps.ppScores[i]->num50s, ps.ppScores[i]->numMisses) * weight;
	}

	if (ps.ppScores.size() > 0)
		acc /= (20.0f * (1.0f - getWeightForIndex(ps.ppScores.size()))); // normalize accuracy

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
	return std::pow(0.95f, (float)i);
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

OsuDatabaseBeatmap *OsuDatabase::getBeatmap(std::string md5hash)
{
	for (int i=0; i<m_databaseBeatmaps.size(); i++)
	{
		OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
		const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
		for (int d=0; d<diffs.size(); d++)
		{
			const OsuDatabaseBeatmap *diff = diffs[d];

			bool uuidMatches = (diff->getMD5Hash().length() > 0 && diff->getMD5Hash().length() == md5hash.length());
			for (int u=0; u<32 && u<diff->getMD5Hash().length() && u<md5hash.length(); u++)
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

OsuDatabaseBeatmap *OsuDatabase::getBeatmapDifficulty(std::string md5hash)
{
	// TODO: optimize db accesses by caching a hashmap from md5hash -> OsuBeatmap*, currently it just does a loop over all diffs of all beatmaps (for every call)
	for (int i=0; i<m_databaseBeatmaps.size(); i++)
	{
		OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
		const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
		for (int d=0; d<diffs.size(); d++)
		{
			OsuDatabaseBeatmap *diff = diffs[d];

			bool uuidMatches = (diff->getMD5Hash().length() > 0);
			for (int u=0; u<32 && u<diff->getMD5Hash().length(); u++)
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
	m_sPlayerName = db->readString();
	m_iNumBeatmapsToLoad = db->readInt();

	debugLog("Database: version = %i, folderCount = %i, playerName = %s, numDiffs = %i\n", m_iVersion, m_iFolderCount, m_sPlayerName.toUtf8(), m_iNumBeatmapsToLoad);

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

		//debugLog("Database: Entry #%i: size = %u, artist = %s, songtitle = %s, creator = %s, diff = %s, audiofilename = %s, md5hash = %s, osufilename = %s\n", i, size, artistName.toUtf8(), songTitle.toUtf8(), creatorName.toUtf8(), difficultyName.toUtf8(), audioFileName.toUtf8(), md5hash.toUtf8(), osuFileName.toUtf8());
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
			double starRating = db->readDouble();
			//debugLog("%f stars for %u\n", starRating, mods);

			if (mods == 0)
				numOsuStandardStars = starRating;
		}

		unsigned int numTaikoStarRatings = db->readInt();
		//debugLog("%i star ratings for taiko\n", numTaikoStarRatings);
		for (int s=0; s<numTaikoStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		unsigned int numCtbStarRatings = db->readInt();
		//debugLog("%i star ratings for ctb\n", numCtbStarRatings);
		for (int s=0; s<numCtbStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		unsigned int numManiaStarRatings = db->readInt();
		//debugLog("%i star ratings for mania\n", numManiaStarRatings);
		for (int s=0; s<numManiaStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		/*unsigned int drainTime = */db->readInt(); // seconds
		int duration = db->readInt(); // milliseconds
		duration = duration >= 0 ? duration : 0; // sanity clamp
		int previewTime = db->readInt();
		previewTime = previewTime >= 0 ? previewTime : 0; // sanity clamp

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

		// build beatmap & diffs from all the data
		UString beatmapPath = songFolder;
		beatmapPath.append(path);
		beatmapPath.append("/");
		UString fullFilePath = beatmapPath;
		fullFilePath.append(osuFileName);

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
				diff2->m_fStarsNomod = numOsuStandardStars;

				// calculate bpm range
				float minBeatLength = 0;
				float maxBeatLength = std::numeric_limits<float>::max();
				for (int t=0; t<timingPoints.size(); t++)
				{
					if (timingPoints[t].msPerBeat >= 0)
					{
						if (timingPoints[t].msPerBeat > minBeatLength)
							minBeatLength = timingPoints[t].msPerBeat;
						if (timingPoints[t].msPerBeat < maxBeatLength)
							maxBeatLength = timingPoints[t].msPerBeat;
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

	// load collection.db
	UString collectionFilePath = osu_folder.getString();
	collectionFilePath.append("collection.db");
	OsuFile collectionFile(collectionFilePath);
	if (collectionFile.isReady())
	{
		struct RawCollection
		{
			UString name;
			std::vector<std::string> hashes;
		};

		const int version = collectionFile.readInt();
		const int numCollections = collectionFile.readInt();

		debugLog("Collection: version = %i, numCollections = %i\n", version, numCollections);

		if (version > osu_database_version.getInt() && !osu_database_ignore_version.getBool())
			m_osu->getNotificationOverlay()->addNotification(UString::format("collection.db version unknown (%i),  skipping loading.", version), 0xffffff00, false, 5.0f);

		if (version <= osu_database_version.getInt() || osu_database_ignore_version.getBool())
		{
			for (int i=0; i<numCollections; i++)
			{
				if (m_bInterruptLoad.load()) break; // cancellation point

				m_fLoadingProgress = 0.75f + 0.24f*((float)(i+1)/(float)numCollections);

				UString name = collectionFile.readString();
				int numBeatmaps = collectionFile.readInt();

				RawCollection rc;
				rc.name = name;

				if (Osu::debug->getBool())
					debugLog("Raw Collection #%i: name = %s, numBeatmaps = %i\n", i, name.toUtf8(), numBeatmaps);

				for (int b=0; b<numBeatmaps; b++)
				{
					if (m_bInterruptLoad.load()) break; // cancellation point

					std::string md5hash = collectionFile.readStdString();
					rc.hashes.push_back(md5hash);
				}

				if (rc.hashes.size() > 0)
				{
					// collect OsuBeatmaps corresponding to this collection
					Collection c;
					c.name = rc.name;

					// go through every hash of the collection
					std::vector<OsuDatabaseBeatmap*> matchingDiffs2;
					for (int h=0; h<rc.hashes.size(); h++)
					{
						if (m_bInterruptLoad.load()) break; // cancellation point

						// new: use hashmap
						if (rc.hashes[h].length() == 32)
						{
							const auto result = hashToDiff2.find(rc.hashes[h]);
							if (result != hashToDiff2.end())
								matchingDiffs2.push_back(result->second);
						}
					}

					// we now have an array of all OsuBeatmapDifficulty objects within this collection

					// go through every found OsuBeatmapDifficulty
					for (int md=0; md<matchingDiffs2.size(); md++)
					{
						if (m_bInterruptLoad.load()) break; // cancellation point

						OsuDatabaseBeatmap *diff2 = matchingDiffs2[md];

						// find the OsuBeatmap object corresponding to this diff
						OsuDatabaseBeatmap *beatmap = NULL;
						if (diff2->getMD5Hash().length() == 32)
						{
							// new: use hashmap
							const auto result = hashToBeatmap.find(diff2->getMD5Hash());
							if (result != hashToBeatmap.end())
								beatmap = result->second;
						}

						if (beatmap != NULL)
						{
							// we now have one matching OsuBeatmap and OsuBeatmapDifficulty, add either of them if they don't exist yet
							bool beatmapIsAlreadyInCollection = false;
							for (int m=0; m<c.beatmaps.size(); m++)
							{
								if (m_bInterruptLoad.load()) break; // cancellation point

								if (c.beatmaps[m].first == beatmap)
								{
									beatmapIsAlreadyInCollection = true;

									// the beatmap already exists, check if we have to add the current diff
									bool diffIsAlreadyInCollection = false;
									for (int d=0; d<c.beatmaps[m].second.size(); d++)
									{
										if (m_bInterruptLoad.load()) break; // cancellation point

										if (c.beatmaps[m].second[d] == diff2)
										{
											diffIsAlreadyInCollection = true;
											break;
										}
									}

									// add diff
									if (!diffIsAlreadyInCollection && diff2 != NULL)
										c.beatmaps[m].second.push_back(diff2);

									break;
								}
							}

							// add beatmap
							if (!beatmapIsAlreadyInCollection && diff2 != NULL)
							{
								std::vector<OsuDatabaseBeatmap*> diffs2;
								diffs2.push_back(diff2);
								c.beatmaps.push_back(std::pair<OsuDatabaseBeatmap*, std::vector<OsuDatabaseBeatmap*>>(beatmap, diffs2));
							}
						}
					}

					// add the collection
					if (c.beatmaps.size() > 0) // sanity check
						m_collections.push_back(c);
				}
			}
		}

		if (Osu::debug->getBool())
		{
			for (int i=0; i<m_collections.size(); i++)
			{
				debugLog("Collection #%i: name = %s, numBeatmaps = %i\n", i, m_collections[i].name.toUtf8(), m_collections[i].beatmaps.size());
			}
		}
	}
	else
		debugLog("OsuBeatmapDatabase::loadDB() : Couldn't load collection.db");

	// signal that we are done
	m_fLoadingProgress = 1.0f;
}

void OsuDatabase::loadScores()
{
	if (m_bScoresLoaded) return;

	debugLog("OsuDatabase::loadScores()\n");

	// reset
	m_scores.clear();

	// load legacy osu scores
	if (osu_scores_legacy_enabled.getBool())
	{
		int scoreCounter = 0;

		UString scoresPath = osu_folder.getString();
		scoresPath.append("scores.db");
		OsuFile db(scoresPath, false);
		if (db.isReady())
		{
			const int dbVersion = db.readInt();
			const int numBeatmaps = db.readInt();

			debugLog("Legacy scores: version = %i, numBeatmaps = %i\n", dbVersion, numBeatmaps);

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
		            if (scoreVersion >= 20140721)
		            	/*onlineScoreID = */db.readLongLong();
		            else if (scoreVersion >= 20121008)
		            	/*onlineScoreID = */db.readInt();

		            if (mods & OsuReplay::Mods::Target)
		            	/*double totalAccuracy = */db.readDouble();

		            if (gamemode == 0x0) // gamemode filter (osu!standard)
					{
						Score sc;

						sc.isLegacyScore = true;
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

						// temp
						sc.sortHack = m_iSortHackCounter++;

						addScoreRaw(md5hash, sc);
						scoreCounter++;
					}
				}
			}
			debugLog("Loaded %i individual scores.\n", scoreCounter);
		}
		else
			debugLog("No legacy scores found.\n");
	}

	// load custom scores
	if (osu_scores_custom_enabled.getBool())
	{
		OsuFile db((m_osu->isInVRMode() ? "scoresvr.db" : "scores.db"), false);
		if (db.isReady())
		{
			int scoreCounter = 0;

			const int dbVersion = db.readInt();
			const int numBeatmaps = db.readInt();

			debugLog("Custom scores: version = %i, numBeatmaps = %i\n", dbVersion, numBeatmaps);

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
					const unsigned char gamemode = db.readByte();
					const int scoreVersion = db.readInt();
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

		            if (gamemode == 0x0) // gamemode filter (osu!standard)
					{
						Score sc;

						sc.isLegacyScore = false;
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
						sc.CS = CS; sc.AR = AR; sc.OD = OD; sc.HP = HP;
						sc.maxPossibleCombo = maxPossibleCombo;
						sc.numHitObjects = numHitObjects;
						sc.numCircles = numCircles;
						sc.experimentalModsConVars = experimentalMods;

						// temp
						sc.sortHack = m_iSortHackCounter++;

						addScoreRaw(md5hash, sc);
						scoreCounter++;
					}
				}
			}
			debugLog("Loaded %i individual scores.\n", scoreCounter);
		}
		else
			debugLog("No custom scores found.\n");
	}

	if (m_scores.size() > 0)
		m_bScoresLoaded = true;
}

void OsuDatabase::saveScores()
{
	if (!m_bDidScoresChangeForSave) return;
	m_bDidScoresChangeForSave = false;

	const int dbVersion = 20190103;

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
							db.writeByte(0); // gamemode (hardcoded atm)
							db.writeInt(it->second[i].version);
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

OsuDatabaseBeatmap *OsuDatabase::loadRawBeatmap(UString beatmapPath)
{
	if (Osu::debug->getBool())
		debugLog("OsuBeatmapDatabase::loadRawBeatmap() : %s\n", beatmapPath.toUtf8());

	// try loading all diffs
	std::vector<OsuDatabaseBeatmap*> diffs2;
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

			// try to load it. if successful, save it, else cleanup and continue to the next osu file
			engine->getResourceManager()->loadResource(diff2);
			if (!diff2->isReady())
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

			diffs2.push_back(diff2);
		}
	}

	// if we found any valid diffs, create beatmap
	return (diffs2.size() > 0 ? new OsuDatabaseBeatmap(m_osu, diffs2) : NULL);
}
