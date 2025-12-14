//================ Copyright (c) 2019, PG & Francesco149 & Khangaroo & Givikap120, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//=======================================================================================================================//

#ifndef OSUDIFFICULTYCALCULATOR_H
#define OSUDIFFICULTYCALCULATOR_H

#include "OsuDatabase.h" // for OsuDatabase::Score

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
	static constexpr const int PP_ALGORITHM_VERSION = 20251007;

public:
	class Skills
	{
	public:
		static constexpr const int NUM_SKILLS = 3;

	public:
		enum class Skill
		{
			SPEED,
			AIM_SLIDERS,
			AIM_NO_SLIDERS,
		};

		static int skillToIndex(const Skill skill)
		{
			switch (skill)
			{
			case Skill::SPEED:
				return 0;
			case Skill::AIM_SLIDERS:
				return 1;
			case Skill::AIM_NO_SLIDERS:
				return 2;
			}

			return 0;
		}
	};

	static constexpr const double performance_base_multiplier = 1.14; // keep final pp normalized across changes

	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs

	static constexpr const double decay_base[Skills::NUM_SKILLS] = {0.3, 0.15, 0.15};			// how much strains decay per interval (if the previous interval's peak strains after applying decay are still higher than the current one's, they will be used as the peak strains).

	static constexpr const double DIFFCALC_EPSILON = 1e-32;

	// This struct is the stripped out version of difficulty attributes needed for incremental (per-object) calculation
	struct IncrementalState
	{
		double interval_end;
		double max_strain;
		double max_object_strain;
		double max_slider_strain;

		// for difficult strain count calculation
		double consistent_top_strain;
		double difficult_strains;
		double top_weighted_sliders;
		
		std::vector<double> highest_strains;
		std::vector<double> slider_strains;

		// difficulty attributes
		double aim_difficult_slider_count;
		double speed_note_count;
	};

	// This struct is the core data computed by difficulty calculation and used in performance calculation
	// TODO: this should match osu-lazer difficulty attributes:
	// 1) Add StarRating here, and use it globally instead of separate variable
	// 2) Add MaxCombo, HitCircle and Spinner count here, and use them globally instead of separate variable (together with SliderCount)
	// 3) Remove ApproachRate and OverallDifficulty
	struct DifficultyAttributes
	{
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
			speedMultiplier = 1.0f;

			breakDuration = 0;
			playableLength = 0;
		}
	};

	class DiffObject
	{
	public:
		OsuDifficultyHitObject *ho;

		double strains[Skills::NUM_SKILLS];

		// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
		// needed because raw speed strain and rhythm strain are combined in different ways
		double raw_speed_strain;
		double rhythm;

		Vector2 norm_start;		// start position normalized on radius

		double angle;			// precalc

		double jumpDistance;	// precalc
		double minJumpDistance;	// precalc
		double minJumpTime;		// precalc
		double travelDistance;	// precalc

		double delta_time;		// strain temp
		double adjusted_delta_time;		// strain temp

		bool lazyCalcFinished;	// precalc temp
		Vector2 lazyEndPos;		// precalc temp
		double lazyTravelDist;	// precalc temp
		double lazyTravelTime;	// precalc temp
		double travelTime;		// precalc temp

		double smallCircleBonus;

		const std::vector<DiffObject> &objects;	// NOTE: McOsu stores the first object in this array while lazer doesn't. newer lazer algorithms require referencing objects "randomly", so we just keep the entire vector around.
		int prevObjectIndex;					// WARNING: this will be -1 for the first object (as the name implies), see note above

		DiffObject(OsuDifficultyHitObject *base_object, float radius_scaling_factor, std::vector<DiffObject> &diff_objects, int prevObjectIdx);

		inline const DiffObject *get_previous(int backwardsIdx) const {return (objects.size() > 0 && prevObjectIndex - backwardsIdx < (int)objects.size() ? &objects[std::max(0, prevObjectIndex - backwardsIdx)] : NULL);}
		inline const DiffObject *get_next(int forwardIdx) const {return (objects.size() > 0 && prevObjectIndex + forwardIdx < (int)objects.size() ? &objects[std::max(0, prevObjectIndex + forwardIdx)] : NULL);}
		inline double get_strain(Skills::Skill type) const {return strains[Skills::skillToIndex(type)] * (type == Skills::Skill::SPEED ? rhythm : 1.0);}
		inline double get_slider_strain(Skills::Skill type) const {return ho->type == OsuDifficultyHitObject::TYPE::SLIDER ? strains[Skills::skillToIndex(type)]* (type == Skills::Skill::SPEED ? rhythm : 1.0) : -1;}
		inline static double applyDiminishingExp(double val) {return std::pow(val, 0.99);}
		inline static double strainDecay(Skills::Skill type, double ms) {return std::pow(decay_base[Skills::skillToIndex(type)], ms / 1000.0);}

		void calculate_strains(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf);
		void calculate_strain(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf, const Skills::Skill dtype);
		static double calculate_difficulty(const Skills::Skill type, const DiffObject *dobjects, size_t dobjectCount, IncrementalState *incremental, std::vector<double> *outStrains = NULL, DifficultyAttributes *outAttributes = NULL);

		static double spacing_weight1(const double distance, const Skills::Skill diff_type);
		double spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf);
		
		double get_doubletapness(const DiffObject *next, double hitWindow300) const;
	};

public:
	// stars, fully static
	static double calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex = -1, std::vector<double> *outAimStrains = NULL, std::vector<double> *outSpeedStrains = NULL);
	static double calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead);
	static double calculateDifficultyAttributesInternal(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex, std::vector<DiffObject> &cachedDiffObjects, IncrementalState *incremental, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead);

	static void calculateScoreV1Attributes(DifficultyAttributes &attributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex);
	static double calculateScoreV1SpinnerScore(double spinnerDuration);

	// pp, use runtime mods (convenience)
	static double calculatePPv2(Osu *osu, OsuBeatmap *beatmap, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0, unsigned long legacyTotalScore = 0);

	// pp, fully static
	static double calculatePPv2(bool isImportedLegacyScore, int version, int modsLegacy, double timescale, double ar, double od, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, unsigned long legacyTotalScore);

	// helper functions
	static double calculateTotalStarsFromSkills(double aim, double speed);

private:
	static ConVar *m_osu_slider_end_inside_check_offset_ref;
	static ConVar *m_osu_slider_curve_max_length_ref;

private:
	struct ScoreData
	{
		double accuracy;
		int modsLegacy;
		int countGreat;
		int countOk;
		int countMeh;
		int countMiss;
		int totalHits;
		int totalSuccessfulHits;
		int beatmapMaxCombo;
		int scoreMaxCombo;
		int amountHitObjectsWithAccuracy;
		unsigned long legacyTotalScore;
	};

	struct RhythmIsland
	{
		// NOTE: lazer stores "deltaDifferenceEpsilon" (hitWindow300 * 0.3) in this struct, but OD is constant here
		int delta;
		int deltaCount;

		inline bool equals(RhythmIsland &other, double deltaDifferenceEpsilon) const {return std::abs(delta - other.delta) < deltaDifferenceEpsilon && deltaCount == other.deltaCount;}
	};

private:
	// Skill values calculation
	static double computeAimValue(const ScoreData &score, const DifficultyAttributes &attributes, double effectiveMissCount);
	static double computeSpeedValue(const ScoreData &score, const DifficultyAttributes &attributes, double effectiveMissCount, double speedDeviation);
	static double computeAccuracyValue(const ScoreData &score, const DifficultyAttributes &attributes);

	// High deviation nerf
	static double calculateSpeedDeviation(const ScoreData &score, const DifficultyAttributes &attributes, double timescale);
	static double calculateDeviation(const DifficultyAttributes &attributes, double timescale, double relevantCountGreat, double relevantCountOk, double relevantCountMeh, double relevantCountMiss);
	static double calculateSpeedHighDeviationNerf(const DifficultyAttributes &attributes, double speedDeviation);

	static double calculateEstimatedSliderBreaks(const ScoreData &score, const DifficultyAttributes &attributes, double topWeightedSliderFactor, double effectiveMissCount);

	// ScoreV1 misscount estimation
	static double calculateScoreBasedMisscount(const DifficultyAttributes &attributes, const ScoreData &score);
	static double calculateScoreAtCombo(const DifficultyAttributes &attributes, const ScoreData &score, double combo, double relevantComboPerObject, double scoreV1Multiplier);
	static double calculateRelevantScoreComboPerObject(const DifficultyAttributes &attributes, const ScoreData &score);
	static double calculateMaximumComboBasedMissCount(const DifficultyAttributes &attributes, const ScoreData &score);
	static float getLegacyScoreMultiplier(const ScoreData &score);

	static double calculateDifficultyRating(double difficultyValue);
	static double calculateAimVisibilityFactor(double approachRate, double mechanicalDifficultyRating);
	static double calculateSpeedVisibilityFactor(double approachRate, double mechanicalDifficultyRating);
	static double calculateVisibilityBonus(double approachRate, double visibilityFactor = 1.0, double sliderFactor = 1.0);
	static double computeAimRating(double aimDifficultyValue, int totalHits, double approachRate, double overallDifficulty, double mechanicalDifficultyRating, double sliderFactor, const BeatmapDiffcalcData &beatmapData);
	static double computeSpeedRating(double speedDifficultyValue, int totalHits, double approachRate, double overallDifficulty, double mechanicalDifficultyRating, const BeatmapDiffcalcData &beatmapData);
	static double calculateStarRating(double basePerformance);
	static double calculateMechanicalDifficultyRating(double aimDifficultyValue, double speedDifficultyValue);

	// helper functions
	static double erf(double x);
	static double erfInv(double x);
	static double reverseLerp(double x, double start, double end) {return clamp<double>((x - start) / (end - start), 0.0, 1.0);};
	static double smoothstep(double x, double start, double end) {x = reverseLerp(x, start, end); return x * x * (3.0 - 2.0 * x);};
	static double smootherStep(double x, double start, double end) {x = reverseLerp(x, start, end); return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);};
	static double smoothstepBellCurve(double x, double mean = 0.5, double width = 0.5) {x -= mean; x = x > 0 ? (width - x) : (width + x); return smoothstep(x, 0, width);};
	static double logistic(double x, double midpointOffset, double multiplier, double maxValue = 1.0) {return maxValue / (1 + std::exp(multiplier * (midpointOffset - x)));}
	static double strainDifficultyToPerformance(double difficulty) {return std::pow(5.0 * std::max(1.0, difficulty / 0.0675) - 4.0, 3.0) / 100000.0;}
	static double adjustHitWindow(double hitwindow) {return std::floor(hitwindow) - 0.5;} // Adjust hitwindow to match lazer
	static double adjustOveralDifficultyByClockRate(double OD, double clockRate); // Lazer formula for adjusting OD by clock rate
};

#endif
