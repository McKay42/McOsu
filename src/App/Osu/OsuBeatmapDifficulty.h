//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty file loader and container
//
// $NoKeywords: $osudiff
//===============================================================================//

#ifndef OSUBEATMAPDIFFICULTY_H
#define OSUBEATMAPDIFFICULTY_H

#include "cbase.h"

class Osu;
class OsuHitObject;
class OsuBeatmap;

class BackgroundImagePathLoader;

class OsuBeatmapDifficulty
{
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
		char type;
		int repeat;
		float pixelLength;
		long time;
		int sampleType;
		int number;
		int colorCounter;
		std::vector<Vector2> points;

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
	int mode; // 0 = osu!, 1 = Taiko, 2 = Catch the Beat, 3 = osu!mania

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

	// objects
	std::vector<HITCIRCLE> hitcircles;
	std::vector<SLIDER> sliders;
	std::vector<SPINNER> spinners;
	std::vector<TIMINGPOINT> timingpoints;
	std::vector<Color> combocolors;

	// custom
	UString fullSoundFilePath;
	Image *backgroundImage;
	long localoffset;
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

private:
	float getSliderTimeForSlider(SLIDER *slider);
	float getTimingPointMultiplierForSlider(SLIDER *slider); // needed for slider ticks

	Osu *m_osu;

	UString m_sFilePath;
	UString m_sFolder;

	// custom
	BackgroundImagePathLoader *m_backgroundImagePathLoader;
};

#endif
