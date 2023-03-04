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

	struct SLIDER_SCORING_TIME
	{
		enum class TYPE
		{
			TICK,
			REPEAT,
			END,
		};

		TYPE type;
		int time;

		unsigned long long sortHack;
	};

	struct SliderScoringTimeComparator
	{
		bool operator() (const SLIDER_SCORING_TIME &a, const SLIDER_SCORING_TIME &b) const
		{
			// strict weak ordering!
			if (a.time == b.time)
				return a.sortHack < b.sortHack;
			else
				return a.time < b.time;
		}
	};

public:
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time); // circle
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime); // spinner
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<SLIDER_SCORING_TIME> scoringTimes, int repeats, bool calculateSliderCurveInConstructor); // slider
	~OsuDifficultyHitObject();

	OsuDifficultyHitObject(const OsuDifficultyHitObject&) = delete;
	OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj);

	OsuDifficultyHitObject& operator = (OsuDifficultyHitObject &&dobj);

	void updateStackPosition(float stackOffset);
	void updateCurveStackPosition(float stackOffset);

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
	std::vector<SLIDER_SCORING_TIME> scoringTimes;
	int repeats;

	// custom
	OsuSliderCurve *curve;
	bool scheduledCurveAlloc;
	std::vector<Vector2> scheduledCurveAllocControlPoints;
	float scheduledCurveAllocStackOffset;

	int stack;
	Vector2 originalPos;

	unsigned long long sortHack;

private:
	static unsigned long long sortHackCounter;
};

class OsuDifficultyCalculator
{
public:
	static constexpr const int PP_ALGORITHM_VERSION = 20220902;

public:
	// stars, fully static
	static double calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool touchDevice, double *aim, double* aimSliderFactor, double *speed, double *speedNotes, int upToObjectIndex = -1, std::vector<double> *outAimStrains = NULL, std::vector<double> *outSpeedStrains = NULL);

	// pp, use runtime mods (convenience)
	static double calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double aimSliderFactor, double speed, double speedNotes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0);

	// pp, fully static
	static double calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double aimSliderFactor, double speed, double speedNotes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50);

	// helper functions
	static double calculateTotalStarsFromSkills(double aim, double speed);

private:
	static ConVar *m_osu_slider_scorev2_ref;

	struct Attributes
	{
		double AimStrain;
		double SliderFactor;
		double SpeedStrain;
		double SpeedNoteCount;
		double ApproachRate;
		double OverallDifficulty;
		int SliderCount;
	};

	struct ScoreData
	{
		double accuracy;
		int modsLegacy;
		int countGreat;
		int countGood;
		int countMeh;
		int countMiss;
		int totalHits;
		int totalSuccessfulHits;
		int beatmapMaxCombo;
		int scoreMaxCombo;
		int amountHitObjectsWithAccuracy;
	};

	static double computeAimValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount);
	static double computeSpeedValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount);
	static double computeAccuracyValue(const ScoreData &score, const Attributes &attributes);
};

#endif
