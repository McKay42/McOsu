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

class Osu;
class OsuFile;
class OsuBeatmap;
class OsuBeatmapDifficulty;

class OsuDatabaseLoader;

class OsuDatabase
{
public:
	struct Collection
	{
		UString name;
		std::vector<std::pair<OsuBeatmap*, std::vector<OsuBeatmapDifficulty*>>> beatmaps;
	};

	struct Score
	{
		bool isLegacyScore;
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
		UString experimentalModsConVars;

		// temp
		unsigned long long sortHack;
	};

public:
	OsuDatabase(Osu *osu);
	~OsuDatabase();

	void reset();

	void update();

	void load();
	void cancel();
	void save();

	int addScore(std::string beatmapMD5Hash, OsuDatabase::Score score);
	void deleteScore(std::string beatmapMD5Hash, uint64_t scoreUnixTimestamp);
	void sortScores(std::string beatmapMD5Hash);

	inline float getProgress() const {return m_fLoadingProgress.load();}
	bool isFinished() const {return getProgress() >= 1.0f;}
	inline bool foundChanges() const {return m_bFoundChanges;}

	inline int getNumBeatmaps() const {return m_beatmaps.size();} // valid beatmaps
	inline const std::vector<OsuBeatmap*> getBeatmaps() const {return m_beatmaps;}
	inline int getNumCollections() const {return m_collections.size();}
	inline const std::vector<Collection> getCollections() const {return m_collections;}

	inline std::unordered_map<std::string, std::vector<Score>> *getScores() {return &m_scores;}

private:
	friend class OsuDatabaseLoader;

	void loadRaw();
	void loadDB(OsuFile *db);
	void loadScores();
	void saveScores();

	OsuBeatmap *loadRawBeatmap(UString beatmapPath);

	OsuBeatmap *createBeatmapForActiveGamemode(); // TEMP: workaround

	Osu *m_osu;
	Timer *m_importTimer;
	bool m_bIsFirstLoad;
	bool m_bFoundChanges; // for total refresh detection of raw loading

	// global
	int m_iNumBeatmapsToLoad;
	std::atomic<float> m_fLoadingProgress;
	std::atomic<bool> m_bKYS;
	std::vector<OsuBeatmap*> m_beatmaps;

	// osu!.db
	int m_iVersion;
	int m_iFolderCount;
	UString m_sPlayerName;

	// collection.db
	std::vector<Collection> m_collections;

	// scores.db (legacy and custom)
	bool m_bScoresLoaded;
	std::unordered_map<std::string, std::vector<Score>> m_scores;
	bool m_bDidScoresChange;
	unsigned long long m_iSortHackCounter;

	// raw load
	bool m_bRawBeatmapLoadScheduled;
	int m_iCurRawBeatmapLoadIndex;
	UString m_sRawBeatmapLoadOsuSongFolder;
	std::vector<UString> m_rawBeatmapFolders;
	std::vector<UString> m_rawLoadBeatmapFolders;
};

#endif
