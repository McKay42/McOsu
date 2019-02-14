//================ Copyright (c) 2019, PG & Francesco149, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//==============================================================================================//

#ifndef OSUDIFFICULTYCALCULATOR_H
#define OSUDIFFICULTYCALCULATOR_H

#include "cbase.h"

class Osu;
class OsuBeatmap;

class OsuSliderCurve;

class ConVar;

class OsuDifficultyHitObject
{
public:
	enum class TYPE : char
	{
		INVALID = 0,
		CIRCLE,
		SPINNER,
		SLIDER,
	};

public:
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time); // circle
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime); // spinner
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<long> scoringTimes); // slider
	~OsuDifficultyHitObject();

	OsuDifficultyHitObject(const OsuDifficultyHitObject&) = delete;
	OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj);

	void updateStackPosition(float stackOffset);

	Vector2 getOriginalRawPosAt(long pos); // for stacking calculations, always returns the unstacked original position at that point in time

	inline long getDuration() const {return endTime - time;}

	// circles (base)
	TYPE type;
	Vector2 pos;
	long time;

	// spinners + sliders
	long endTime;

	// sliders
	float spanDuration; // i.e. sliderTimeWithoutRepeats
	char osuSliderCurveType;
	float pixelLength;
	std::vector<long> scoringTimes;

	// custom
	OsuSliderCurve *curve;
	int stack;
	Vector2 originalPos;
	unsigned long long sortHack;

private:
	static unsigned long long sortHackCounter;
};

class OsuDifficultyCalculator
{
public:
	enum class SCORE_VERSION
	{
		SCORE_V1,
		SCORE_V2
	};

public:
	// stars, fully static
	static double calculateStarDiffForHitObjects(std::vector<std::shared_ptr<OsuDifficultyHitObject>> &sortedHitObjects, float CS, double *aim, double *speed, int upToObjectIndex = -1);

	// pp, use runtime mods (convenience)
	static double calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0);

	// pp, fully static
	static double calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, SCORE_VERSION scoreVersion);

	// helper functions
	static double calculateAcc(int c300, int c100, int c50, int misses);
	static double calculateBaseStrain(double strain);
	static double calculateTotalStarsFromSkills(double aim, double speed);

private:
	static ConVar *m_osu_slider_scorev2_ref;
};

#endif
