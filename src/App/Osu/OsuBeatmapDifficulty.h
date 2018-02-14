//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap file loader and container (also generates hitobjects)
//
// $NoKeywords: $osudiff
//===============================================================================//

#ifndef OSUBEATMAPDIFFICULTY_H
#define OSUBEATMAPDIFFICULTY_H

#include "cbase.h"

class BackgroundImagePathLoader;

class Osu;
class OsuBeatmap;
class OsuBeatmapStandard;
class OsuBeatmapMania;

class OsuHitObject;

class OsuBeatmapDifficulty
{
public:
	static ConVar *m_osu_slider_scorev2;
	static ConVar *m_osu_draw_statistics_pp;
	static ConVar *m_osu_debug_pp;
	static ConVar *m_osu_database_dynamic_star_calculation;

public:
	OsuBeatmapDifficulty(Osu *osu, UString filepath, UString folder);
	~OsuBeatmapDifficulty();
	void unload();

	bool loadMetadataRaw(bool calculateStars = false);
	bool loadRaw(OsuBeatmap *beatmap, std::vector<OsuHitObject*> *hitobjects);

	void loadBackgroundImage();
	void unloadBackgroundImage();
	void loadBackgroundImagePath();

	inline unsigned long long getSortHack() const {return m_iSortHack;}
	inline bool shouldBackgroundImageBeLoaded() const {return m_bShouldBackgroundImageBeLoaded;}
	bool isBackgroundLoaderActive();

	struct HITCIRCLE
	{
		int x,y;
		unsigned long time;
		int sampleType;
		int number;
		int colorCounter;
		bool clicked;
		long maniaEndTime;
	};

	struct SLIDER_CLICK
	{
		long time;
		bool clicked;
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
		std::vector<Vector2> points;
		std::vector<int> hitSounds;

		float sliderTime;
		float sliderTimeWithoutRepeats;
		std::vector<float> ticks;
		std::vector<SLIDER_CLICK> clicked;
		std::vector<SLIDER_CLICK> ticked;
	};

	struct SPINNER
	{
		int x,y;
		unsigned long time;
		int sampleType;
		unsigned long endTime;
	};

	struct BREAK
	{
		int startTime;
		int endTime;
	};

	struct TIMINGPOINT
	{
		long offset;
		float msPerBeat;
		int sampleType;
		int sampleSet;
		int volume;
		bool kiai;
		unsigned long long sortHack;
	};

	bool loaded;

	// metadata
	int mode; // 0 = osu!standard, 1 = Taiko, 2 = Catch the Beat, 3 = osu!mania

	UString title;
	UString audioFileName;
	unsigned long lengthMS;

	float stackLeniency;

	UString artist;
	UString creator;
	UString name; // difficulty name ("Version")
	UString source;
	UString tags;
	UString md5hash;
	long beatmapId;

	float AR;
	float CS;
	float HP;
	float OD;
	float sliderTickRate;
	float sliderMultiplier;

	UString backgroundImageName;

	unsigned long previewTime;
	long long lastModificationTime;

	// objects
	std::vector<HITCIRCLE> hitcircles;
	std::vector<SLIDER> sliders;
	std::vector<SPINNER> spinners;
	std::vector<BREAK> breaks;
	std::vector<TIMINGPOINT> timingpoints;
	std::vector<Color> combocolors;

	// custom
	UString fullSoundFilePath;
	Image *backgroundImage;
	long localoffset;
	long onlineOffset;
	int minBPM;
	int maxBPM;
	int numObjects;
	float starsNoMod;
	int ID;
	int setID;

	// timing (points) + breaks
	struct TIMING_INFO
	{
		long offset;
		float beatLengthBase;
		float beatLength;
		float volume;
		int sampleType;
		int sampleSet;
	};

	TIMING_INFO getTimingInfoForTime(unsigned long positionMS);
	unsigned long getBreakDuration(unsigned long positionMS);

	bool isInBreak(unsigned long positionMS);

	// pp & star calculation
	enum class SCORE_VERSION
	{
		SCORE_V1,
		SCORE_V2
	};
	enum class PP_HITOBJECT_TYPE : char
	{
		invalid = 0,
		circle,
		spinner,
		slider,
	};
	struct PPHitObject
	{
		Vector2 pos;
		long time = 0;
		PP_HITOBJECT_TYPE type = PP_HITOBJECT_TYPE::invalid;
		long end_time = 0; // for spinners and sliders
		unsigned long long sortHack = 0;
		//slider_data slider;
	};
	void rebuildStarCacheForUpToHitObjectIndex(OsuBeatmap *beatmap, std::atomic<bool> &kys, std::atomic<int> &progress);
	std::vector<PPHitObject> generatePPHitObjectsForBeatmap(OsuBeatmap *beatmap);
	static double calculateStarDiffForHitObjects(std::vector<PPHitObject> &hitObjects, float CS, double *aim, double *speed, int upToObjectIndex = -1);
	double calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, int upToObjectIndex = -1);
	static double calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0/*, SCORE_VERSION scoreVersion = SCORE_VERSION::SCORE_V1*/);
	static double calculatePPv2Acc(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, double acc, int numHitObjects, int numCircles, int maxPossibleCombo, int combo = -1, int misses = 0/*, SCORE_VERSION scoreVersion = SCORE_VERSION::SCORE_V1*/);
	static double calculateAcc(int c300, int c100, int c50, int misses);
	static double calculateBaseStrain(double strain);

	inline int getMaxCombo() {return m_iMaxCombo;}
	inline unsigned long long getScoreV2ComboPortionMaximum() {return m_fScoreV2ComboPortionMaximum;}
	inline double getAimStarsForUpToHitObjectIndex(int upToHitObjectIndex) {return (m_aimStarsForNumHitObjects.size() > 0 ? m_aimStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_aimStarsForNumHitObjects.size()-1)] : 0);}
	inline double getSpeedStarsForUpToHitObjectIndex(int upToHitObjectIndex) {return (m_speedStarsForNumHitObjects.size() > 0 ? m_speedStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_speedStarsForNumHitObjects.size()-1)] : 0);}

private:
	static unsigned long long sortHackCounter;

	friend class BackgroundImagePathLoader;

	// every supported type of beatmap/gamemode gets its own build function here. it should build the hitobject classes from the data loaded from disk.
	void buildStandardHitObjects(OsuBeatmapStandard *beatmap, std::vector<OsuHitObject*> *hitobjects);
	void buildManiaHitObjects(OsuBeatmapMania *beatmap, std::vector<OsuHitObject*> *hitobjects);
	// void buildTaikoHitObjects(OsuBeatmapTaiko *beatmap, std::vector<OsuHitObject*> *hitobjects);

	// generic helper functions
	float getSliderTickDistance();
	float getSliderTimeForSlider(SLIDER *slider);
	float getSliderVelocity(SLIDER *slider);
	float getTimingPointMultiplierForSlider(SLIDER *slider); // needed for slider ticks

	void deleteBackgroundImagePathLoader();

	Osu *m_osu;

	UString m_sFilePath;
	UString m_sFolder;

	// custom
	bool m_bShouldBackgroundImageBeLoaded;
	BackgroundImagePathLoader *m_backgroundImagePathLoader;
	unsigned long long m_iSortHack;

	// for pp & star & score calculation
	int m_iMaxCombo;
	std::vector<double> m_aimStarsForNumHitObjects;
	std::vector<double> m_speedStarsForNumHitObjects;
	unsigned long long m_fScoreV2ComboPortionMaximum;
};

#endif
