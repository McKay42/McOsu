//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu!.db + collection.db + raw loader + scores etc.
//
// $NoKeywords: $osubdb
//===============================================================================//

#ifndef OSUDATABASE_H
#define OSUDATABASE_H

#include "cbase.h"

class Timer;
class ConVar;

class Osu;
class OsuFile;
class OsuDatabaseBeatmap;

class OsuDatabaseLoader;

class OsuDatabase
{
public:
	struct Collection
	{
		UString name;
		std::vector<std::pair<OsuDatabaseBeatmap*, std::vector<OsuDatabaseBeatmap*>>> beatmaps;
	};

	struct Score
	{
		bool isLegacyScore;
		bool isImportedLegacyScore; // used for identifying imported osu! scores (which were previously legacy scores, so they don't have any numSliderBreaks/unstableRate/hitErrorAvgMin/hitErrorAvgMax)
		int version;
		uint64_t unixTimestamp;

		// default
		UString playerName;

		int num300s;
		int num100s;
		int num50s;
		int numGekis;
		int numKatus;
		int numMisses;

		unsigned long long score;
		int comboMax;
		bool perfect;
		int modsLegacy;

		// custom
		int numSliderBreaks;
		float pp;
		float unstableRate;
		float hitErrorAvgMin;
		float hitErrorAvgMax;
		float starsTomTotal;
		float starsTomAim;
		float starsTomSpeed;
		float speedMultiplier;
		float CS, AR, OD, HP;
		int maxPossibleCombo;
		int numHitObjects;
		int numCircles;
		UString experimentalModsConVars;

		// runtime
		unsigned long long sortHack;
		std::string md5hash;

		bool isLegacyScoreEqualToImportedLegacyScore(const OsuDatabase::Score &importedLegacyScore) const
		{
			if (!isLegacyScore) return false;
			if (!importedLegacyScore.isImportedLegacyScore) return false;

			const bool isScoreValueEqual = (score == importedLegacyScore.score);
			const bool isTimestampEqual = (unixTimestamp == importedLegacyScore.unixTimestamp);
			const bool isComboMaxEqual = (comboMax == importedLegacyScore.comboMax);
			const bool isModsLegacyEqual = (modsLegacy == importedLegacyScore.modsLegacy);
			const bool isNum300sEqual = (num300s == importedLegacyScore.num300s);
			const bool isNum100sEqual = (num100s == importedLegacyScore.num100s);
			const bool isNum50sEqual = (num50s == importedLegacyScore.num50s);
			const bool isNumGekisEqual = (numGekis == importedLegacyScore.numGekis);
			const bool isNumKatusEqual = (numKatus == importedLegacyScore.numKatus);
			const bool isNumMissesEqual = (numMisses == importedLegacyScore.numMisses);

			return (isScoreValueEqual
				 && isTimestampEqual
				 && isComboMaxEqual
				 && isModsLegacyEqual
				 && isNum300sEqual
				 && isNum100sEqual
				 && isNum50sEqual
				 && isNumGekisEqual
				 && isNumKatusEqual
				 && isNumMissesEqual);
		}

		bool isScoreEqualToCopiedScoreIgnoringPlayerName(const OsuDatabase::Score &copiedScore) const
		{
			const bool isScoreValueEqual = (score == copiedScore.score);
			const bool isTimestampEqual = (unixTimestamp == copiedScore.unixTimestamp);
			const bool isComboMaxEqual = (comboMax == copiedScore.comboMax);
			const bool isModsLegacyEqual = (modsLegacy == copiedScore.modsLegacy);
			const bool isNum300sEqual = (num300s == copiedScore.num300s);
			const bool isNum100sEqual = (num100s == copiedScore.num100s);
			const bool isNum50sEqual = (num50s == copiedScore.num50s);
			const bool isNumGekisEqual = (numGekis == copiedScore.numGekis);
			const bool isNumKatusEqual = (numKatus == copiedScore.numKatus);
			const bool isNumMissesEqual = (numMisses == copiedScore.numMisses);

			const bool isSpeedMultiplierEqual = (speedMultiplier == copiedScore.speedMultiplier);
			const bool isCSEqual = (CS == copiedScore.CS);
			const bool isAREqual = (AR == copiedScore.AR);
			const bool isODEqual = (OD == copiedScore.OD);
			const bool isHPEqual = (HP == copiedScore.HP);
			const bool areExperimentalModsConVarsEqual = (experimentalModsConVars == copiedScore.experimentalModsConVars);

			return (isScoreValueEqual
				 && isTimestampEqual
				 && isComboMaxEqual
				 && isModsLegacyEqual
				 && isNum300sEqual
				 && isNum100sEqual
				 && isNum50sEqual
				 && isNumGekisEqual
				 && isNumKatusEqual
				 && isNumMissesEqual

				 && isSpeedMultiplierEqual
				 && isCSEqual
				 && isAREqual
				 && isODEqual
				 && isHPEqual
				 && areExperimentalModsConVarsEqual);
		}
	};

	struct PlayerStats
	{
		UString name;
		float pp;
		float accuracy;
		int numScoresWithPP;
		int level;
		float percentToNextLevel;
		unsigned long long totalScore;
	};

	struct PlayerPPScores
	{
		std::vector<Score*> ppScores;
		unsigned long long totalScore;
	};

	struct SCORE_SORTING_COMPARATOR
	{
		virtual ~SCORE_SORTING_COMPARATOR() {;}
		virtual bool operator() (OsuDatabase::Score const &a, OsuDatabase::Score const &b) const = 0;
	};

	struct SCORE_SORTING_METHOD
	{
		UString name;
		SCORE_SORTING_COMPARATOR *comparator;
	};

public:
	OsuDatabase(Osu *osu);
	~OsuDatabase();

	void reset();

	void update();

	void load();
	void cancel();
	void save();

	OsuDatabaseBeatmap *addBeatmap(UString beatmapFolderPath);

	int addScore(std::string beatmapMD5Hash, OsuDatabase::Score score);
	void deleteScore(std::string beatmapMD5Hash, uint64_t scoreUnixTimestamp);
	void sortScores(std::string beatmapMD5Hash);
	void forceScoreUpdateOnNextCalculatePlayerStats() {m_bDidScoresChangeForStats = true;}
	void forceScoresSaveOnNextShutdown() {m_bDidScoresChangeForSave = true;}

	std::vector<UString> getPlayerNamesWithPPScores();
	std::vector<UString> getPlayerNamesWithScoresForUserSwitcher();
	PlayerPPScores getPlayerPPScores(UString playerName);
	PlayerStats calculatePlayerStats(UString playerName);
	static float getWeightForIndex(int i);
	static float getBonusPPForNumScores(int numScores);
	unsigned long long getRequiredScoreForLevel(int level);
	int getLevelForScore(unsigned long long score, int maxLevel = 120);

	inline float getProgress() const {return m_fLoadingProgress.load();}
	inline bool isFinished() const {return (getProgress() >= 1.0f);}
	inline bool foundChanges() const {return m_bFoundChanges;}

	inline const std::vector<OsuDatabaseBeatmap*> getDatabaseBeatmaps() const {return m_databaseBeatmaps;}
	OsuDatabaseBeatmap *getBeatmap(const std::string &md5hash);
	OsuDatabaseBeatmap *getBeatmapDifficulty(const std::string &md5hash);
	inline int getNumCollections() const {return m_collections.size();}
	inline const std::vector<Collection> getCollections() const {return m_collections;}

	inline std::unordered_map<std::string, std::vector<Score>> *getScores() {return &m_scores;}
	inline const std::vector<SCORE_SORTING_METHOD> &getScoreSortingMethods() const {return m_scoreSortingMethods;}

	inline unsigned long long getAndIncrementScoreSortHackCounter() {return m_iSortHackCounter++;}

private:
	friend class OsuDatabaseLoader;

	static ConVar *m_name_ref;
	static ConVar *m_osu_songbrowser_scores_sortingtype_ref;

	void addScoreRaw(const std::string &beatmapMD5Hash, const OsuDatabase::Score &score);

	UString parseLegacyCfgBeatmapDirectoryParameter();
	void scheduleLoadRaw();
	void loadDB(OsuFile *db, bool &fallbackToRawLoad);

	void loadScores();
	void saveScores();

	void loadCollections(const std::unordered_map<std::string, OsuDatabaseBeatmap*> &hashToDiff2, const std::unordered_map<std::string, OsuDatabaseBeatmap*> &hashToBeatmap);
	void saveCollections();

	OsuDatabaseBeatmap *loadRawBeatmap(UString beatmapPath); // only used for raw loading without db

	void onScoresRename(UString args);

	Osu *m_osu;
	Timer *m_importTimer;
	bool m_bIsFirstLoad;	// only load differences after first raw load
	bool m_bFoundChanges;	// for total refresh detection of raw loading

	// global
	int m_iNumBeatmapsToLoad;
	std::atomic<float> m_fLoadingProgress;
	std::atomic<bool> m_bInterruptLoad;
	std::vector<OsuDatabaseBeatmap*> m_databaseBeatmaps;

	// osu!.db
	int m_iVersion;
	int m_iFolderCount;
	UString m_sPlayerName;

	// collection.db
	std::vector<Collection> m_collections;

	// scores.db (legacy and custom)
	bool m_bScoresLoaded;
	std::unordered_map<std::string, std::vector<Score>> m_scores;
	bool m_bDidScoresChangeForSave;
	bool m_bDidScoresChangeForStats;
	unsigned long long m_iSortHackCounter;
	PlayerStats m_prevPlayerStats;
	std::vector<SCORE_SORTING_METHOD> m_scoreSortingMethods;

	// raw load
	bool m_bRawBeatmapLoadScheduled;
	int m_iCurRawBeatmapLoadIndex;
	UString m_sRawBeatmapLoadOsuSongFolder;
	std::vector<UString> m_rawBeatmapFolders;
	std::vector<UString> m_rawLoadBeatmapFolders;
};

#endif
