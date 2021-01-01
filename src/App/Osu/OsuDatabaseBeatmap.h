//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		loader + container for raw beatmap files/data (v2 rewrite)
//
// $NoKeywords: $osudiff
//===============================================================================//

#ifndef OSUDATABASEBEATMAP_H
#define OSUDATABASEBEATMAP_H

#include "Resource.h"

#include "Osu.h"
#include "OsuDifficultyCalculator.h"

// purpose:
// 1) contain all infos which are ALWAYS kept in memory for beatmaps
// 2) be the data source for OsuBeatmap when starting a difficulty
// 3) allow async calculations/loaders to work on the contained data (e.g. background image loader)
// 4) be a container for difficulties (all top level OsuDatabaseBeatmap objects are containers)

// TODO: move all unnecessary shit into OsuBeatmap, and clear out the garbage
// TODO: star loading (separate class which just uses the m_sFilePath of this resource)
// TODO: break storage + break duration functions, put all of that into OsuBeatmap. now load breaks into beatmap.
// TODO: calc and store m_aimStarsForNumHitObjects in OsuBeatmap.
// TODO: calc and store m_speedStarsForNumHitObjects in OsuBeatmap.
// TODO: while playing, storage of numCircles/sliders/spinners etc.
// TODO: the DatabaseBeatmap class being a Resource feels like a clusterfuck when actually trying to use it, think about improving this structure

class Osu;
class OsuBeatmap;
class OsuHitObject;

class OsuDatabase;

class OsuBackgroundImageHandler;

class OsuDatabaseBeatmap
{
public:
	// raw structs

	struct TIMINGPOINT
	{
		long offset;

		float msPerBeat;

		int sampleType;
		int sampleSet;
		int volume;

		bool timingChange;
		bool kiai;

		unsigned long long sortHack;
	};

	struct BREAK
	{
		int startTime;
		int endTime;
	};



public:
	// custom structs

	struct LOAD_RESULT
	{
		int errorCode;

		std::vector<OsuHitObject*> hitobjects;
		std::vector<BREAK> breaks;
		std::vector<Color> combocolors;

		LOAD_RESULT()
		{
			errorCode = 0;
		}
	};

	struct TIMING_INFO
	{
		long offset;

		float beatLengthBase;
		float beatLength;

		float volume;
		int sampleType;
		int sampleSet;

		bool isNaN;
	};



public:
	OsuDatabaseBeatmap(Osu *osu, UString filePath, UString folder);
	OsuDatabaseBeatmap(Osu *osu, std::vector<OsuDatabaseBeatmap*> &difficulties);
	~OsuDatabaseBeatmap();



	static std::vector<std::shared_ptr<OsuDifficultyHitObject>> loadDifficultyHitObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, float AR, float CS, int version, float stackLeniency, float speedMultiplier, bool calculateStarsInaccurately = false);
	static LOAD_RESULT load(OsuDatabaseBeatmap *databaseBeatmap, OsuBeatmap *beatmap);
	static bool loadMetadata(OsuDatabaseBeatmap *databaseBeatmap);



	void setDifficulties(std::vector<OsuDatabaseBeatmap*> &difficulties);

	void setLocalOffset(long localOffset) {m_iLocalOffset = localOffset;}



	inline Osu *getOsu() {return m_osu;}

	inline UString getFolder() const {return m_sFolder;}
	inline UString getFilePath() const {return m_sFilePath;}

	inline unsigned long long getSortHack() const {return m_iSortHack;}

	inline const std::vector<OsuDatabaseBeatmap*> &getDifficulties() const {return m_difficulties;}

	inline const std::string &getMD5Hash() const {return m_sMD5Hash;}

	TIMING_INFO getTimingInfoForTime(unsigned long positionMS);
	static TIMING_INFO getTimingInfoForTimeAndTimingPoints(unsigned long positionMS, std::vector<TIMINGPOINT> &timingpoints);



public:
	// raw metadata

	inline int getVersion() const {return m_iVersion;}
	inline int getGameMode() const {return m_iGameMode;}
	inline int getID() const {return m_iID;}
	inline int getSetID() const {return m_iSetID;}

	inline const UString &getTitle() const {return m_sTitle;}
	inline const UString &getArtist() const {return m_sArtist;}
	inline const UString &getCreator() const {return m_sCreator;}
	inline const UString &getDifficultyName() const {return m_sDifficultyName;}
	inline const UString &getSource() const {return m_sSource;}
	inline const UString &getTags() const {return m_sTags;}
	inline const UString &getBackgroundImageFileName() const {return m_sBackgroundImageFileName;}
	inline const UString &getAudioFileName() const {return m_sAudioFileName;}

	inline unsigned long getLengthMS() const {return m_iLengthMS;}
	inline unsigned long getPreviewTime() const {return m_iPreviewTime;}

	inline float getAR() const {return m_fAR;}
	inline float getCS() const {return m_fCS;}
	inline float getHP() const {return m_fHP;}
	inline float getOD() const {return m_fOD;}

	inline float getStackLeniency() const {return m_fStackLeniency;}
	inline float getSliderTickRate() const {return m_fSliderTickRate;}
	inline float getSliderMultiplier() const {return m_fSliderMultiplier;}

	inline const std::vector<TIMINGPOINT> &getTimingpoints() const {return m_timingpoints;}



	// redundant data

	inline const UString &getFullSoundFilePath() const {return m_sFullSoundFilePath;}
	inline const UString &getFullBackgroundImageFilePath() const {return m_sFullBackgroundImageFilePath;}



	// precomputed data

	inline float getStarsNomod() const {return m_fStarsNomod;}

	inline int getMinBPM() const {return m_iMinBPM;}
	inline int getMaxBPM() const {return m_iMaxBPM;}

	inline int getNumObjects() const {return m_iNumObjects;}
	inline int getNumCircles() const {return m_iNumCircles;}
	inline int getNumSliders() const {return m_iNumSliders;}
	inline int getNumSpinners() const {return m_iNumSpinners;}



	// custom data

	inline long long getLastModificationTime() const {return m_iLastModificationTime;}

	inline long getLocalOffset() const {return m_iLocalOffset;}
	inline long getOnlineOffset() const {return m_iOnlineOffset;}



private:
	// raw metadata

	int m_iVersion; // e.g. "osu file format v12" -> 12
	int m_iGameMode;// 0 = osu!standard, 1 = Taiko, 2 = Catch the Beat, 3 = osu!mania
	long m_iID;		// online ID, if uploaded
	int m_iSetID;	// online set ID, if uploaded

	UString m_sTitle;
	UString m_sArtist;
	UString m_sCreator;
	UString m_sDifficultyName;	// difficulty name ("Version")
	UString m_sSource;			// only used by search
	UString m_sTags;			// only used by search
	UString m_sBackgroundImageFileName;
	UString m_sAudioFileName;

	unsigned long m_iLengthMS;
	unsigned long m_iPreviewTime;

	float m_fAR;
	float m_fCS;
	float m_fHP;
	float m_fOD;

	float m_fStackLeniency;
	float m_fSliderTickRate;
	float m_fSliderMultiplier;

	std::vector<TIMINGPOINT> m_timingpoints; // necessary for main menu anim



	// redundant data (technically contained in metadata, but precomputed anyway)

	UString m_sFullSoundFilePath;
	UString m_sFullBackgroundImageFilePath;



	// precomputed data (can-run-without-but-nice-to-have data)

	float m_fStarsNomod;

	int m_iMinBPM;
	int m_iMaxBPM;

	int m_iNumObjects;
	int m_iNumCircles;
	int m_iNumSliders;
	int m_iNumSpinners;



	// custom data (not necessary, not part of the beatmap file, and not precomputed)

	long long m_iLastModificationTime; // only used for sorting

	long m_iLocalOffset;
	long m_iOnlineOffset;



private:
	// primitive objects

	struct HITCIRCLE
	{
		int x,y;
		unsigned long time;
		int sampleType;
		int number;
		int colorCounter;
		int colorOffset;
		bool clicked;
		long maniaEndTime;
	};

	struct SLIDER
	{
		int x,y;
		char type;
		int repeat;
		float pixelLength;
		long time;
		int sampleType;
		int number;
		int colorCounter;
		int colorOffset;
		std::vector<Vector2> points;
		std::vector<int> hitSounds;

		float sliderTime;
		float sliderTimeWithoutRepeats;
		std::vector<float> ticks;

		std::vector<long> scoringTimesForStarCalc;
	};

	struct SPINNER
	{
		int x,y;
		unsigned long time;
		int sampleType;
		unsigned long endTime;
	};

	struct PRIMITIVE_CONTAINER
	{
		int errorCode;

		std::vector<HITCIRCLE> hitcircles;
		std::vector<SLIDER> sliders;
		std::vector<SPINNER> spinners;
		std::vector<BREAK> breaks;

		std::vector<TIMINGPOINT> timingpoints;
		std::vector<Color> combocolors;

		float sliderMultiplier;
		float sliderTickRate;

		int maxPossibleCombo;
	};



private:
	// class internal data (custom)

	friend class OsuDatabase;
	friend class OsuBackgroundImageHandler;

	static unsigned long long sortHackCounter;

	static ConVar *m_osu_slider_curve_max_length_ref;
	static ConVar *m_osu_show_approach_circle_on_first_hidden_object_ref;
	static ConVar *m_osu_stars_xexxar_angles_sliders_ref;
	static ConVar *m_osu_stars_stacking_ref;
	static ConVar *m_osu_debug_pp_ref;
	static ConVar *m_osu_slider_end_inside_check_offset_ref;
	static ConVar *m_osu_mod_random_ref;
	static ConVar *m_osu_mod_random_circle_offset_x_percent_ref;
	static ConVar *m_osu_mod_random_circle_offset_y_percent_ref;
	static ConVar *m_osu_mod_random_slider_offset_x_percent_ref;
	static ConVar *m_osu_mod_random_slider_offset_y_percent_ref;
	static ConVar *m_osu_mod_reverse_sliders_ref;
	static ConVar *m_osu_mod_random_spinner_offset_x_percent_ref;
	static ConVar *m_osu_mod_random_spinner_offset_y_percent_ref;



	static PRIMITIVE_CONTAINER loadPrimitiveObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode);
	static void calculateSliderTimesClicksTicks(std::vector<SLIDER> &sliders, std::vector<TIMINGPOINT> &timingpoints, float sliderMultiplier, float sliderTickRate);



	Osu *m_osu;

	UString m_sFolder;		// path to folder containing .osu file (e.g. "/path/to/beatmapfolder/")
	UString m_sFilePath;	// path to .osu file (e.g. "/path/to/beatmapfolder/beatmap.osu")

	unsigned long long m_iSortHack;

	std::vector<OsuDatabaseBeatmap*> m_difficulties;

	std::string m_sMD5Hash;



	// helper functions

	struct TimingPointSortComparator
	{
	    bool operator() (OsuDatabaseBeatmap::TIMINGPOINT const &a, OsuDatabaseBeatmap::TIMINGPOINT const &b) const
	    {
	    	// first condition: offset
	    	// second condition: if offset is the same, non-inherited timingpoints go before inherited timingpoints

	    	// strict weak ordering!
	    	if (a.offset == b.offset && ((a.msPerBeat >= 0 && b.msPerBeat < 0) == (b.msPerBeat >= 0 && a.msPerBeat < 0)))
	    		return a.sortHack < b.sortHack;
	    	else
	    		return (a.offset < b.offset) || (a.offset == b.offset && a.msPerBeat >= 0 && b.msPerBeat < 0);
	    }
	};
};



class OsuDatabaseBeatmapBackgroundImagePathLoader : public Resource
{
public:
	OsuDatabaseBeatmapBackgroundImagePathLoader(const UString &filePath);

	inline const UString &getLoadedBackgroundImageFileName() const {return m_sLoadedBackgroundImageFileName;}

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy() {;}

	UString m_sFilePath;

	UString m_sLoadedBackgroundImageFileName;
};



class OsuDatabaseBeatmapStarCalculator : public Resource
{
public:
	OsuDatabaseBeatmapStarCalculator();

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy() {;}
};

#endif
