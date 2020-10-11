//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap file loader and container (also generates hitobjects)
//
// $NoKeywords: $osudiff
//===============================================================================//

#ifndef OSUBEATMAPDIFFICULTY_H
#define OSUBEATMAPDIFFICULTY_H

#include "cbase.h"

#include "OsuDifficultyCalculator.h"

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
	static ConVar *m_osu_slider_end_inside_check_offset;
	static ConVar *m_osu_stars_xexxar_angles_sliders;
	static ConVar *m_osu_slider_curve_max_length;

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
		bool timingChange;
		bool kiai;
		unsigned long long sortHack;
	};

	// custom
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
	OsuBeatmapDifficulty(Osu *osu, UString filepath, UString folder);
	~OsuBeatmapDifficulty();
	void unload();

	bool loadMetadataRaw(bool calculateStars = false, bool calculateStarsInaccurately = false);
	bool loadRaw(OsuBeatmap *beatmap, std::vector<OsuHitObject*> *hitobjects);

	void loadBackgroundImage();
	void unloadBackgroundImage();
	void loadBackgroundImagePath();

	bool loaded;

	// metadata
	int version;	// e.g. "osu file format v12" -> 12
	int mode;		// 0 = osu!standard, 1 = Taiko, 2 = Catch the Beat, 3 = osu!mania

	UString title;
	UString audioFileName;
	unsigned long lengthMS;

	float stackLeniency;

	UString artist;
	UString creator;
	UString name;	// difficulty name ("Version")
	UString source;
	UString tags;
	std::string md5hash;
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
	int numCircles;
	int numSliders;
	float starsNoMod;
	int ID;
	int setID;
	bool starsWereCalculatedAccurately;
	std::atomic<bool> semaphore; // yes, I know this is disgusting

	// timing (points) + breaks
	TIMING_INFO getTimingInfoForTime(unsigned long positionMS);

	// for score v2
	inline int getMaxCombo() {return m_iMaxCombo;}
	inline unsigned long long getScoreV2ComboPortionMaximum() {return m_fScoreV2ComboPortionMaximum;}

	// star calculation
	// NOTE: calculateStarsInaccurately is only used for initial songbrowser sorting/calculation: as soon as a beatmap is selected it will be recalculated fully (together with the background image loader)
	std::vector<std::shared_ptr<OsuDifficultyHitObject>> generateDifficultyHitObjectsForBeatmap(OsuBeatmap *beatmap, bool calculateStarsInaccurately = false);
	double calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, int upToObjectIndex = -1, bool calculateStarsInaccurately = false, std::vector<double> *outAimStrains = NULL, std::vector<double> *outSpeedStrains = NULL);

	// for live pp
	void rebuildStarCacheForUpToHitObjectIndex(OsuBeatmap *beatmap, std::atomic<bool> &kys, std::atomic<int> &progress);
	inline double getAimStarsForUpToHitObjectIndex(int upToHitObjectIndex) {return (m_aimStarsForNumHitObjects.size() > 0 ? m_aimStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_aimStarsForNumHitObjects.size()-1)] : 0);}
	inline double getSpeedStarsForUpToHitObjectIndex(int upToHitObjectIndex) {return (m_speedStarsForNumHitObjects.size() > 0 ? m_speedStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_speedStarsForNumHitObjects.size()-1)] : 0);}
	/*
	inline const std::vector<double> &getAimStrains() const {return m_aimStrains;}
	inline const std::vector<double> &getSpeedStrains() const {return m_speedStrains;}
	*/

private:
	// every supported type of beatmap/gamemode gets its own build function here. it should build the hitobject classes from the data loaded from disk.
	void buildStandardHitObjects(OsuBeatmapStandard *beatmap, std::vector<OsuHitObject*> *hitobjects);
	void buildManiaHitObjects(OsuBeatmapMania *beatmap, std::vector<OsuHitObject*> *hitobjects);
	//void buildTaikoHitObjects(OsuBeatmapTaiko *beatmap, std::vector<OsuHitObject*> *hitobjects);

	// generic helper functions
	void calculateAllSliderTimesAndClicksTicks();

	float getSliderTickDistance();
	float getSliderTimeForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo);
	float getSliderVelocity(const SLIDER &slider, const TIMING_INFO &timingInfo);
	float getTimingPointMultiplierForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo); // needed for slider ticks

	Osu *m_osu;

	UString m_sFilePath;	// path to .osu file
	UString m_sFolder;		// path to folder containing .osu file

	// for pp & star & score calculation
	int m_iMaxCombo;
	std::vector<double> m_aimStarsForNumHitObjects;
	std::vector<double> m_speedStarsForNumHitObjects;
	/*
	std::vector<double> m_aimStrains;
	std::vector<double> m_speedStrains;
	*/
	unsigned long long m_fScoreV2ComboPortionMaximum;
};

#endif
