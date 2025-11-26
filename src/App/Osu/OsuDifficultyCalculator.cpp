//================ Copyright (c) 2019, PG & Francesco149, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//==============================================================================================//

#include "OsuDifficultyCalculator.h"

#include "Engine.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuGameRules.h"
#include "OsuReplay.h"

#include "OsuSliderCurves.h"

#include <climits> // for INT_MAX

ConVar osu_stars_xexxar_angles_sliders("osu_stars_xexxar_angles_sliders", true, FCVAR_NONE, "completely enables/disables the new star/pp calc algorithm");
ConVar osu_stars_slider_curve_points_separation("osu_stars_slider_curve_points_separation", 20.0f, FCVAR_NONE, "massively reduce curve accuracy for star calculations to save memory/performance");
ConVar osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled("osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled", true, FCVAR_NONE, "generally disables all nerfs for relax/autopilot in both star/pp algorithms. since mcosu has always allowed these, the default is to not nerf them.");
ConVar osu_stars_always_recalc_live_strains("osu_stars_always_recalc_live_strains", false, FCVAR_NONE, "leave this disabled for massive performance gains for live stars/pp calc loading times (at the cost of extremely rare temporary minor accuracy loss (~0.001 stars))");
ConVar osu_stars_ignore_clamped_sliders("osu_stars_ignore_clamped_sliders", true, FCVAR_NONE, "skips processing sliders limited by osu_slider_curve_max_length");

// UTILS FUNCTIONS:
double reverseLerp(double x, double start, double end) {
	return clamp<double>((x - start) / (end - start), 0.0, 1.0);
};

double smoothStep(double x, double start, double end) {
	x = reverseLerp(x, start, end);
	return x * x * (3.0 - 2.0 * x);
};

double smootherStep(double x, double start, double end) {
	x = reverseLerp(x, start, end);
	return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
};

double smoothstepBellCurve(double x, double mean = 0.5, double width = 0.5)
{
	x -= mean;
	x = x > 0 ? (width - x) : (width + x);
	return smoothStep(x, 0, width);
};

unsigned long long OsuDifficultyHitObject::sortHackCounter = 0;

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time) : OsuDifficultyHitObject(type, pos, time, time)
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime) : OsuDifficultyHitObject(type, pos, time, endTime, 0.0f, '\0', std::vector<Vector2>(), 0.0f, std::vector<SLIDER_SCORING_TIME>(), 0, true)
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<SLIDER_SCORING_TIME> scoringTimes, int repeats, bool calculateSliderCurveInConstructor)
{
	this->type = type;
	this->pos = pos;
	this->time = time;
	this->endTime = endTime;
	this->spanDuration = spanDuration;
	this->osuSliderCurveType = osuSliderCurveType;
	this->pixelLength = pixelLength;
	this->scoringTimes = scoringTimes;

	this->curve = NULL;
	this->scheduledCurveAlloc = false;
	this->scheduledCurveAllocStackOffset = 0.0f;
	this->repeats = repeats;

	this->stack = 0;
	this->originalPos = this->pos;
	this->sortHack = sortHackCounter++;

	// build slider curve, if this is a (valid) slider
	if (this->type == TYPE::SLIDER && osu_stars_xexxar_angles_sliders.getBool() && controlPoints.size() > 1)
	{
		if (calculateSliderCurveInConstructor)
		{
			// old: too much kept memory allocations for over 14000 sliders in https://osu.ppy.sh/beatmapsets/592138#osu/1277649

			// NOTE: this is still used for beatmaps with less than 5000 sliders (because precomputing all curves is way faster, especially for star cache loader)
			// NOTE: the 5000 slider threshold was chosen by looking at the longest non-aspire marathon maps
			// NOTE: 15000 slider curves use ~1.6 GB of RAM, for 32-bit executables with 2GB cap limiting to 5000 sliders gives ~530 MB which should be survivable without crashing (even though the heap gets fragmented to fuck)

			// 6000 sliders @ The Weather Channel - 139 Degrees (Tiggz Mumbson) [Special Weather Statement].osu
			// 3674 sliders @ Various Artists - Alternator Compilation (Monstrata) [Marathon].osu
			// 4599 sliders @ Renard - Because Maybe! (Mismagius) [- Nogard Marathon -].osu
			// 4921 sliders @ Renard - Because Maybe! (Mismagius) [- Nogard Marathon v2 -].osu
			// 14960 sliders @ pishifat - H E L L O  T H E R E (Kondou-Shinichi) [Sliders in the 69th centries].osu
			// 5208 sliders @ MillhioreF - haitai but every hai adds another haitai in the background (Chewy-san) [Weriko Rank the dream (nerf) but loli].osu

			this->curve = OsuSliderCurve::createCurve(this->osuSliderCurveType, controlPoints, this->pixelLength, osu_stars_slider_curve_points_separation.getFloat());
		}
		else
		{
			// new: delay curve creation to when it's needed, and also immediately delete afterwards (at the cost of having to store a copy of the control points)
			this->scheduledCurveAlloc = true;
			this->scheduledCurveAllocControlPoints = controlPoints;
		}
	}
}

OsuDifficultyHitObject::~OsuDifficultyHitObject()
{
	SAFE_DELETE(curve);
}

OsuDifficultyHitObject::OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj)
{
	// move
	this->type = dobj.type;
	this->pos = dobj.pos;
	this->time = dobj.time;
	this->endTime = dobj.endTime;
	this->spanDuration = dobj.spanDuration;
	this->osuSliderCurveType = dobj.osuSliderCurveType;
	this->pixelLength = dobj.pixelLength;
	this->scoringTimes = std::move(dobj.scoringTimes);

	this->curve = dobj.curve;
	this->scheduledCurveAlloc = dobj.scheduledCurveAlloc;
	this->scheduledCurveAllocControlPoints = std::move(dobj.scheduledCurveAllocControlPoints);
	this->scheduledCurveAllocStackOffset = dobj.scheduledCurveAllocStackOffset;
	this->repeats = dobj.repeats;

	this->stack = dobj.stack;
	this->originalPos = dobj.originalPos;
	this->sortHack = dobj.sortHack;

	// reset source
	dobj.curve = NULL;
	dobj.scheduledCurveAlloc = false;
}

OsuDifficultyHitObject& OsuDifficultyHitObject::operator = (OsuDifficultyHitObject &&dobj)
{
	// move
	this->type = dobj.type;
	this->pos = dobj.pos;
	this->time = dobj.time;
	this->endTime = dobj.endTime;
	this->spanDuration = dobj.spanDuration;
	this->osuSliderCurveType = dobj.osuSliderCurveType;
	this->pixelLength = dobj.pixelLength;
	this->scoringTimes = std::move(dobj.scoringTimes);

	this->curve = dobj.curve;
	this->scheduledCurveAlloc = dobj.scheduledCurveAlloc;
	this->scheduledCurveAllocControlPoints = std::move(dobj.scheduledCurveAllocControlPoints);
	this->scheduledCurveAllocStackOffset = dobj.scheduledCurveAllocStackOffset;
	this->repeats = dobj.repeats;

	this->stack = dobj.stack;
	this->originalPos = dobj.originalPos;
	this->sortHack = dobj.sortHack;

	// reset source
	dobj.curve = NULL;
	dobj.scheduledCurveAlloc = false;

	return *this;
}

void OsuDifficultyHitObject::updateStackPosition(float stackOffset)
{
	scheduledCurveAllocStackOffset = stackOffset;

	pos = originalPos - Vector2(stack * stackOffset, stack * stackOffset);

	updateCurveStackPosition(stackOffset);
}

void OsuDifficultyHitObject::updateCurveStackPosition(float stackOffset)
{
	if (curve != NULL)
		curve->updateStackPosition(stack * stackOffset, false);
}

Vector2 OsuDifficultyHitObject::getOriginalRawPosAt(long pos)
{
	// NOTE: the delayed curve creation has been deliberately disabled here for stacking purposes for beatmaps with insane slider counts for performance reasons
	// NOTE: this means that these aspire maps will have incorrect stars due to incorrect slider stacking, but the delta is below 0.02 even for the most insane maps which currently exist
	// NOTE: if this were to ever get implemented properly, then a sliding window for curve allocation must be added to the stack algorithm itself (as doing it in here is O(n!) madness)
	// NOTE: to validate the delta, use Acid Rain [Aspire] - Karoo13 (6.76* with slider stacks -> 6.75* without slider stacks)

	if (type != TYPE::SLIDER || (curve == NULL/* && !scheduledCurveAlloc*/))
		return originalPos;
	else
	{
		/*
		// MCKAY:
		{
			// delay curve creation to when it's needed (1)
			if (curve == NULL && scheduledCurveAlloc)
				curve = OsuSliderCurve::createCurve(osuSliderCurveType, scheduledCurveAllocControlPoints, pixelLength, osu_stars_slider_curve_points_separation.getFloat());
		}
		*/

		// old (broken)
		/*
        float progress = (float)(clamp<long>(pos, time, endTime)) / spanDuration;
        if (std::fmod(progress, 2.0f) >= 1.0f)
            progress = 1.0f - std::fmod(progress, 1.0f);
        else
            progress = std::fmod(progress, 1.0f);

        const Vector2 originalPointAt = curve->originalPointAt(progress);
        */

		// new (correct)
		if (pos <= time)
			return curve->originalPointAt(0.0f);
		else if (pos >= endTime)
		{
			if (repeats % 2 == 0)
				return curve->originalPointAt(0.0f);
			else
				return curve->originalPointAt(1.0f);
		}
		else
			return curve->originalPointAt(getT(pos, false));

        /*
        // MCKAY:
        {
        	// and delete it immediately afterwards (2)
        	if (scheduledCurveAlloc)
        		SAFE_DELETE(curve);
        }
		*/
	}
}

float OsuDifficultyHitObject::getT(long pos, bool raw)
{
	float t = (float)((long)pos - (long)time) / spanDuration;
	if (raw)
		return t;
	else
	{
		float floorVal = (float) std::floor(t);
		return ((int)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
	}
}

// DIFFICULTY CALCULATOR

ConVar *OsuDifficultyCalculator::m_osu_slider_scorev2_ref = NULL;
ConVar *OsuDifficultyCalculator::m_osu_slider_end_inside_check_offset_ref = NULL;
ConVar *OsuDifficultyCalculator::m_osu_slider_curve_max_length_ref = NULL;

const double performance_base_multiplier = 1.14;

// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuRatingCalculator.cs

double calculateDifficultyRating(double difficultyValue) {
	const double difficulty_multiplier = 0.0675;
	return std::sqrt(difficultyValue) * difficulty_multiplier;
}

double calculateAimVisibilityFactor(double approachRate, double mechanicalDifficultyRating) {
	const double ar_factor_end_point = 11.5;

	double mechanicalDifficultyFactor = reverseLerp(mechanicalDifficultyRating, 5, 10);
	double arFactorStartingPoint = lerp<double>(9, 10.33, mechanicalDifficultyFactor);

	return reverseLerp(approachRate, ar_factor_end_point, arFactorStartingPoint);
}

double calculateSpeedVisibilityFactor(double approachRate, double mechanicalDifficultyRating) {
	const double ar_factor_end_point = 11.5;

	double mechanicalDifficultyFactor = reverseLerp(mechanicalDifficultyRating, 5, 10);
	double arFactorStartingPoint = lerp<double>(10, 10.33, mechanicalDifficultyFactor);

	return reverseLerp(approachRate, ar_factor_end_point, arFactorStartingPoint);
}

double calculateVisibilityBonus(double approachRate, double visibilityFactor = 1, double sliderFactor = 1) {
	// WARNING: this value is equal to true for Traceable (Lazer mod) and the lazer-specific HD setting.
	// So in this case we're always setting it to false
	bool isAlwaysPartiallyVisible = false;

	double readingBonus = (isAlwaysPartiallyVisible ? 0.025 : 0.04) * (12.0 - std::max(approachRate, 7.0));

	readingBonus *= visibilityFactor;

	// We want to reward slideraim on low AR less
	double sliderVisibilityFactor = std::pow(sliderFactor, 3);

	// For AR up to 0 - reduce reward for very low ARs when object is visible
	if (approachRate < 7)
		readingBonus += (isAlwaysPartiallyVisible ? 0.02 : 0.045) * (7.0 - std::max(approachRate, 0.0)) * sliderVisibilityFactor;

	// Starting from AR0 - cap values so they won't grow to infinity
	if (approachRate < 0)
		readingBonus += (isAlwaysPartiallyVisible ? 0.01 : 0.1) * (1 - std::max(1.5, approachRate)) * sliderVisibilityFactor;

	return readingBonus;
}

double computeAimRating(double aimDifficultyValue, int totalHits, double approachRate, double overallDifficulty, double mechanicalDifficultyRating, double sliderFactor, bool hidden, bool touchDevice, bool relax, bool autopilot) {
	if (autopilot)
		return 0;

	double aimRating = calculateDifficultyRating(aimDifficultyValue);

	if (touchDevice)
		aimRating = std::pow(aimRating, 0.8);

	if (relax)
		aimRating *= 0.9;

	double ratingMultiplier = 1.0;

	double approachRateLengthBonus = 0.95 + 0.4 * std::min(1.0, totalHits / 2000.0) +
										(totalHits > 2000 ? std::log10(totalHits / 2000.0) * 0.5 : 0.0);

	double approachRateFactor = 0.0;
	if (approachRate > 10.33)
		approachRateFactor = 0.3 * (approachRate - 10.33);
	else if (approachRate < 8.0)
		approachRateFactor = 0.05 * (8.0 - approachRate);

	if (relax)
		approachRateFactor = 0.0;

	ratingMultiplier += approachRateFactor * approachRateLengthBonus; // Buff for longer maps with high AR.

	if (hidden)
	{
		double visibilityFactor = calculateAimVisibilityFactor(approachRate, mechanicalDifficultyRating);
		ratingMultiplier += calculateVisibilityBonus(approachRate, visibilityFactor, sliderFactor);
	}

	// It is important to consider accuracy difficulty when scaling with accuracy.
	ratingMultiplier *= 0.98 + std::pow(std::max(0.0, overallDifficulty), 2) / 2500;

	return aimRating * std::cbrt(ratingMultiplier);
}

double computeSpeedRating(double speedDifficultyValue, int totalHits, double approachRate, double overallDifficulty, double mechanicalDifficultyRating, bool hidden, bool relax, bool autopilot) {
	if (relax)
		return 0;

	double speedRating = calculateDifficultyRating(speedDifficultyValue);

	if (autopilot)
		speedRating *= 0.5;

	double ratingMultiplier = 1.0;

	double approachRateLengthBonus = 0.95 + 0.4 * std::min(1.0, totalHits / 2000.0) +
										(totalHits > 2000 ? std::log10(totalHits / 2000.0) * 0.5 : 0.0);

	double approachRateFactor = 0.0;
	if (approachRate > 10.33)
		approachRateFactor = 0.3 * (approachRate - 10.33);

	if (autopilot)
		approachRateFactor = 0.0;

	ratingMultiplier += approachRateFactor * approachRateLengthBonus; // Buff for longer maps with high AR.

	if (hidden)
	{
		double visibilityFactor = calculateSpeedVisibilityFactor(approachRate, mechanicalDifficultyRating);
		ratingMultiplier += calculateVisibilityBonus(approachRate, visibilityFactor);
	}

	ratingMultiplier *= 0.95 + std::pow(std::max(0.0, overallDifficulty), 2) / 750;

	return speedRating * std::cbrt(ratingMultiplier);
}

double strainDifficultyToPerformance(double difficulty) { return std::pow(5.0 * std::max(1.0, difficulty / 0.0675) - 4.0, 3.0) / 100000.0;; }

// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuDifficultyCalculator.cs#L148-L164
double calculateStarRating(double basePerformance) {
	const double star_rating_multiplier = 0.0265;

	if (basePerformance <= 0.00001)
		return 0;

	return std::cbrt(performance_base_multiplier) * star_rating_multiplier * (std::cbrt(100000 / std::pow(2, 1 / 1.1) * basePerformance) + 4);
}

double calculateMechanicalDifficultyRating(double aimDifficultyValue, double speedDifficultyValue) {
	double aimValue = strainDifficultyToPerformance(calculateDifficultyRating(aimDifficultyValue));
	double speedValue = strainDifficultyToPerformance(calculateDifficultyRating(speedDifficultyValue));

	double totalValue = std::pow(std::pow(aimValue, 1.1) + std::pow(speedValue, 1.1), 1 / 1.1);

	return calculateStarRating(totalValue);
}

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float AR, float OD, float speedMultiplier, bool hidden, bool relax, bool autopilot, bool touchDevice, Attributes* attributes, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains)
{
	std::atomic<bool> dead;
	dead = false;
	return calculateStarDiffForHitObjects(sortedHitObjects, CS, AR, OD, speedMultiplier, hidden, relax, autopilot, touchDevice, attributes, upToObjectIndex, outAimStrains, outSpeedStrains, dead);
}

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float AR, float OD, float speedMultiplier, bool hidden, bool relax, bool autopilot, bool touchDevice, Attributes* attributes, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead)
{
	std::vector<DiffObject> emptyCachedDiffObjects;
	return calculateStarDiffForHitObjectsInt(emptyCachedDiffObjects, sortedHitObjects, CS, AR, OD, speedMultiplier, hidden, relax, autopilot, touchDevice, attributes, upToObjectIndex, NULL, outAimStrains, outSpeedStrains, dead);
}

double OsuDifficultyCalculator::calculateStarDiffForHitObjectsInt(std::vector<DiffObject> &cachedDiffObjects, std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float AR, float OD, float speedMultiplier, bool hidden, bool relax, bool autopilot, bool touchDevice, Attributes* attributes, int upToObjectIndex, IncrementalState *incremental, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead)
{
	// NOTE: depends on speed multiplier + CS + OD + relax + touchDevice

	// NOTE: upToObjectIndex is applied way below, during the construction of the 'dobjects'

	// NOTE: osu always returns 0 stars for beatmaps with only 1 object, except if that object is a slider
	if (sortedHitObjects.size() < 2)
	{
		if (sortedHitObjects.size() < 1) return 0.0;
		if (sortedHitObjects[0].type != OsuDifficultyHitObject::TYPE::SLIDER) return 0.0;
	}

	if (m_osu_slider_end_inside_check_offset_ref == NULL)
		m_osu_slider_end_inside_check_offset_ref = convar->getConVarByName("osu_slider_end_inside_check_offset");

	// global independent variables/constants
	float circleRadiusInOsuPixels = 64.0f * OsuGameRules::getRawHitCircleScale(clamp<float>(CS, 0.0f, 12.142f)); // NOTE: clamped CS because McOsu allows CS > ~12.1429 (at which point the diameter becomes negative)
	const float hitWindow300 = 2.0f * OsuGameRules::getRawHitWindow300(OD) / speedMultiplier;

	// ****************************************************************************************************************************************** //

	// see setDistances() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

	static const float normalized_radius = 50.0f;		// normalization factor
	static const float maximum_slider_radius = normalized_radius * 2.4f;
	static const float assumed_slider_radius = normalized_radius * 1.8f;
	static const float circlesize_buff_treshold = 30;	// non-normalized diameter where the circlesize buff starts

	// multiplier to normalize positions so that we can calc as if everything was the same circlesize.
	// also handle high CS bonus

	float radius_scaling_factor = normalized_radius / circleRadiusInOsuPixels;
	if (circleRadiusInOsuPixels < circlesize_buff_treshold)
	{
		const float smallCircleBonus = std::min(circlesize_buff_treshold - circleRadiusInOsuPixels, 5.0f) / 50.0f;
		radius_scaling_factor *= 1.0f + smallCircleBonus;
	}

	// ****************************************************************************************************************************************** //

	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

	class DistanceCalc
	{
	public:
		static void computeSliderCursorPosition(DiffObject &slider, float circleRadius)
		{
			if (slider.lazyCalcFinished || slider.ho->curve == NULL) return;

			// NOTE: lazer won't load sliders above a certain length, but mcosu will
			// this isn't entirely accurate to how lazer does it (as that skips loading the object entirely),
			// but this is a good middle ground for maps that aren't completely aspire and still have relatively normal star counts on lazer
			// see: DJ Noriken - Stargazer feat. YUC'e (PSYQUI Remix) (Hishiro Chizuru) [Starg-Azer isn't so great? Are you kidding me?]
			if (osu_stars_ignore_clamped_sliders.getBool())
			{
				if (m_osu_slider_curve_max_length_ref == NULL)
					m_osu_slider_curve_max_length_ref = convar->getConVarByName("osu_slider_curve_max_length");

				if (slider.ho->curve->getPixelLength() >= m_osu_slider_curve_max_length_ref->getFloat()) return;
			}

			// NOTE: although this looks like a duplicate of the end tick time, this really does have a noticeable impact on some maps due to precision issues
			// see: Ocelot - KAEDE (Hollow Wings) [EX EX]
			const double tailLeniency = (double)m_osu_slider_end_inside_check_offset_ref->getInt();
			const double totalDuration = (double)slider.ho->spanDuration * slider.ho->repeats;
			double trackingEndTime = (double)slider.ho->time + std::max(totalDuration - tailLeniency, totalDuration / 2.0);

			// NOTE: lazer has logic to reorder the last slider tick if it happens after trackingEndTime here, which already happens in mcosu

			slider.lazyTravelTime = trackingEndTime - (double)slider.ho->time;

			double endTimeMin = slider.lazyTravelTime / slider.ho->spanDuration;
			if (std::fmod(endTimeMin, 2.0) >= 1.0)
				endTimeMin = 1.0 - std::fmod(endTimeMin, 1.0);
			else
				endTimeMin = std::fmod(endTimeMin, 1.0);

			slider.lazyEndPos = slider.ho->curve->pointAt(endTimeMin);

			Vector2 cursor_pos = slider.ho->pos;
			double scaling_factor = 50.0 / circleRadius;

			for (size_t i=0; i<slider.ho->scoringTimes.size(); i++)
			{
				Vector2 diff;

				if (slider.ho->scoringTimes[i].type == OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::END)
				{
					// NOTE: In lazer, the position of the slider end is at the visual end, but the time is at the scoring end
					diff = slider.ho->curve->pointAt(slider.ho->repeats % 2 ? 1.0 : 0.0) - cursor_pos;
				}
				else
				{
					double progress = (clamp<float>(slider.ho->scoringTimes[i].time - (float)slider.ho->time, 0.0f, slider.ho->getDuration())) / slider.ho->spanDuration;
					if (std::fmod(progress, 2.0) >= 1.0)
						progress = 1.0 - std::fmod(progress, 1.0);
					else
						progress = std::fmod(progress, 1.0);

					diff = slider.ho->curve->pointAt(progress) - cursor_pos;
				}

				double diff_len = scaling_factor * diff.length();

				double req_diff = 90.0;

				if (i == slider.ho->scoringTimes.size() - 1)
				{
					// Slider end
					Vector2 lazy_diff = slider.lazyEndPos - cursor_pos;
					if (lazy_diff.length() < diff.length())
						diff = lazy_diff;
					diff_len = scaling_factor * diff.length();
				}
				else if (slider.ho->scoringTimes[i].type == OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::REPEAT)
				{
					// Slider repeat
					req_diff = 50.0;
				}

				if (diff_len > req_diff)
				{
					cursor_pos += (diff * (float)((diff_len - req_diff) / diff_len));
					diff_len *= (diff_len - req_diff) / diff_len;
					slider.lazyTravelDist += diff_len;
				}

				if (i == slider.ho->scoringTimes.size() - 1)
					slider.lazyEndPos = cursor_pos;
			}

			slider.lazyCalcFinished = true;
		}

		static Vector2 getEndCursorPosition(DiffObject &hitObject, float circleRadius)
		{
			if (hitObject.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
			{
				computeSliderCursorPosition(hitObject, circleRadius);
				return hitObject.lazyEndPos; // (slider.lazyEndPos is already initialized to ho->pos in DiffObject constructor)
			}

			return hitObject.ho->pos;
		}
	};

	// ****************************************************************************************************************************************** //

	// initialize dobjects
	const size_t numDiffObjects = (upToObjectIndex < 0) ? sortedHitObjects.size() : upToObjectIndex + 1;
	const bool isUsingCachedDiffObjects = (cachedDiffObjects.size() > 0);
	DiffObject *diffObjects;
	if (!isUsingCachedDiffObjects)
	{
		// not cached (full rebuild computation)
		cachedDiffObjects.reserve(numDiffObjects);
		for (size_t i=0; i<numDiffObjects; i++)
		{
			if (dead.load())
				return 0.0;

			cachedDiffObjects.push_back(DiffObject(&sortedHitObjects[i], radius_scaling_factor, cachedDiffObjects, (int)i - 1)); // this already initializes the angle to NaN
		}
	}
	diffObjects = cachedDiffObjects.data();

	// calculate angles and travel/jump distances (before calculating strains)
	if (!isUsingCachedDiffObjects)
	{
		if (osu_stars_xexxar_angles_sliders.getBool())
		{
			const float starsSliderCurvePointsSeparation = osu_stars_slider_curve_points_separation.getFloat();
			for (size_t i=0; i<numDiffObjects; i++)
			{
				if (dead.load())
					return 0.0;

				// see setDistances() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

				if (i > 0)
				{
					// calculate travel/jump distances
					DiffObject &cur = diffObjects[i];
					DiffObject &prev1 = diffObjects[i - 1];

					// MCKAY:
					{
						// delay curve creation to when it's needed (1)
						if (prev1.ho->scheduledCurveAlloc && prev1.ho->curve == NULL)
						{
							prev1.ho->curve = OsuSliderCurve::createCurve(prev1.ho->osuSliderCurveType, prev1.ho->scheduledCurveAllocControlPoints, prev1.ho->pixelLength, starsSliderCurvePointsSeparation);
							prev1.ho->updateCurveStackPosition(prev1.ho->scheduledCurveAllocStackOffset); // NOTE: respect stacking
						}
					}

					if (cur.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
					{
						DistanceCalc::computeSliderCursorPosition(cur, circleRadiusInOsuPixels);
						cur.travelDistance = cur.lazyTravelDist * std::pow(1.0 + (cur.ho->repeats - 1) / 2.5, 1.0 / 2.5);
						cur.travelTime = std::max(cur.lazyTravelTime, 25.0);
					}

					// don't need to jump to reach spinners
					if (cur.ho->type == OsuDifficultyHitObject::TYPE::SPINNER || prev1.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
						continue;

					const Vector2 lastCursorPosition = DistanceCalc::getEndCursorPosition(prev1, circleRadiusInOsuPixels);

					double cur_strain_time = (double)std::max(cur.ho->time - prev1.ho->time, 25l); // adjusted_delta_time isn't initialized here
					cur.jumpDistance = (cur.norm_start - lastCursorPosition*radius_scaling_factor).length();
					cur.minJumpDistance = cur.jumpDistance;
					cur.minJumpTime = cur_strain_time;

					if (prev1.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
					{
						double last_travel = std::max(prev1.lazyTravelTime, 25.0);
						cur.minJumpTime = std::max(cur_strain_time - last_travel, 25.0);

						// NOTE: "curve shouldn't be null here, but Yin [test7] causes that to happen"
						// NOTE: the curve can be null if controlPoints.size() < 1 because the OsuDifficultyHitObject() constructor will then not set scheduledCurveAlloc to true (which is perfectly fine and correct)
						float tail_jump_dist = (prev1.ho->curve ? prev1.ho->curve->pointAt(prev1.ho->repeats % 2 ? 1.0 : 0.0) : prev1.ho->pos).distance(cur.ho->pos) * radius_scaling_factor;
						cur.minJumpDistance = std::max(0.0f, std::min((float)cur.minJumpDistance - (maximum_slider_radius - assumed_slider_radius), tail_jump_dist - maximum_slider_radius));
					}

					// calculate angles
					if (i > 1)
					{
						DiffObject &prev2 = diffObjects[i - 2];
						if (prev2.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
							continue;

						const Vector2 lastLastCursorPosition = DistanceCalc::getEndCursorPosition(prev2, circleRadiusInOsuPixels);

						// MCKAY:
						{
							// and also immediately delete afterwards (2)
							if (i > 2) // NOTE: this trivial sliding window implementation will keep the last 2 curves alive at the end, but they get auto deleted later anyway so w/e
							{
								DiffObject &prev3 = diffObjects[i - 3];

								if (prev3.ho->scheduledCurveAlloc)
									SAFE_DELETE(prev3.ho->curve);
							}
						}

						const Vector2 v1 = lastLastCursorPosition - prev1.ho->pos;
						const Vector2 v2 = cur.ho->pos - lastCursorPosition;

						const double dot = v1.dot(v2);
						const double det = (v1.x * v2.y) - (v1.y * v2.x);

						cur.angle = std::fabs(std::atan2(det, dot));
					}
				}
			}
		}
	}

	// calculate strains/skills
	if (!isUsingCachedDiffObjects || osu_stars_always_recalc_live_strains.getBool()) // NOTE: yes, this loses some extremely minor accuracy (~0.001 stars territory) for live star/pp for some rare individual upToObjectIndex due to not being recomputed for the cut set of cached diffObjects every time, but the performance gain is so insane I don't care
	{
		bool autopilotNerf = !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool() && autopilot;
		for (size_t i=1; i<numDiffObjects; i++) // NOTE: start at 1
		{
			diffObjects[i].calculate_strains(diffObjects[i - 1], (i == numDiffObjects - 1) ? nullptr : &diffObjects[i + 1], hitWindow300, autopilotNerf);
		}
	}

	// calculate final difficulty (weigh strains)
	double aimNoSliders = osu_stars_xexxar_angles_sliders.getBool() ? DiffObject::calculate_difficulty(Skills::Skill::AIM_NO_SLIDERS, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::AIM_NO_SLIDERS] : NULL) : 0.0;
	double aim = DiffObject::calculate_difficulty(Skills::Skill::AIM_SLIDERS, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::AIM_SLIDERS] : NULL, outAimStrains, attributes);
	double speed = DiffObject::calculate_difficulty(Skills::Skill::SPEED, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::SPEED] : NULL, outSpeedStrains, attributes);

	attributes->SliderFactor = (aim > 0 && osu_stars_xexxar_angles_sliders.getBool()) ? calculateDifficultyRating(aimNoSliders) / calculateDifficultyRating(aim) : 1.0;

	double mechanicalDifficultyRating = calculateMechanicalDifficultyRating(aim, speed);

	if (osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
	{
		relax = false;
		autopilot = false;
	}

	aimNoSliders = computeAimRating(aimNoSliders, numDiffObjects, AR, OD, mechanicalDifficultyRating, attributes->SliderFactor, hidden, touchDevice, relax, autopilot);
	aim = computeAimRating(aim, numDiffObjects, AR, OD, mechanicalDifficultyRating, attributes->SliderFactor, hidden, touchDevice, relax, autopilot);
	speed = computeSpeedRating(speed, numDiffObjects, AR, OD, mechanicalDifficultyRating, hidden, relax, autopilot);

	attributes->AimDifficulty = aim;
	attributes->SpeedDifficulty = speed;

	return calculateTotalStarsFromSkills(aim, speed);
}

double OsuDifficultyCalculator::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, Attributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
{
	// NOTE: depends on active mods + OD + AR

	if (m_osu_slider_scorev2_ref == NULL)
		m_osu_slider_scorev2_ref = convar->getConVarByName("osu_slider_scorev2");

	// get runtime mods
	int modsLegacy = osu->getScore()->getModsLegacy();
	{
		// special case: manual slider accuracy has been enabled (affects pp but not score)
		modsLegacy |= (m_osu_slider_scorev2_ref->getBool() ? OsuReplay::Mods::ScoreV2 : 0);
	}

	return calculatePPv2(modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(), beatmap->getOD(), attributes, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo, combo, misses, c300, c100, c50);
}

double OsuDifficultyCalculator::calculatePPv2(int modsLegacy, double timescale, double ar, double od, Attributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
{
	// NOTE: depends on active mods + OD + AR

	// apply "timescale" aka speed multiplier to ar/od
	// (the original incoming ar/od values are guaranteed to not yet have any speed multiplier applied to them, but they do have non-time-related mods already applied, like HR or any custom overrides)
	// (yes, this does work correctly when the override slider "locking" feature is used. in this case, the stored ar/od is already compensated such that it will have the locked value AFTER applying the speed multiplier here)
	// (all UI elements which display ar/od from stored scores, like the ranking screen or score buttons, also do this calculation before displaying the values to the user. of course the mod selection screen does too.)
	od = OsuGameRules::getRawOverallDifficultyForSpeedMultiplier(OsuGameRules::getRawHitWindow300(od), timescale);
	ar = OsuGameRules::getRawApproachRateForSpeedMultiplier(OsuGameRules::getRawApproachTime(ar), timescale);

	if (c300 < 0)
		c300 = numHitObjects - c100 - c50 - misses;

	if (combo < 0)
		combo = maxPossibleCombo;

	if (combo < 1) return 0.0;

	ScoreData score;
	{
		score.modsLegacy = modsLegacy;
		score.countGreat = c300;
		score.countGood = c100;
		score.countMeh = c50;
		score.countMiss = misses;
		score.totalHits = c300 + c100 + c50 + misses;
		score.totalSuccessfulHits = c300 + c100 + c50;
		score.beatmapMaxCombo = maxPossibleCombo;
		score.scoreMaxCombo = combo;
		{
			score.accuracy = (score.totalHits > 0 ? (double)(c300 * 300 + c100 * 100 + c50 * 50) / (double)(score.totalHits * 300) : 0.0);
			score.amountHitObjectsWithAccuracy = (modsLegacy & OsuReplay::ScoreV2 ? numCircles + numSliders : numCircles);
		}
	}

	attributes.ApproachRate = ar;
	attributes.OverallDifficulty = od;
	attributes.SliderCount = numSliders;

	// calculateEffectiveMissCount @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuPerformanceCalculator.cs
	// required because slider breaks aren't exposed to pp calculation
	double comboBasedMissCount = 0.0;
	if (numSliders > 0)
	{
		double fullComboThreshold = maxPossibleCombo - (0.1 * numSliders);
		if ((double)combo < fullComboThreshold)
			comboBasedMissCount = fullComboThreshold / std::max(1.0, (double)combo);
	}
	double effectiveMissCount = clamp<double>(comboBasedMissCount, (double)misses, (double)(c50 + c100 + misses));

	// custom multipliers for nofail and spunout
	double multiplier = 1.15; // keep final pp normalized across changes
	{
		if (modsLegacy & OsuReplay::Mods::NoFail)
			multiplier *= std::max(0.9, 1.0 - 0.02 * effectiveMissCount); // see https://github.com/ppy/osu-performance/pull/127/files

		if ((modsLegacy & OsuReplay::Mods::SpunOut) && score.totalHits > 0)
			multiplier *= 1.0 - std::pow((double)numSpinners / (double)score.totalHits, 0.85); // see https://github.com/ppy/osu-performance/pull/110/

		if ((modsLegacy & OsuReplay::Mods::Relax) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		{
			double okMultiplier = std::max(0.0, od > 0.0 ? 1.0 - std::pow(od / 13.33, 1.8) : 1.0);	// 100
			double mehMultiplier = std::max(0.0, od > 0.0 ? 1.0 - std::pow(od / 13.33, 5.0) : 1.0);	// 50
			effectiveMissCount = std::min(effectiveMissCount + c100 * okMultiplier + c50 * mehMultiplier, (double)score.totalHits);
		}
	}

	const double speedDeviation = calculateSpeedDeviation(score, attributes, timescale);
	const double aimValue = computeAimValue(score, attributes, effectiveMissCount);
	const double speedValue = computeSpeedValue(score, attributes, effectiveMissCount, speedDeviation);
	const double accuracyValue = computeAccuracyValue(score, attributes);

	const double totalValue = std::pow(
								std::pow(aimValue, 1.1)
								+ std::pow(speedValue, 1.1)
								+ std::pow(accuracyValue, 1.1), 1.0 / 1.1
							) * multiplier;

	return totalValue;
}

double OsuDifficultyCalculator::calculateTotalStarsFromSkills(double aim, double speed)
{
	double baseAimPerformance = strainDifficultyToPerformance(aim);
	double baseSpeedPerformance = strainDifficultyToPerformance(speed);
	double basePerformance = std::pow(std::pow(baseAimPerformance, 1.1) + std::pow(baseSpeedPerformance, 1.1), 1.0 / 1.1);
	return calculateStarRating(basePerformance);
}

// https://github.com/ppy/osu-performance/blob/master/src/performance/osu/OsuScore.cpp
// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuPerformanceCalculator.cs

double OsuDifficultyCalculator::computeAimValue(const ScoreData &score, const OsuDifficultyCalculator::Attributes &attributes, double effectiveMissCount)
{
	if ((score.modsLegacy & OsuReplay::Relax2) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		return 0.0;

	double aimDifficulty = attributes.AimDifficulty;

	// McOsu doesn't track dropped slider ends, so the ScoreV2/lazer case can't be handled here
	if (attributes.SliderCount > 0 && attributes.AimDifficultSliderCount > 0)
	{
		int maximumPossibleDroppedSliders = score.countGood + score.countMeh + score.countMiss;
		double estimateImproperlyFollowedDifficultSliders = clamp<double>((double)std::min(maximumPossibleDroppedSliders, score.beatmapMaxCombo - score.scoreMaxCombo), 0.0, attributes.AimDifficultSliderCount);
		double sliderNerfFactor = (1.0 - attributes.SliderFactor) * std::pow(1.0 - estimateImproperlyFollowedDifficultSliders / attributes.AimDifficultSliderCount, 3.0) + attributes.SliderFactor;
		aimDifficulty *= sliderNerfFactor;
	}

	double aimValue = std::pow(5.0 * std::max(1.0, aimDifficulty / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	double lengthBonus = 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);
	aimValue *= lengthBonus;

	// miss penalty
	// see https://github.com/ppy/osu/pull/16280/
	if (effectiveMissCount > 0 && score.totalHits > 0)
		aimValue *= 0.96 / ((effectiveMissCount / (4.0 * std::pow(std::log(attributes.AimDifficultStrainCount), 0.94))) + 1.0);

	// scale aim with acc
	aimValue *= score.accuracy;

	return aimValue;
}

double OsuDifficultyCalculator::computeSpeedValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount, double speedDeviation)
{
	if (((score.modsLegacy & OsuReplay::Relax) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool()) || std::isnan(speedDeviation))
		return 0.0;

	double speedValue = std::pow(5.0 * std::max(1.0, attributes.SpeedDifficulty / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	double lengthBonus = 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);
	speedValue *= lengthBonus;

	// miss penalty
	// see https://github.com/ppy/osu/pull/16280/
	if (effectiveMissCount > 0)
		speedValue *= 0.96 / ((effectiveMissCount / (4.0 * std::pow(std::log(attributes.SpeedDifficultStrainCount), 0.94))) + 1.0);

	double speedHighDeviationMultiplier = calculateSpeedHighDeviationNerf(attributes, speedDeviation);
	speedValue *= speedHighDeviationMultiplier;

	// "Calculate accuracy assuming the worst case scenario"
	double relevantTotalDiff = std::max(0.0, score.totalHits - attributes.SpeedNoteCount);
	double relevantCountGreat = std::max(0.0, score.countGreat - relevantTotalDiff);
	double relevantCountOk = std::max(0.0, score.countGood - std::max(0.0, relevantTotalDiff - score.countGreat));
	double relevantCountMeh = std::max(0.0, score.countMeh - std::max(0.0, relevantTotalDiff - score.countGreat - score.countGood));
	double relevantAccuracy = attributes.SpeedNoteCount == 0 ? 0 : (relevantCountGreat * 6.0 + relevantCountOk * 2.0 + relevantCountMeh) / (attributes.SpeedNoteCount * 6.0);

	// see https://github.com/ppy/osu-performance/pull/128/
	// Scale the speed value with accuracy and OD
	speedValue *= std::pow((score.accuracy + relevantAccuracy) / 2.0, (14.5 - attributes.OverallDifficulty) / 2);

	return speedValue;
}

double OsuDifficultyCalculator::computeAccuracyValue(const ScoreData &score, const Attributes &attributes)
{
	if ((score.modsLegacy & OsuReplay::Relax) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		return 0.0;

	double betterAccuracyPercentage;
	if (score.amountHitObjectsWithAccuracy > 0)
		betterAccuracyPercentage = ((double)(score.countGreat - std::max(score.totalHits - score.amountHitObjectsWithAccuracy, 0)) * 6.0 + (score.countGood * 2.0) + score.countMeh) / (double)(score.amountHitObjectsWithAccuracy * 6.0);
	else
		betterAccuracyPercentage = 0.0;

	// it's possible to reach negative accuracy, cap at zero
	if (betterAccuracyPercentage < 0.0)
		betterAccuracyPercentage = 0.0;

	// arbitrary values tom crafted out of trial and error
	double accuracyValue = std::pow(1.52163, attributes.OverallDifficulty) * std::pow(betterAccuracyPercentage, 24.0) * 2.83;

	// length bonus
	accuracyValue *= std::min(1.15, std::pow(score.amountHitObjectsWithAccuracy / 1000.0, 0.3));

	// hidden bonus
	if (score.modsLegacy & OsuReplay::Mods::Hidden)
		accuracyValue *= 1.08;
	// flashlight bonus
	if (score.modsLegacy & OsuReplay::Mods::Flashlight)
		accuracyValue *= 1.02;

	return accuracyValue;
}

double OsuDifficultyCalculator::calculateSpeedDeviation(const ScoreData &score, const Attributes &attributes, double timescale)
{
	if (score.countGreat + score.countGood + score.countMeh == 0)
		return std::numeric_limits<double>::quiet_NaN();

	double speedNoteCount = attributes.SpeedNoteCount;
	speedNoteCount += (score.totalHits - attributes.SpeedNoteCount) * 0.1;

	double relevantCountMiss = std::min((double)score.countMiss, speedNoteCount);
	double relevantCountMeh = std::min((double)score.countMeh, speedNoteCount - relevantCountMiss);
	double relevantCountOk = std::min((double)score.countGood, speedNoteCount - relevantCountMiss - relevantCountMeh);
	double relevantCountGreat = std::max(0.0, speedNoteCount - relevantCountMiss - relevantCountMeh - relevantCountOk);

	return calculateDeviation(attributes, timescale, relevantCountGreat, relevantCountOk, relevantCountMeh, relevantCountMiss);
}

double OsuDifficultyCalculator::calculateDeviation(const Attributes &attributes, double timescale, double relevantCountGreat, double relevantCountOk, double relevantCountMeh, double relevantCountMiss)
{
	if (relevantCountGreat + relevantCountOk + relevantCountMeh <= 0.0)
		return std::numeric_limits<double>::quiet_NaN();

	const double greatHitWindow = OsuGameRules::getRawHitWindow300(attributes.OverallDifficulty) / timescale;
	const double okHitWindow = OsuGameRules::getRawHitWindow100(attributes.OverallDifficulty) / timescale;
	const double mehHitWindow = OsuGameRules::getRawHitWindow50(attributes.OverallDifficulty) / timescale;

	const double z = 2.32634787404;
	const double sqrt2 = 1.4142135623730951;
	const double sqrt3 = 1.7320508075688772;
	const double sqrt2OverPi = 0.7978845608028654;

	double objectCount = relevantCountGreat + relevantCountOk + relevantCountMeh + relevantCountMiss;
	double n = std::max(1.0, objectCount - relevantCountMiss - relevantCountMeh);
	double p = relevantCountGreat / n;
	double pLowerBound = (n * p + z * z / 2.0) / (n + z * z) - z / (n + z * z) * sqrt(n * p * (1.0 - p) + z * z / 4.0);
	double deviation = greatHitWindow / (sqrt2 * erfInv(pLowerBound));
	double randomValue = sqrt2OverPi * okHitWindow * std::exp(-0.5 * std::pow(okHitWindow / deviation, 2.0)) / (deviation * erf(okHitWindow / (sqrt2 * deviation)));
	deviation *= std::sqrt(1.0 - randomValue);

	double limitValue = okHitWindow / sqrt3;
	if (pLowerBound == 0.0 || randomValue >= 1.0 || deviation > limitValue)
		deviation = limitValue;

	double mehVariance = (mehHitWindow * mehHitWindow + okHitWindow * mehHitWindow + okHitWindow * okHitWindow) / 3.0;
	return std::sqrt(((relevantCountGreat + relevantCountOk) * std::pow(deviation, 2.0) + relevantCountMeh * mehVariance) / (relevantCountGreat + relevantCountOk + relevantCountMeh));
}

double OsuDifficultyCalculator::calculateSpeedHighDeviationNerf(const Attributes &attributes, double speedDeviation)
{
	if (std::isnan(speedDeviation))
		return 0.0;

	double speedValue = std::pow(5.0 * std::max(1.0, attributes.SpeedDifficulty / 0.0675) - 4.0, 3.0) / 100000.0;
	double excessSpeedDifficultyCutoff = 100.0 + 220.0 * std::pow(22.0 / speedDeviation, 6.5);
	if (speedValue <= excessSpeedDifficultyCutoff)
		return 1.0;

	const double scale = 50.0;
	double adjustedSpeedValue = scale * (std::log((speedValue - excessSpeedDifficultyCutoff) / scale + 1.0) + excessSpeedDifficultyCutoff / scale);
	double lerpVal = 1.0 - clamp<double>((speedDeviation - 22.0) / (27.0 - 22.0), 0.0, 1.0);
	adjustedSpeedValue = lerp<double>(adjustedSpeedValue, speedValue, lerpVal);

	return adjustedSpeedValue / speedValue;
}


double OsuDifficultyCalculator::erf(double x)
{
	switch (std::fpclassify(x))
	{
		case FP_INFINITE:
			return (x > 0) ? 1.0 : -1.0;
		case FP_NAN:
			return std::numeric_limits<double>::quiet_NaN();
		case FP_ZERO:
			return 0.0;
		default:
			return erfImp(x, false);
	}
}

double OsuDifficultyCalculator::erfInv(double z)
{
	if (z == 0.0)
		return 0.0;
	else if (z >= 1.0)
		return std::numeric_limits<double>::infinity();
	else if (z <= -1.0)
		return -std::numeric_limits<double>::infinity();

	if (z < 0.0)
		return erfInvImp(-z, 1.0 + z, -1.0);
	else
		return erfInvImp(z, 1.0 - z, 1.0);
}

double OsuDifficultyCalculator::erfImp(double z, bool invert)
{
	if (z < 0.0)
	{
		if (!invert)
			return -erfImp(-z, false);

		if (z < -0.5)
			return 2 - erfImp(-z, true);

		return 1.0 + erfImp(-z, false);
	}

	double result;
	if (z < 0.5)
	{
		static constexpr double erf_imp_an[] = {0.00337916709551257388990745, -0.00073695653048167948530905, -0.374732337392919607868241, 0.0817442448733587196071743, -0.0421089319936548595203468, 0.0070165709512095756344528, -0.00495091255982435110337458, 0.000871646599037922480317225};
		static constexpr double erf_imp_ad[] = {1, -0.218088218087924645390535, 0.412542972725442099083918, -0.0841891147873106755410271, 0.0655338856400241519690695, -0.0120019604454941768171266, 0.00408165558926174048329689, -0.000615900721557769691924509};

		if (z < 1e-10)
			result = (z * 1.125) + (z * 0.003379167095512573896158903121545171688);
		else
			result = (z * 1.125) + (z * evaluatePolynomial(z, erf_imp_an) / evaluatePolynomial(z, erf_imp_ad));
	}
	else if (z < 110)
	{
		invert = !invert;
		double r, b;

		if (z < 0.75)
		{
			static constexpr double erf_imp_bn[] = {-0.0361790390718262471360258, 0.292251883444882683221149, 0.281447041797604512774415, 0.125610208862766947294894, 0.0274135028268930549240776, 0.00250839672168065762786937};
			static constexpr double erf_imp_bd[] = {1, 1.8545005897903486499845, 1.43575803037831418074962, 0.582827658753036572454135, 0.124810476932949746447682, 0.0113724176546353285778481};
			r = evaluatePolynomial(z - 0.5, erf_imp_bn) / evaluatePolynomial(z - 0.5, erf_imp_bd);

			// NOTE: despite being assigned to a double, all of these are single-precision float literals in the original code
			b = 0.3440242112f;
		}
		else if (z < 1.25)
		{
			static constexpr double erf_imp_cn[] = {-0.0397876892611136856954425, 0.153165212467878293257683, 0.191260295600936245503129, 0.10276327061989304213645, 0.029637090615738836726027, 0.0046093486780275489468812, 0.000307607820348680180548455};
			static constexpr double erf_imp_cd[] = {1, 1.95520072987627704987886, 1.64762317199384860109595, 0.768238607022126250082483, 0.209793185936509782784315, 0.0319569316899913392596356, 0.00213363160895785378615014};
			r = evaluatePolynomial(z - 0.75, erf_imp_cn) / evaluatePolynomial(z - 0.75, erf_imp_cd);
			b = 0.419990927f;
		}
		else if (z < 2.25)
		{
			static constexpr double erf_imp_dn[] = {-0.0300838560557949717328341, 0.0538578829844454508530552, 0.0726211541651914182692959, 0.0367628469888049348429018, 0.00964629015572527529605267, 0.00133453480075291076745275, 0.778087599782504251917881e-4};
			static constexpr double erf_imp_dd[] = {1, 1.75967098147167528287343, 1.32883571437961120556307, 0.552528596508757581287907, 0.133793056941332861912279, 0.0179509645176280768640766, 0.00104712440019937356634038, -0.106640381820357337177643e-7};
			r = evaluatePolynomial(z - 1.25, erf_imp_dn) / evaluatePolynomial(z - 1.25, erf_imp_dd);
			b = 0.4898625016f;
		}
		else if (z < 3.5)
		{
			static constexpr double erf_imp_en[] = {-0.0117907570137227847827732, 0.014262132090538809896674, 0.0202234435902960820020765, 0.00930668299990432009042239, 0.00213357802422065994322516, 0.00025022987386460102395382, 0.120534912219588189822126e-4};
			static constexpr double erf_imp_ed[] = {1, 1.50376225203620482047419, 0.965397786204462896346934, 0.339265230476796681555511, 0.0689740649541569716897427, 0.00771060262491768307365526, 0.000371421101531069302990367};
			r = evaluatePolynomial(z - 2.25, erf_imp_en) / evaluatePolynomial(z - 2.25, erf_imp_ed);
			b = 0.5317370892f;
		}
		else if (z < 5.25)
		{
			static constexpr double erf_imp_fn[] = {-0.00546954795538729307482955, 0.00404190278731707110245394, 0.0054963369553161170521356, 0.00212616472603945399437862, 0.000394984014495083900689956, 0.365565477064442377259271e-4, 0.135485897109932323253786e-5};
			static constexpr double erf_imp_fd[] = {1, 1.21019697773630784832251, 0.620914668221143886601045, 0.173038430661142762569515, 0.0276550813773432047594539, 0.00240625974424309709745382, 0.891811817251336577241006e-4, -0.465528836283382684461025e-11};
			r = evaluatePolynomial(z - 3.5, erf_imp_fn) / evaluatePolynomial(z - 3.5, erf_imp_fd);
			b = 0.5489973426f;
		}
		else if (z < 8)
		{
			static constexpr double erf_imp_gn[] = {-0.00270722535905778347999196, 0.0013187563425029400461378, 0.00119925933261002333923989, 0.00027849619811344664248235, 0.267822988218331849989363e-4, 0.923043672315028197865066e-6};
			static constexpr double erf_imp_gd[] = {1, 0.814632808543141591118279, 0.268901665856299542168425, 0.0449877216103041118694989, 0.00381759663320248459168994, 0.000131571897888596914350697, 0.404815359675764138445257e-11};
			r = evaluatePolynomial(z - 5.25, erf_imp_gn) / evaluatePolynomial(z - 5.25, erf_imp_gd);
			b = 0.5571740866f;
		}
		else if (z < 11.5)
		{
			static constexpr double erf_imp_hn[] = {-0.00109946720691742196814323, 0.000406425442750422675169153, 0.000274499489416900707787024, 0.465293770646659383436343e-4, 0.320955425395767463401993e-5, 0.778286018145020892261936e-7};
			static constexpr double erf_imp_hd[] = {1, 0.588173710611846046373373, 0.139363331289409746077541, 0.0166329340417083678763028, 0.00100023921310234908642639, 0.24254837521587225125068e-4};
			r = evaluatePolynomial(z - 8, erf_imp_hn) / evaluatePolynomial(z - 8, erf_imp_hd);
			b = 0.5609807968f;
		}
		else if (z < 17)
		{
			static constexpr double erf_imp_in[] = {-0.00056907993601094962855594, 0.000169498540373762264416984, 0.518472354581100890120501e-4, 0.382819312231928859704678e-5, 0.824989931281894431781794e-7};
			static constexpr double erf_imp_id[] = {1, 0.339637250051139347430323, 0.043472647870310663055044, 0.00248549335224637114641629, 0.535633305337152900549536e-4, -0.117490944405459578783846e-12};
			r = evaluatePolynomial(z - 11.5, erf_imp_in) / evaluatePolynomial(z - 11.5, erf_imp_id);
			b = 0.5626493692f;
		}
		else if (z < 24)
		{
			static constexpr double erf_imp_jn[] = {-0.000241313599483991337479091, 0.574224975202501512365975e-4, 0.115998962927383778460557e-4, 0.581762134402593739370875e-6, 0.853971555085673614607418e-8};
			static constexpr double erf_imp_jd[] = {1, 0.233044138299687841018015, 0.0204186940546440312625597, 0.000797185647564398289151125, 0.117019281670172327758019e-4};
			r = evaluatePolynomial(z - 17, erf_imp_jn) / evaluatePolynomial(z - 17, erf_imp_jd);
			b = 0.5634598136f;
		}
		else if (z < 38)
		{
			static constexpr double erf_imp_kn[] = {-0.000146674699277760365803642, 0.162666552112280519955647e-4, 0.269116248509165239294897e-5, 0.979584479468091935086972e-7, 0.101994647625723465722285e-8};
			static constexpr double erf_imp_kd[] = {1, 0.165907812944847226546036, 0.0103361716191505884359634, 0.000286593026373868366935721, 0.298401570840900340874568e-5};
			r = evaluatePolynomial(z - 24, erf_imp_kn) / evaluatePolynomial(z - 24, erf_imp_kd);
			b = 0.5638477802f;
		}
		else if (z < 60)
		{
			static constexpr double erf_imp_ln[] = {-0.583905797629771786720406e-4, 0.412510325105496173512992e-5, 0.431790922420250949096906e-6, 0.993365155590013193345569e-8, 0.653480510020104699270084e-10};
			static constexpr double erf_imp_ld[] = {1, 0.105077086072039915406159, 0.00414278428675475620830226, 0.726338754644523769144108e-4, 0.477818471047398785369849e-6};
			r = evaluatePolynomial(z - 38, erf_imp_ln) / evaluatePolynomial(z - 38, erf_imp_ld);
			b = 0.5640528202f;
		}
		else if (z < 85)
		{
			static constexpr double erf_imp_mn[] = {-0.196457797609229579459841e-4, 0.157243887666800692441195e-5, 0.543902511192700878690335e-7, 0.317472492369117710852685e-9};
			static constexpr double erf_imp_md[] = {1, 0.052803989240957632204885, 0.000926876069151753290378112, 0.541011723226630257077328e-5, 0.535093845803642394908747e-15};
			r = evaluatePolynomial(z - 60, erf_imp_mn) / evaluatePolynomial(z - 60, erf_imp_md);
			b = 0.5641309023f;
		}
		else
		{
			static constexpr double erf_imp_nn[] = {-0.789224703978722689089794e-5, 0.622088451660986955124162e-6, 0.145728445676882396797184e-7, 0.603715505542715364529243e-10};
			static constexpr double erf_imp_nd[] = {1, 0.0375328846356293715248719, 0.000467919535974625308126054, 0.193847039275845656900547e-5};
			r = evaluatePolynomial(z - 85, erf_imp_nn) / evaluatePolynomial(z - 85, erf_imp_nd);
			b = 0.5641584396f;
		}

		double g = std::exp(-z * z) / z;
		result = (g * b) + (g * r);
	}
	else
	{
		result = 0.0;
		invert = !invert;
	}

	if (invert)
		result = 1.0 - result;

	return result;
}

double OsuDifficultyCalculator::erfInvImp(double p, double q, double s)
{
	double result;

	if (p <= 0.5)
	{
		static constexpr double erv_inv_imp_an[] = {-0.000508781949658280665617, -0.00836874819741736770379, 0.0334806625409744615033, -0.0126926147662974029034, -0.0365637971411762664006, 0.0219878681111168899165, 0.00822687874676915743155, -0.00538772965071242932965};
		static constexpr double erv_inv_imp_ad[] = {1, -0.970005043303290640362, -1.56574558234175846809, 1.56221558398423026363, 0.662328840472002992063, -0.71228902341542847553, -0.0527396382340099713954, 0.0795283687341571680018, -0.00233393759374190016776, 0.000886216390456424707504};
		const float y = 0.0891314744949340820313f;
		double g = p * (p + 10);
		double r = evaluatePolynomial(p, erv_inv_imp_an) / evaluatePolynomial(p, erv_inv_imp_ad);
		result = (g * y) + (g * r);
	}
	else if (q >= 0.25)
	{
		static constexpr double erv_inv_imp_bn[] = {-0.202433508355938759655, 0.105264680699391713268, 8.37050328343119927838, 17.6447298408374015486, -18.8510648058714251895, -44.6382324441786960818, 17.445385985570866523, 21.1294655448340526258, -3.67192254707729348546};
		static constexpr double erv_inv_imp_bd[] = {1, 6.24264124854247537712, 3.9713437953343869095, -28.6608180499800029974, -20.1432634680485188801, 48.5609213108739935468, 10.8268667355460159008, -22.6436933413139721736, 1.72114765761200282724};
		const float y = 2.249481201171875f;
		double g = std::sqrt(-2.0 * std::log(q));
		double xs = q - 0.25;
		double r = evaluatePolynomial(xs, erv_inv_imp_bn) / evaluatePolynomial(xs, erv_inv_imp_bd);
		result = g / (y + r);
	}
	else
	{
		double x = std::sqrt(-std::log(q));

		if (x < 3)
		{
			static constexpr double erv_inv_imp_cn[] = {-0.131102781679951906451, -0.163794047193317060787, 0.117030156341995252019, 0.387079738972604337464, 0.337785538912035898924, 0.142869534408157156766, 0.0290157910005329060432, 0.00214558995388805277169, -0.679465575181126350155e-6, 0.285225331782217055858e-7, -0.681149956853776992068e-9};
			static constexpr double erv_inv_imp_cd[] = {1, 3.46625407242567245975, 5.38168345707006855425, 4.77846592945843778382, 2.59301921623620271374, 0.848854343457902036425, 0.152264338295331783612, 0.01105924229346489121};
			const float y = 0.807220458984375f;
			double xs = x - 1.125;
			double r = evaluatePolynomial(xs, erv_inv_imp_cn) / evaluatePolynomial(xs, erv_inv_imp_cd);
			result = (y * x) + (r * x);
		}
		else if (x < 6)
		{
			static constexpr double erv_inv_imp_dn[] = {-0.0350353787183177984712, -0.00222426529213447927281, 0.0185573306514231072324, 0.00950804701325919603619, 0.00187123492819559223345, 0.000157544617424960554631, 0.460469890584317994083e-5, -0.230404776911882601748e-9, 0.266339227425782031962e-11};
			static constexpr double erv_inv_imp_dd[] = {1, 1.3653349817554063097, 0.762059164553623404043, 0.220091105764131249824, 0.0341589143670947727934, 0.00263861676657015992959, 0.764675292302794483503e-4};
			const float y = 0.93995571136474609375f;
			double xs = x - 3;
			double r = evaluatePolynomial(xs, erv_inv_imp_dn) / evaluatePolynomial(xs, erv_inv_imp_dd);
			result = (y * x) + (r * x);
		}
		else if (x < 18)
		{
			static constexpr double erv_inv_imp_en[] = {-0.0167431005076633737133, -0.00112951438745580278863, 0.00105628862152492910091, 0.000209386317487588078668, 0.149624783758342370182e-4, 0.449696789927706453732e-6, 0.462596163522878599135e-8, -0.281128735628831791805e-13, 0.99055709973310326855e-16};
			static constexpr double erv_inv_imp_ed[] = {1, 0.591429344886417493481, 0.138151865749083321638, 0.0160746087093676504695, 0.000964011807005165528527, 0.275335474764726041141e-4, 0.282243172016108031869e-6};
			const float y = 0.98362827301025390625f;
			double xs = x - 6;
			double r = evaluatePolynomial(xs, erv_inv_imp_en) / evaluatePolynomial(xs, erv_inv_imp_ed);
			result = (y * x) + (r * x);
		}
		else if (x < 44)
		{
			static constexpr double erv_inv_imp_fn[] = {-0.0024978212791898131227, -0.779190719229053954292e-5, 0.254723037413027451751e-4, 0.162397777342510920873e-5, 0.396341011304801168516e-7, 0.411632831190944208473e-9, 0.145596286718675035587e-11, -0.116765012397184275695e-17};
			static constexpr double erv_inv_imp_fd[] = {1, 0.207123112214422517181, 0.0169410838120975906478, 0.000690538265622684595676, 0.145007359818232637924e-4, 0.144437756628144157666e-6, 0.509761276599778486139e-9};
			const float y = 0.99714565277099609375f;
			double xs = x - 18;
			double r = evaluatePolynomial(xs, erv_inv_imp_fn) / evaluatePolynomial(xs, erv_inv_imp_fd);
			result = (y * x) + (r * x);
		}
		else
		{
			static constexpr double erv_inv_imp_gn[] = {-0.000539042911019078575891, -0.28398759004727721098e-6, 0.899465114892291446442e-6, 0.229345859265920864296e-7, 0.225561444863500149219e-9, 0.947846627503022684216e-12, 0.135880130108924861008e-14, -0.348890393399948882918e-21};
			static constexpr double erv_inv_imp_gd[] = {1, 0.0845746234001899436914, 0.00282092984726264681981, 0.468292921940894236786e-4, 0.399968812193862100054e-6, 0.161809290887904476097e-8, 0.231558608310259605225e-11};
			const float y = 0.99941349029541015625f;
			double xs = x - 44;
			double r = evaluatePolynomial(xs, erv_inv_imp_gn) / evaluatePolynomial(xs, erv_inv_imp_gd);
			result = (y * x) + (r * x);
		}
	}

	return s * result;
}


OsuDifficultyCalculator::DiffObject::DiffObject(OsuDifficultyHitObject *base_object, float radius_scaling_factor, std::vector<DiffObject> &diff_objects, int prevObjectIdx) : objects(diff_objects)
{
	ho = base_object;

	for (int i=0; i<Skills::NUM_SKILLS; i++)
	{
		strains[i] = 0.0;
	}
	raw_speed_strain = 0.0;
	rhythm = 0.0;

	norm_start = ho->pos * radius_scaling_factor;

	angle = std::numeric_limits<float>::quiet_NaN();

	jumpDistance = 0.0;
	minJumpDistance = 0.0;
	minJumpTime = 0.0;
	travelDistance = 0.0;

	delta_time = 0.0;
	adjusted_delta_time = 0.0;

	lazyCalcFinished = false;
	lazyEndPos = ho->pos;
	lazyTravelDist = 0.0;
	lazyTravelTime = 0.0;
	travelTime = 0.0;

	prevObjectIndex = prevObjectIdx;
}

void OsuDifficultyCalculator::DiffObject::calculate_strains(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf)
{
	calculate_strain(prev, next, hitWindow300, autopilotNerf, Skills::Skill::SPEED);
	calculate_strain(prev, next, hitWindow300, autopilotNerf, Skills::Skill::AIM_SLIDERS);
	if (osu_stars_xexxar_angles_sliders.getBool())
		calculate_strain(prev, next, hitWindow300, autopilotNerf, Skills::Skill::AIM_NO_SLIDERS);
}

void OsuDifficultyCalculator::DiffObject::calculate_strain(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf, const Skills::Skill dtype)
{
	const double AimMultiplier = 26;
	const double SpeedMultiplier = 1.47;
	
	double currentStrainOfDiffObject = 0;

	const long time_elapsed = ho->time - prev.ho->time;

	// update our delta time
	delta_time = (double)time_elapsed;
	adjusted_delta_time = (double)std::max(time_elapsed, 25l);

	switch (ho->type)
	{
		case OsuDifficultyHitObject::TYPE::SLIDER:
		case OsuDifficultyHitObject::TYPE::CIRCLE:

			if (!osu_stars_xexxar_angles_sliders.getBool())
				currentStrainOfDiffObject = spacing_weight1((norm_start - prev.norm_start).length(), dtype);
			else
				currentStrainOfDiffObject = spacing_weight2(dtype, prev, next, hitWindow300, autopilotNerf);

			break;

		case OsuDifficultyHitObject::TYPE::SPINNER:
			break;

		case OsuDifficultyHitObject::TYPE::INVALID:
			// NOTE: silently ignore
			return;
	}

	if (!osu_stars_xexxar_angles_sliders.getBool())
		currentStrainOfDiffObject /= (double)std::max(time_elapsed, (long)50); // this has been removed, see https://github.com/Francesco149/oppai-ng/commit/5a1787ec0bd91b2bf686964b154228c68a99bf73

	// see Process() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
	double currentStrain = prev.strains[Skills::skillToIndex(dtype)];
	{
		currentStrain *= strainDecay(dtype, dtype == Skills::Skill::SPEED ? adjusted_delta_time : delta_time);
		currentStrain += currentStrainOfDiffObject * (dtype == Skills::Skill::SPEED ? SpeedMultiplier : AimMultiplier);
	}

	strains[Skills::skillToIndex(dtype)] = currentStrain;
}

double OsuDifficultyCalculator::DiffObject::calculate_difficulty(const Skills::Skill type, const DiffObject *dobjects, size_t dobjectCount, IncrementalState *incremental, std::vector<double> *outStrains, Attributes *attributes)
{
	// (old) see https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
	// (new) see https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/StrainSkill.cs

	static const double strain_step = 400.0;	// the length of each strain section
	static const double decay_weight = 0.9;		// max strains are weighted from highest to lowest, and this is how much the weight decays.

	if (dobjectCount < 1) return 0.0;

	double interval_end = incremental ? incremental->interval_end : (std::ceil((double)dobjects[0].ho->time / strain_step) * strain_step);
	double max_strain = incremental ? incremental->max_strain : 0.0;

	std::vector<double> highestStrains;
	std::vector<double> *highestStrainsRef = incremental ? &incremental->highest_strains : &highestStrains;
	std::vector<double> sliderStrains;
	std::vector<double> *sliderStrainsRef = incremental ? &incremental->slider_strains : &sliderStrains;
	for (size_t i=(incremental ? dobjectCount-1 : 0); i<dobjectCount; i++)
	{
		const DiffObject &cur = dobjects[i];
		const DiffObject &prev = dobjects[i > 0 ? i - 1 : i];

		// make previous peak strain decay until the current object
		while (cur.ho->time > interval_end)
		{
			if (incremental)
				highestStrainsRef->insert(std::upper_bound(highestStrainsRef->begin(), highestStrainsRef->end(), max_strain), max_strain);
			else
				highestStrainsRef->push_back(max_strain);

			// skip calculating strain decay for very long breaks (e.g. beatmap upload size limit hack diffs)
			// strainDecay with a base of 0.3 at 60 seconds is 4.23911583e-32, well below any meaningful difference even after being multiplied by object strain
			double strainDelta = interval_end - (double)prev.ho->time;
			if (i < 1 || strainDelta > 600000.0) // !prev
				max_strain = 0.0;
			else
				max_strain = prev.get_strain(type) * strainDecay(type, strainDelta);

			interval_end += strain_step;
		}

		// calculate max strain for this interval
		double cur_strain = cur.get_strain(type);
		max_strain = std::max(max_strain, cur_strain);

		// NOTE: this is done in StrainValueAt in lazer's code, but doing it here is more convenient for the incremental case
		if (type == Skills::Skill::AIM_SLIDERS && cur.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
			sliderStrainsRef->push_back(cur_strain);
	}

	// the peak strain will not be saved for the last section in the above loop
	if (incremental)
	{
		incremental->interval_end = interval_end;
		incremental->max_strain = max_strain;
		highestStrains.reserve(incremental->highest_strains.size() + 1); // required so insert call doesn't reallocate
		highestStrains = incremental->highest_strains;
		highestStrains.insert(std::upper_bound(highestStrains.begin(), highestStrains.end(), max_strain), max_strain);
	}
	else
		highestStrains.push_back(max_strain);

	if (outStrains != NULL)
		(*outStrains) = highestStrains; // save a copy

	if (attributes)
	{
		if (type == Skills::Skill::SPEED)
		{
			// calculate relevant speed note count
			// RelevantNoteCount @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
			const auto compareDiffObjects = [=] (const DiffObject& x, const DiffObject& y) {
				return (x.get_strain(type) < y.get_strain(type));
			};

			double maxObjectStrain;
			{
				if (incremental)
					maxObjectStrain = std::max(incremental->max_object_strain, dobjects[dobjectCount - 1].get_strain(type));
				else
					maxObjectStrain = (*std::max_element(dobjects, dobjects + dobjectCount, compareDiffObjects)).get_strain(type);
			}

			if (maxObjectStrain == 0.0 || !osu_stars_xexxar_angles_sliders.getBool())
				attributes->SpeedNoteCount = 0.0;
			else
			{
				double tempSum = 0.0;
				if (incremental && std::abs(incremental->max_object_strain - maxObjectStrain) < DIFFCALC_EPSILON)
				{
					incremental->speed_note_count += 1.0 / (1.0 + std::exp(-((dobjects[dobjectCount - 1].get_strain(type) / maxObjectStrain * 12.0) - 6.0)));
					tempSum = incremental->speed_note_count;
				}
				else
				{
					for (size_t i=0; i<dobjectCount; i++)
					{
						tempSum += 1.0 / (1.0 + std::exp(-((dobjects[i].get_strain(type) / maxObjectStrain * 12.0) - 6.0)));
					}

					if (incremental)
					{
						incremental->max_object_strain = maxObjectStrain;
						incremental->speed_note_count = tempSum;
					}
				}
				attributes->SpeedNoteCount = tempSum;
			}
		}
		else if (type == Skills::Skill::AIM_SLIDERS)
		{
			// calculate difficult sliders
			// GetDifficultSliders @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs
			const auto compareSliderObjects = [=] (const DiffObject& x, const DiffObject& y) {
				return (x.get_slider_aim_strain() < y.get_slider_aim_strain());
			};

			if (incremental && dobjects[dobjectCount - 1].ho->type != OsuDifficultyHitObject::TYPE::SLIDER)
				attributes->AimDifficultSliderCount = incremental->aim_difficult_slider_count;
			else
			{
				double maxSliderStrain;
				double curSliderStrain = incremental ? dobjects[dobjectCount - 1].strains[Skills::skillToIndex(Skills::Skill::AIM_SLIDERS)] : 0.0;
				{
					if (incremental)
					{
						incremental->slider_strains.push_back(curSliderStrain);
						maxSliderStrain = std::max(incremental->max_slider_strain, curSliderStrain);
					}
					else
						maxSliderStrain = (*std::max_element(dobjects, dobjects + dobjectCount, compareSliderObjects)).get_slider_aim_strain();
				}

				if (maxSliderStrain <= 0.0 || !osu_stars_xexxar_angles_sliders.getBool())
					attributes->AimDifficultSliderCount = 0.0;
				else
				{
					double tempSum = 0.0;
					if (incremental && std::abs(incremental->max_slider_strain - maxSliderStrain) < DIFFCALC_EPSILON)
					{
						incremental->aim_difficult_slider_count += 1.0 / (1.0 + std::exp(-((curSliderStrain / maxSliderStrain * 12.0) - 6.0)));
						tempSum = incremental->aim_difficult_slider_count;
					}
					else
					{
						if (incremental)
						{
							for (size_t i = 0; i < incremental->slider_strains.size(); i++)
							{
								tempSum += 1.0 / (1.0 + std::exp(-((incremental->slider_strains[i] / maxSliderStrain * 12.0) - 6.0)));
							}
							incremental->max_slider_strain = maxSliderStrain;
							incremental->aim_difficult_slider_count = tempSum;
						}
						else
						{
							for (size_t i = 0; i < dobjectCount; i++)
							{
								double sliderStrain = dobjects[i].get_slider_aim_strain();
								if (sliderStrain >= 0.0)
									tempSum += 1.0 / (1.0 + std::exp(-((sliderStrain / maxSliderStrain * 12.0) - 6.0)));
							}
						}
					}
					attributes->AimDifficultSliderCount = tempSum;
				}
			}
		}
	}

	// (old) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
	// (new) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/StrainSkill.cs
	// (new) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/OsuStrainSkill.cs

	static const size_t reducedSectionCount = 10;
	static const double reducedStrainBaseline = 0.75;

	double difficulty = 0.0;
	double weight = 1.0;

	// sort strains
	{
		// old implementation
		/*
		{
			// weigh the top strains
			for (size_t i=0; i<highestStrains.size(); i++)
			{
				difficulty += highestStrains[i] * weight;
				weight *= decay_weight;
			}
		}

		return difficulty;
		*/

		// new implementation
		// NOTE: lazer does this from highest to lowest, but sorting it in reverse lets the reduced top section loop below have a better average insertion time
		if (!incremental)
			std::sort(highestStrains.begin(), highestStrains.end());
	}

	// new implementation (https://github.com/ppy/osu/pull/13483/)
	{
		size_t skillSpecificReducedSectionCount = reducedSectionCount;
		{
			switch (type)
			{
			case Skills::Skill::SPEED:
				skillSpecificReducedSectionCount = 5;
				break;
			case Skills::Skill::AIM_SLIDERS:
			case Skills::Skill::AIM_NO_SLIDERS:
				break;
			}
		}

		// "We are reducing the highest strains first to account for extreme difficulty spikes"
		size_t actualReducedSectionCount = std::min(highestStrains.size(), skillSpecificReducedSectionCount);
		for (size_t i=0; i<actualReducedSectionCount; i++)
		{
			const double scale = std::log10(lerp<double>(1.0, 10.0, clamp<double>((double)i / (double)skillSpecificReducedSectionCount, 0.0, 1.0)));
			highestStrains[highestStrains.size() - i - 1] *= lerp<double>(reducedStrainBaseline, 1.0, scale);
		}

		// re-sort
		double reducedSections[reducedSectionCount]; // actualReducedSectionCount <= skillSpecificReducedSectionCount <= reducedSectionCount
		memcpy(reducedSections, &highestStrains[highestStrains.size() - actualReducedSectionCount], actualReducedSectionCount * sizeof(double));
		highestStrains.erase(highestStrains.end() - actualReducedSectionCount, highestStrains.end());
		for (size_t i=0; i<actualReducedSectionCount; i++)
		{
			highestStrains.insert(std::upper_bound(highestStrains.begin(), highestStrains.end(), reducedSections[i]), reducedSections[i]);
		}

		// weigh the top strains
		for (size_t i=0; i<highestStrains.size(); i++)
		{
			double last = difficulty;
			difficulty += highestStrains[highestStrains.size() - i - 1] * weight;
			weight *= decay_weight;
			if (std::abs(difficulty - last) < DIFFCALC_EPSILON)
				break;
		}
	}

	// see CountDifficultStrains @ https://github.com/ppy/osu/pull/16280/files#diff-07543a9ffe2a8d7f02cadf8ef7f81e3d7ec795ec376b2fff8bba7b10fb574e19R78
	if (attributes)
	{
		double& difficultStrainCount = type == Skills::Skill::SPEED ? attributes->SpeedDifficultStrainCount : attributes->AimDifficultStrainCount;

		if (difficulty == 0.0)
			difficultStrainCount = difficulty;
		else
		{
			double tempSum = 0.0;
			{
				double consistentTopStrain = difficulty / 10.0;

				if (incremental && std::abs(incremental->consistent_top_strain - consistentTopStrain) < DIFFCALC_EPSILON)
				{
					incremental->difficult_strains += 1.1 / (1.0 + std::exp(-10.0 * (dobjects[dobjectCount - 1].get_strain(type) / consistentTopStrain - 0.88)));
					tempSum = incremental->difficult_strains;
				}
				else
				{
					for (size_t i=0; i<dobjectCount; i++)
					{
						tempSum += 1.1 / (1.0 + std::exp(-10.0 * (dobjects[i].get_strain(type) / consistentTopStrain - 0.88)));
					}

					if (incremental)
					{
						incremental->consistent_top_strain = consistentTopStrain;
						incremental->difficult_strains = tempSum;
					}
				}
			}
			difficultStrainCount = tempSum;
		}
	}

	return difficulty;
}

// old implementation (ppv2.0)
double OsuDifficultyCalculator::DiffObject::spacing_weight1(const double distance, const Skills::Skill diff_type)
{
	// arbitrary tresholds to determine when a stream is spaced enough that is becomes hard to alternate.
	static const double single_spacing_threshold = 125.0;
	static const double stream_spacing = 110.0;

	// almost the normalized circle diameter (104px)
	static const double almost_diameter = 90.0;

	switch (diff_type)
	{
		case Skills::Skill::SPEED:
			if (distance > single_spacing_threshold)
				return 2.5;
			else if (distance > stream_spacing)
				return 1.6 + 0.9 * (distance - stream_spacing) / (single_spacing_threshold - stream_spacing);
			else if (distance > almost_diameter)
				return 1.2 + 0.4 * (distance - almost_diameter) / (stream_spacing - almost_diameter);
			else if (distance > almost_diameter / 2.0)
				return 0.95 + 0.25 * (distance - almost_diameter / 2.0) / (almost_diameter / 2.0);
			else
				return 0.95;

		case Skills::Skill::AIM_SLIDERS:
		case Skills::Skill::AIM_NO_SLIDERS:
			return std::pow(distance, 0.99);
	}

	return 0.0;
}

// new implementation, Xexxar, (ppv2.1), see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/
double OsuDifficultyCalculator::DiffObject::spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf)
{
	static const double single_spacing_threshold = 125.0;

	static const double min_speed_bonus = 75.0; /* ~200BPM 1/4 streams */
	static const double speed_balancing_factor = 40.0;
	static const double distance_multiplier = 0.8;

	static const int history_time_max = 5000;
	static const int history_objects_max = 32;
	static const double rhythm_overall_multiplier = 1.0;
	static const double rhythm_ratio_multiplier = 15.0;

	//double angle_bonus = 1.0; // (apparently unused now in lazer?)

	switch (diff_type)
	{
		case Skills::Skill::SPEED:
			{
				// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
				if (ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
				{
					raw_speed_strain = 0.0;
					rhythm = 0.0;

					return 0.0;
				}

				// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/SpeedEvaluator.cs
				const double distance = std::min(single_spacing_threshold, prev.travelDistance + minJumpDistance);

				double adjusted_delta_time = this->adjusted_delta_time;
				adjusted_delta_time /= clamp<double>((adjusted_delta_time / hitWindow300) / 0.93, 0.92, 1.0);

				double doubletapness = 1.0 - get_doubletapness(next, hitWindow300);

				double speed_bonus = 0.0;
				if (adjusted_delta_time < min_speed_bonus)
					speed_bonus = 0.75 * std::pow((min_speed_bonus - adjusted_delta_time) / speed_balancing_factor, 2.0);

				double distance_bonus = autopilotNerf ? 0.0 : std::pow(distance / single_spacing_threshold, 3.95) * distance_multiplier;
				raw_speed_strain = (1.0 + speed_bonus + distance_bonus) * 1000.0 * doubletapness / adjusted_delta_time;

				// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/RhythmEvaluator.cs
				double rhythmComplexitySum = 0;

				const double deltaDifferenceEpsilon = hitWindow300 * 0.3;

				RhythmIsland island{INT_MAX, 0};
				RhythmIsland previousIsland{INT_MAX, 0};

				std::vector<std::pair<RhythmIsland, int>> islandCounts;

				double startRatio = 0.0; // store the ratio of the current start of an island to buff for tighter rhythms

				bool firstDeltaSwitch = false;

				int historicalNoteCount = std::min(prevObjectIndex, history_objects_max);

				int rhythmStart = 0;

				while (rhythmStart < historicalNoteCount - 2 && ho->time - get_previous(rhythmStart)->ho->time < history_time_max)
				{
					rhythmStart++;
				}

				const DiffObject *prevObj = get_previous(rhythmStart);
				const DiffObject *lastObj = get_previous(rhythmStart + 1);

				for (int i=rhythmStart; i>0; i--)
				{
					const DiffObject *currObj = get_previous(i - 1);

					// scales note 0 to 1 from history to now
					double timeDecay = (history_time_max - (ho->time - currObj->ho->time)) / (double)history_time_max;
					double noteDecay = (double)(historicalNoteCount - i) / historicalNoteCount;

					double currHistoricalDecay = std::min(noteDecay, timeDecay); // either we're limited by time or limited by object count.

					double currDelta = std::max(currObj->delta_time, 1e-7);
					double prevDelta = std::max(prevObj->delta_time, 1e-7);
					double lastDelta = std::max(lastObj->delta_time, 1e-7);

					// calculate how much current delta difference deserves a rhythm bonus
					// this function is meant to reduce rhythm bonus for deltas that are multiples of each other (i.e 100 and 200)
					double deltaDifference = std::max(prevDelta, currDelta) / std::min(prevDelta, currDelta);

					// Take only the fractional part of the value since we're only interested in punishing multiples
					double deltaDifferenceFraction = deltaDifference - std::trunc(deltaDifference);

					double currRatio = 1.0 + rhythm_ratio_multiplier * std::min(0.5, smoothstepBellCurve(deltaDifferenceFraction));

					// reduce ratio bonus if delta difference is too big
					double differenceMultiplier = clamp<double>(2.0 - deltaDifference / 8.0, 0.0, 1.0);

					double windowPenalty = std::min(1.0, std::max(0.0, std::abs(prevDelta - currDelta) - deltaDifferenceEpsilon) / deltaDifferenceEpsilon);

					double effectiveRatio = windowPenalty * currRatio * differenceMultiplier;

					if (firstDeltaSwitch)
					{
						if (std::abs(prevDelta - currDelta) < deltaDifferenceEpsilon)
						{
							// island is still progressing
							if (island.delta == INT_MAX)
							{
								island.delta = std::max((int)currDelta, 25);
							}
							island.deltaCount++;
						}
						else
						{
							if (currObj->ho->type == OsuDifficultyHitObject::TYPE::SLIDER) // bpm change is into slider, this is easy acc window
								effectiveRatio *= 0.125;

							if (prevObj->ho->type == OsuDifficultyHitObject::TYPE::SLIDER) // bpm change was from a slider, this is easier typically than circle -> circle
								effectiveRatio *= 0.3;

							if (island.deltaCount % 2 == previousIsland.deltaCount % 2) // repeated island polarity (2 -> 4, 3 -> 5)
								effectiveRatio *= 0.5;

							if (lastDelta > prevDelta + deltaDifferenceEpsilon && prevDelta > currDelta + deltaDifferenceEpsilon) // previous increase happened a note ago, 1/1->1/2-1/4, dont want to buff this.
								effectiveRatio *= 0.125;

							if (previousIsland.deltaCount == island.deltaCount) // repeated island size (ex: triplet -> triplet)
								effectiveRatio *= 0.5;

							std::pair<RhythmIsland, int> *islandCount = nullptr;
							for (int i=0; i<islandCounts.size(); i++)
							{
								if (islandCounts[i].first.equals(island, deltaDifferenceEpsilon))
								{
									islandCount = &islandCounts[i];
									break;
								}
							}

							if (islandCount != NULL)
							{
								// only add island to island counts if they're going one after another
								if (previousIsland.equals(island, deltaDifferenceEpsilon))
									islandCount->second++;

								// repeated island (ex: triplet -> triplet)
								static const double E = 2.7182818284590451;
								double power = 2.75 / (1.0 + std::pow(E, 14.0 - (0.24 * island.delta)));
								effectiveRatio *= std::min(3.0 / islandCount->second, std::pow(1.0 / islandCount->second, power));
							}
							else
							{
								islandCounts.emplace_back(island, 1);
							}

							// scale down the difficulty if the object is doubletappable
							double doubletapness = prevObj->get_doubletapness(currObj, hitWindow300);
							effectiveRatio *= 1.0 - doubletapness * 0.75;

							rhythmComplexitySum += std::sqrt(effectiveRatio * startRatio) * currHistoricalDecay;

							startRatio = effectiveRatio;

							previousIsland = island;

							if (prevDelta + deltaDifferenceEpsilon < currDelta) // we're slowing down, stop counting
								firstDeltaSwitch = false; // if we're speeding up, this stays true and  we keep counting island size.

							island = RhythmIsland{std::max((int)currDelta, 25), 1};
						}
					}
					else if (prevDelta > currDelta + deltaDifferenceEpsilon) // we want to be speeding up.
					{
						// Begin counting island until we change speed again.
						firstDeltaSwitch = true;

						if (currObj->ho->type == OsuDifficultyHitObject::TYPE::SLIDER) // bpm change is into slider, this is easy acc window
							effectiveRatio *= 0.6;

						if (prevObj->ho->type == OsuDifficultyHitObject::TYPE::SLIDER) // bpm change was from a slider, this is easier typically than circle -> circle
							effectiveRatio *= 0.6;

						startRatio = effectiveRatio;

						island = RhythmIsland{std::max((int)currDelta, 25), 1};
					}

					lastObj = prevObj;
					prevObj = currObj;
				}

				rhythm = std::sqrt(4 + rhythmComplexitySum * rhythm_overall_multiplier) / 2.0; // produces multiplier that can be applied to strain. range [1, infinity) (not really though)
            	rhythm *= 1 - get_doubletapness(get_next(0), hitWindow300);

				return raw_speed_strain;
			}
			break;

		case Skills::Skill::AIM_SLIDERS:
		case Skills::Skill::AIM_NO_SLIDERS:
			{
				// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/AimEvaluator.cs
				static const double wide_angle_multiplier = 1.5;
				static const double acute_angle_multiplier = 2.55;
				static const double slider_multiplier = 1.35;
				static const double velocity_change_multiplier = 0.75;
				static const double wiggle_multiplier = 1.02;

				const bool withSliders = (diff_type == Skills::Skill::AIM_SLIDERS);

				if (ho->type == OsuDifficultyHitObject::TYPE::SPINNER || prevObjectIndex <= 1 || prev.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
					return 0.0;

				auto calcWideAngleBonus = [=] (double angle) {
					return smoothStep(angle, 40.0 * (PI / 180.0), 140.0 * (PI / 180.0));
				};
				auto calcAcuteAngleBonus = [=] (double angle) {
					return smoothStep(angle, 140.0 * (PI / 180.0), 40.0 * (PI / 180.0));
				};

				const DiffObject *prevPrev = get_previous(1);
				const DiffObject *prev2 = get_previous(2);

				double currVelocity = jumpDistance / adjusted_delta_time;

				if (prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER && withSliders)
				{
					double travelVelocity = prev.travelDistance / prev.travelTime;
					double movementVelocity = minJumpDistance / minJumpTime;
					currVelocity = std::max(currVelocity, movementVelocity + travelVelocity);
				}
				double aimStrain = currVelocity;

				double prevVelocity = prev.jumpDistance / prev.adjusted_delta_time;
				if (prevPrev->ho->type == OsuDifficultyHitObject::TYPE::SLIDER && withSliders)
				{
					double travelVelocity = prevPrev->travelDistance / prevPrev->travelTime;
					double movementVelocity = prev.minJumpDistance / prev.minJumpTime;
					prevVelocity = std::max(prevVelocity, movementVelocity + travelVelocity);
				}

				double wideAngleBonus = 0;
				double acuteAngleBonus = 0;
				double sliderBonus = 0;
				double velocityChangeBonus = 0;
				double wiggleBonus = 0;

				if (!std::isnan(angle) && !std::isnan(prev.angle))
				{
					double angleBonus = std::min(currVelocity, prevVelocity);

					if (std::max(adjusted_delta_time, prev.adjusted_delta_time) < 1.25 * std::min(adjusted_delta_time, prev.adjusted_delta_time))
					{
						acuteAngleBonus = calcAcuteAngleBonus(angle);
						acuteAngleBonus *= 0.08 + 0.92 * (1.0 - std::min(acuteAngleBonus, std::pow(calcAcuteAngleBonus(prev.angle), 3.0)));
						acuteAngleBonus *= angleBonus * smootherStep(60000.0 / (adjusted_delta_time * 2.0), 300.0, 400.0) * smootherStep(jumpDistance, 100.0, 200.0);
					}

					wideAngleBonus = calcWideAngleBonus(angle);
					wideAngleBonus *= 1.0 - std::min(wideAngleBonus, pow(calcWideAngleBonus(prev.angle), 3.0));
					wideAngleBonus *= angleBonus * smootherStep(jumpDistance, 0.0, 100.0);
					
					wiggleBonus = angleBonus
						* smootherStep(jumpDistance, 50.0, 100.0)
						* pow(reverseLerp(jumpDistance, 300.0, 100.0), 1.8)
						* smootherStep(angle, 110.0 * (PI / 180.0), 60.0 * (PI / 180.0))
						* smootherStep(prev.jumpDistance, 50.0, 100.0)
						* pow(reverseLerp(prev.jumpDistance, 300.0, 100.0), 1.8)
						* smootherStep(prev.angle, 110.0 * (PI / 180.0), 60.0 * (PI / 180.0));

					if (prev2 != NULL)
					{
						float distance = (prevPrev->ho->pos - prev2->ho->pos).length();

						if (distance < 1)
						{
							wideAngleBonus *= 1 - 0.35 * (1 - distance);
						}
					}
				}

				if (std::max(prevVelocity, currVelocity) != 0.0)
				{
					prevVelocity = (prev.jumpDistance + prevPrev->travelDistance) / prev.adjusted_delta_time;
					currVelocity = (jumpDistance + prev.travelDistance) / adjusted_delta_time;

					double distRatio = smoothStep(std::abs(prevVelocity - currVelocity) / std::max(prevVelocity, currVelocity), 0, 1);
					double overlapVelocityBuff = std::min(125.0 / std::min(adjusted_delta_time, prev.adjusted_delta_time), std::abs(prevVelocity - currVelocity));
					velocityChangeBonus = overlapVelocityBuff * distRatio * std::pow(std::min(adjusted_delta_time, prev.adjusted_delta_time) / std::max(adjusted_delta_time, prev.adjusted_delta_time), 2.0);
				}

				if (prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
					sliderBonus = prev.travelDistance / prev.travelTime;

				aimStrain += wiggleBonus * wiggle_multiplier;
				aimStrain += velocityChangeBonus * velocity_change_multiplier;
            	aimStrain += std::max(acuteAngleBonus * acute_angle_multiplier, wideAngleBonus * wide_angle_multiplier);

				if (withSliders)
					aimStrain += sliderBonus * slider_multiplier;

				return aimStrain;
			}
			break;
	}

	return 0.0;
}

double OsuDifficultyCalculator::DiffObject::get_doubletapness(const OsuDifficultyCalculator::DiffObject *next, double hitWindow300) const
{
	if (next != NULL)
	{
		double cur_delta = std::max(1.0, delta_time);
		double next_delta = std::max(1l, next->ho->time - ho->time); // next delta time isn't initialized yet
		double delta_diff = std::abs(next_delta - cur_delta);
		double speedRatio = cur_delta / std::max(cur_delta, delta_diff);
		double windowRatio = std::pow(std::min(1.0, cur_delta / hitWindow300), 2.0);

		return 1.0 - std::pow(speedRatio, 1.0 - windowRatio);
	}
	return 0.0;
}
