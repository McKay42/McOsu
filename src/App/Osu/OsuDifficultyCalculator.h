//================ Copyright (c) 2019, PG & Francesco149 & Khangaroo & Givikap120, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//=======================================================================================================================//

#ifndef OSUDIFFICULTYCALCULATOR_H
#define OSUDIFFICULTYCALCULATOR_H

#include "OsuDatabase.h" // for OsuDatabase::Score
#include "OsuLazerTypes.h"

class Osu;
class OsuBeatmap;
class OsuDatabaseBeatmap;
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
		float time;

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
	float getT(long pos, bool raw);

	inline long getDuration() const {return endTime - time;}

public:
	// circles (base)
	TYPE type;
	Vector2 pos;
	long time;
	long baseTime; // not adjusted by clockrate

	// spinners + sliders
	long endTime;
	long baseEndTime; // not adjusted by clockrate

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
	static constexpr const int PP_ALGORITHM_VERSION = OsuLazer::Version;

public:
	// This struct is the core data computed by difficulty calculation and used in performance calculation
	// TODO: this should match osu-lazer difficulty attributes:
	// 1) Add StarRating here, and use it globally instead of separate variable
	// 2) Add MaxCombo, HitCircle and Spinner count here, and use them globally instead of separate variable (together with SliderCount)
	// 3) Remove ApproachRate and OverallDifficulty
	struct DifficultyAttributes
	{
		OsuLazer::Attributes lazer;
		double AimDifficulty;
		double AimDifficultSliderCount;

		double SpeedDifficulty;
		double SpeedNoteCount;

		double SliderFactor;

		double AimTopWeightedSliderFactor;
		double SpeedTopWeightedSliderFactor;
		
		double AimDifficultStrainCount;
		double SpeedDifficultStrainCount;

		double NestedScorePerObject;
        double LegacyScoreBaseMultiplier;
        unsigned long MaximumLegacyComboScore;

		// Those 3 attributes are performance calculator only (for now)
		// TODO: use SliderCount globally like the attributes above and remove AR and OD
		double ApproachRate;
		double OverallDifficulty;

		int SliderCount;

		void clear();
	};

	// This structs has all the beatmap data necessary for difficulty calculation
	// It's purpose is to remove dependancy of diffcalc on the specific beatmap class/object
	struct BeatmapDiffcalcData
	{
		// Hitobjects
		std::vector<OsuDifficultyHitObject> &sortedHitObjects; // WARNING: reference...

		// Basic attributes, they're NOT adjusted by rate
		float CS, HP, AR, OD;

		// Relevant mods
		bool hidden, relax, autopilot, touchDevice;
		bool flashlight = false;
		float speedMultiplier;

		// ScoreV1 data
		unsigned long breakDuration, playableLength;

		BeatmapDiffcalcData(const OsuBeatmap* beatmap, std::vector<OsuDifficultyHitObject> &loadedSortedHitObjects); // From normal beatmap
		BeatmapDiffcalcData(const OsuDatabaseBeatmap* beatmap, const OsuDatabase::Score &score, std::vector<OsuDifficultyHitObject> &loadedSortedHitObjects); // From database beatmap (for base values), score (for mods), and loaded hitobjects
		BeatmapDiffcalcData(std::vector<OsuDifficultyHitObject> &loadedSortedHitObjects) : sortedHitObjects(loadedSortedHitObjects) // Bare minimum init with just hitobjects
		{
			CS = 5.0f;
			HP = 5.0f;
			AR = 5.0f;
			OD = 5.0f;

			hidden = false;
			relax = false;
			autopilot = false;
			touchDevice = false;
			flashlight = false;
			speedMultiplier = 1.0f;

			breakDuration = 0;
			playableLength = 0;
		}
	};

public:
	// stars, fully static
	static double calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex = -1, std::vector<double> *outAimStrains = NULL, std::vector<double> *outSpeedStrains = NULL);
	static double calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead);
	static std::vector<DifficultyAttributes> calculateLazerLiveDifficultyAttributes(const BeatmapDiffcalcData &beatmapData, const std::atomic<bool> &dead);

	static void calculateScoreV1Attributes(DifficultyAttributes &attributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex);
	static double calculateScoreV1SpinnerScore(double spinnerDuration);

	// pp, use runtime mods (convenience)
	static double calculatePPv2(Osu *osu, OsuBeatmap *beatmap, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0, unsigned long legacyTotalScore = 0);

	// pp, fully static
	static double calculatePPv2(bool isImportedLegacyScore, int version, int modsLegacy, double timescale, double ar, double od, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, unsigned long legacyTotalScore);

};

#endif
