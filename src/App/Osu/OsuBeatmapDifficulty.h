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

class OsuHitObject;

class OsuBeatmapDifficulty
{
public:
	static ConVar *m_osu_slider_scorev2;

public:
	OsuBeatmapDifficulty(Osu *osu, UString filepath, UString folder);
	~OsuBeatmapDifficulty();
	void unload();

	bool loadMetadataRaw();
	bool loadRaw(OsuBeatmap *beatmap, std::vector<OsuHitObject*> *hitobjects);

	void loadBackgroundImage();
	void unloadBackgroundImage();
	void loadBackgroundImagePath();

	struct HITCIRCLE
	{
		int x,y;
		unsigned long time;
		int sampleType;
		int number;
		int colorCounter;
		bool clicked;
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

	inline bool shouldBackgroundImageBeLoaded() const {return m_bShouldBackgroundImageBeLoaded;}
	bool isInBreak(unsigned long positionMS);

	// pp & star calculation
	enum class SCORE_VERSION
	{
		SCORE_V1,
		SCORE_V2
	};
	inline int getMaxCombo() {return m_iMaxCombo;}
	double calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, double *rhythm_awkwardness);
	double calculatePPv2(OsuBeatmap *beatmap, double aim, double speed, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0/*, SCORE_VERSION scoreVersion = SCORE_VERSION::SCORE_V1*/);
	static double calculateAcc(int c300, int c100, int c50, int misses);
	static double calculateBaseStrain(double strain);

private:
	friend class BackgroundImagePathLoader;

	// every supported type of beatmap/gamemode gets its own build function here. it should build the hitobject classes from the data loaded from disk.
	void buildStandardHitObjects(OsuBeatmapStandard *beatmap, std::vector<OsuHitObject*> *hitobjects);

	float getSliderTimeForSlider(SLIDER *slider);
	float getTimingPointMultiplierForSlider(SLIDER *slider); // needed for slider ticks

	void deleteBackgroundImagePathLoader();

	Osu *m_osu;

	UString m_sFilePath;
	UString m_sFolder;

	// custom
	bool m_bShouldBackgroundImageBeLoaded;
	BackgroundImagePathLoader *m_backgroundImagePathLoader;

	int m_iMaxCombo;
};

#endif
