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



ConVar *OsuDifficultyCalculator::m_osu_slider_scorev2_ref = NULL;
ConVar *OsuDifficultyCalculator::m_osu_slider_end_inside_check_offset_ref = NULL;
ConVar *OsuDifficultyCalculator::m_osu_slider_curve_max_length_ref = NULL;

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool touchDevice, double *aim, double *aimSliderFactor, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains)
{
	std::atomic<bool> dead;
	dead = false;
	return calculateStarDiffForHitObjects(sortedHitObjects, CS, OD, speedMultiplier, relax, touchDevice, aim, aimSliderFactor, difficultAimStrains, speed, speedNotes, difficultSpeedStrains, upToObjectIndex, outAimStrains, outSpeedStrains, dead);
}

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool touchDevice, double *aim, double *aimSliderFactor, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead)
{
	std::vector<DiffObject> emptyCachedDiffObjects;
	return calculateStarDiffForHitObjectsInt(emptyCachedDiffObjects, sortedHitObjects, CS, OD, speedMultiplier, relax, touchDevice, aim, aimSliderFactor, difficultAimStrains, speed, speedNotes, difficultSpeedStrains, upToObjectIndex, NULL, outAimStrains, outSpeedStrains, dead);
}

double OsuDifficultyCalculator::calculateStarDiffForHitObjectsInt(std::vector<DiffObject> &cachedDiffObjects, std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool touchDevice, double *aim, double *aimSliderFactor, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex, IncrementalState* incremental, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead)
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
	DiffObject* diffObjects;
	if (!isUsingCachedDiffObjects)
	{
		// not cached (full rebuild computation)
		cachedDiffObjects.reserve(numDiffObjects);
		for (size_t i=0; i<numDiffObjects; i++) // respect upToObjectIndex!
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

					double cur_strain_time = (double)std::max(cur.ho->time - prev1.ho->time, 25l); // strain_time isn't initialized here
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
		for (size_t i=1; i<numDiffObjects; i++) // NOTE: start at 1
		{
			diffObjects[i].calculate_strains(diffObjects[i - 1], (i == numDiffObjects - 1) ? nullptr : &diffObjects[i + 1], hitWindow300);
		}
	}

	// calculate final difficulty (weigh strains)
	double aimNoSliders = osu_stars_xexxar_angles_sliders.getBool() ? DiffObject::calculate_difficulty(Skills::Skill::AIM_NO_SLIDERS, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::AIM_NO_SLIDERS] : NULL) : 0.0;
	*aim = DiffObject::calculate_difficulty(Skills::Skill::AIM_SLIDERS, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::AIM_SLIDERS] : NULL, outAimStrains, difficultAimStrains);
	*speed = DiffObject::calculate_difficulty(Skills::Skill::SPEED, diffObjects, numDiffObjects, incremental ? &incremental[(size_t)Skills::Skill::SPEED] : NULL, outSpeedStrains, difficultSpeedStrains, speedNotes);

	static const double star_scaling_factor = 0.0675;

	aimNoSliders = std::sqrt(aimNoSliders) * star_scaling_factor;
	*aim = std::sqrt(*aim) * star_scaling_factor;
	*speed = std::sqrt(*speed) * star_scaling_factor;

	*aimSliderFactor = (*aim > 0 && osu_stars_xexxar_angles_sliders.getBool()) ? aimNoSliders / *aim : 1.0;

	if (touchDevice)
		*aim = std::pow(*aim, 0.8);

	if (relax && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
	{
		*aim *= 0.9;
		*speed = 0.0;
	}

	return calculateTotalStarsFromSkills(*aim, *speed);
}

double OsuDifficultyCalculator::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double aimSliderFactor, double aimDifficultStrains, double speed, double speedNotes, double speedDifficultStrains, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
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

	return calculatePPv2(modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(), beatmap->getOD(), aim, aimSliderFactor, aimDifficultStrains, speed, speedNotes, speedDifficultStrains, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo, combo, misses, c300, c100, c50);
}

double OsuDifficultyCalculator::calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double aimSliderFactor, double aimDifficultStrains, double speed, double speedNotes, double speedDifficultStrains, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
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

	Attributes attributes;
	{
		attributes.AimStrain = aim;
		attributes.SliderFactor = aimSliderFactor;
		attributes.AimDifficultStrainCount = aimDifficultStrains;
		attributes.SpeedStrain = speed;
		attributes.SpeedNoteCount = speedNotes;
		attributes.SpeedDifficultStrainCount = speedDifficultStrains;
		attributes.ApproachRate = ar;
		attributes.OverallDifficulty = od;
		attributes.SliderCount = numSliders;
	}

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

	const double aimValue = computeAimValue(score, attributes, effectiveMissCount);
	const double speedValue = computeSpeedValue(score, attributes, effectiveMissCount);
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
	double baseAimPerformance = std::pow(5.0 * std::max(1.0, aim / 0.0675) - 4.0, 3.0) / 100000.0;
	double baseSpeedPerformance = std::pow(5.0 * std::max(1.0, speed / 0.0675) - 4.0, 3.0) / 100000.0;
	double basePerformance = std::pow(std::pow(baseAimPerformance, 1.1) + std::pow(baseSpeedPerformance, 1.1), 1.0 / 1.1);
	return basePerformance > 0.00001 ? 1.0476895531716472 /* Math.Cbrt(OsuPerformanceCalculator.PERFORMANCE_BASE_MULTIPLIER) */ * 0.027 * (std::cbrt(100000.0 / std::pow(2.0, 1 / 1.1) * basePerformance) + 4.0) : 0.0;
}



// https://github.com/ppy/osu-performance/blob/master/src/performance/osu/OsuScore.cpp
// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuPerformanceCalculator.cs

double OsuDifficultyCalculator::computeAimValue(const ScoreData &score, const OsuDifficultyCalculator::Attributes &attributes, double effectiveMissCount)
{
	double rawAim = attributes.AimStrain;

	double aimValue = std::pow(5.0 * std::max(1.0, rawAim / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	double lengthBonus = 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);
	aimValue *= lengthBonus;

	// miss penalty
	// see https://github.com/ppy/osu/pull/16280/
	if (effectiveMissCount > 0 && score.totalHits > 0)
		aimValue *= 0.96 / ((effectiveMissCount / (4.0 * std::pow(std::log(attributes.AimDifficultStrainCount), 0.94))) + 1.0);

	// ar bonus
	double approachRateFactor = 0.0; // see https://github.com/ppy/osu-performance/pull/125/
	if ((score.modsLegacy & OsuReplay::Relax) == 0 || osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
	{
		if (attributes.ApproachRate > 10.33)
			approachRateFactor = 0.3 * (attributes.ApproachRate - 10.33); // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/
		else if (attributes.ApproachRate < 8.0)
			approachRateFactor = 0.05 * (8.0 - attributes.ApproachRate); // from 0.01 to 0.1 see https://github.com/ppy/osu-performance/pull/125/ // and back again from 0.1 to 0.01 see https://github.com/ppy/osu-performance/pull/133/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/
	}

	aimValue *= 1.0 + approachRateFactor * lengthBonus;

	// hidden
	if (score.modsLegacy & OsuReplay::Mods::Hidden)
		aimValue *= 1.0 + 0.04 * (std::max(12.0 - attributes.ApproachRate, 0.0)); // NOTE: clamped to 0 because McOsu allows AR > 12

	// "We assume 15% of sliders in a map are difficult since there's no way to tell from the performance calculator."
	double estimateDifficultSliders = attributes.SliderCount * 0.15;
	if (attributes.SliderCount > 0)
	{
		double estimateSliderEndsDropped = clamp<double>((double)std::min(score.countGood + score.countMeh + score.countMiss, score.beatmapMaxCombo - score.scoreMaxCombo), 0.0, estimateDifficultSliders);
		double sliderNerfFactor = (1.0 - attributes.SliderFactor) * std::pow(1.0 - estimateSliderEndsDropped / estimateDifficultSliders, 3.0) + attributes.SliderFactor;
		aimValue *= sliderNerfFactor;
	}

	// scale aim with acc
	aimValue *= score.accuracy;
	// also consider acc difficulty when doing that
	aimValue *= 0.98 + std::pow(attributes.OverallDifficulty, 2.0) / 2500.0;

	return aimValue;
}

double OsuDifficultyCalculator::computeSpeedValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount)
{
	if ((score.modsLegacy & OsuReplay::Relax) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		return 0.0;

	double speedValue = std::pow(5.0 * std::max(1.0, attributes.SpeedStrain / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	double lengthBonus = 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);
	speedValue *= lengthBonus;

	// miss penalty
	// see https://github.com/ppy/osu/pull/16280/
	if (effectiveMissCount > 0)
		speedValue *= 0.96 / ((effectiveMissCount / (4.0 * std::pow(std::log(attributes.SpeedDifficultStrainCount), 0.94))) + 1.0);

	// ar bonus
	double approachRateFactor = 0.0; // see https://github.com/ppy/osu-performance/pull/125/
	if (attributes.ApproachRate > 10.33)
		approachRateFactor = 0.3 * (attributes.ApproachRate - 10.33); // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/

	speedValue *= 1.0 + approachRateFactor * lengthBonus;

	// hidden
	if (score.modsLegacy & OsuReplay::Mods::Hidden)
		speedValue *= 1.0 + 0.04 * (std::max(12.0 - attributes.ApproachRate, 0.0)); // NOTE: clamped to 0 because McOsu allows AR > 12

	// "Calculate accuracy assuming the worst case scenario"
	double relevantTotalDiff = score.totalHits - attributes.SpeedNoteCount;
	double relevantCountGreat = std::max(0.0, score.countGreat - relevantTotalDiff);
	double relevantCountOk = std::max(0.0, score.countGood - std::max(0.0, relevantTotalDiff - score.countGreat));
	double relevantCountMeh = std::max(0.0, score.countMeh - std::max(0.0, relevantTotalDiff - score.countGreat - score.countGood));
	double relevantAccuracy = attributes.SpeedNoteCount == 0 ? 0 : (relevantCountGreat * 6.0 + relevantCountOk * 2.0 + relevantCountMeh) / (attributes.SpeedNoteCount * 6.0);

	// see https://github.com/ppy/osu-performance/pull/128/
	// Scale the speed value with accuracy and OD
	speedValue *= (0.95 + std::pow(attributes.OverallDifficulty, 2.0) / 750.0) * std::pow((score.accuracy + relevantAccuracy) / 2.0, (14.5 - attributes.OverallDifficulty) / 2.0);
	// Scale the speed value with # of 50s to punish doubletapping.
	speedValue *= std::pow(0.99, score.countMeh < (score.totalHits / 500.0) ? 0.0 : score.countMeh - (score.totalHits / 500.0));

	return speedValue;
}

double OsuDifficultyCalculator::computeAccuracyValue(const ScoreData &score, const Attributes &attributes)
{
	if ((score.modsLegacy & OsuReplay::Relax) && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		return 0.0;

	double betterAccuracyPercentage;
	if (score.amountHitObjectsWithAccuracy > 0)
		betterAccuracyPercentage = ((double)(score.countGreat - (score.totalHits - score.amountHitObjectsWithAccuracy)) * 6.0 + (score.countGood * 2.0) + score.countMeh) / (double)(score.amountHitObjectsWithAccuracy * 6.0);
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
	strain_time = 0.0;

	lazyCalcFinished = false;
	lazyEndPos = ho->pos;
	lazyTravelDist = 0.0;
	lazyTravelTime = 0.0;
	travelTime = 0.0;

	prevObjectIndex = prevObjectIdx;
}

void OsuDifficultyCalculator::DiffObject::calculate_strains(const DiffObject &prev, const DiffObject *next, double hitWindow300)
{
	calculate_strain(prev, next, hitWindow300, Skills::Skill::SPEED);
	calculate_strain(prev, next, hitWindow300, Skills::Skill::AIM_SLIDERS);
	if (osu_stars_xexxar_angles_sliders.getBool())
		calculate_strain(prev, next, hitWindow300, Skills::Skill::AIM_NO_SLIDERS);
}

void OsuDifficultyCalculator::DiffObject::calculate_strain(const DiffObject &prev, const DiffObject *next, double hitWindow300, const Skills::Skill dtype)
{
	double currentStrainOfDiffObject = 0;

	const long time_elapsed = ho->time - prev.ho->time;

	// update our delta time
	delta_time = (double)time_elapsed;
	strain_time = (double)std::max(time_elapsed, 25l);

	switch (ho->type)
	{
		case OsuDifficultyHitObject::TYPE::SLIDER:
		case OsuDifficultyHitObject::TYPE::CIRCLE:

			if (!osu_stars_xexxar_angles_sliders.getBool())
				currentStrainOfDiffObject = spacing_weight1((norm_start - prev.norm_start).length(), dtype);
			else
				currentStrainOfDiffObject = spacing_weight2(dtype, prev, next, hitWindow300);

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
		currentStrain *= strainDecay(dtype, dtype == Skills::Skill::SPEED ? strain_time : delta_time);
		currentStrain += currentStrainOfDiffObject * weight_scaling[Skills::skillToIndex(dtype)];
	}
	strains[Skills::skillToIndex(dtype)] = currentStrain;
}

double OsuDifficultyCalculator::DiffObject::calculate_difficulty(const Skills::Skill type, const DiffObject *dobjects, size_t dobjectCount, IncrementalState *incremental, std::vector<double> *outStrains, double *outDifficultStrains, double *outRelevantNotes)
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
	{
		highestStrains.push_back(max_strain);
	}

	if (outStrains != NULL)
		(*outStrains) = highestStrains; // save a copy

	// calculate relevant speed note count
	// RelevantNoteCount @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
	if (outRelevantNotes)
	{
		const auto diffObjCompare = [=](const DiffObject& x, const DiffObject& y)
			{
				return x.get_strain(type)<y.get_strain(type);
			};
		double maxObjectStrain;
		if (incremental)
			maxObjectStrain = std::max(incremental->max_object_strain, dobjects[dobjectCount-1].get_strain(type));
		else
			maxObjectStrain = (*std::max_element(dobjects, dobjects + dobjectCount, diffObjCompare)).get_strain(type);
		if (maxObjectStrain == 0.0 || !osu_stars_xexxar_angles_sliders.getBool())
			*outRelevantNotes = 0.0;
		else
		{
			double tempSum = 0.0;
			if (incremental && std::abs(incremental->max_object_strain - maxObjectStrain) < DIFFCALC_EPSILON)
			{
				incremental->relevant_note_sum += 1.0 / (1.0 + std::exp(-(dobjects[dobjectCount-1].get_strain(type) / maxObjectStrain * 12.0 - 6.0)));
				tempSum = incremental->relevant_note_sum;
			}
			else
			{
				for (size_t i=0; i<dobjectCount; i++)
				{
					tempSum += 1.0 / (1.0 + std::exp(-(dobjects[i].get_strain(type) / maxObjectStrain * 12.0 - 6.0)));
				}
				if (incremental)
				{
					incremental->max_object_strain = maxObjectStrain;
					incremental->relevant_note_sum = tempSum;
				}
			}
			*outRelevantNotes = tempSum;
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
	// NOTE: lazer does this from highest to lowest, but sorting it in reverse lets the reduced top section loop below have a better average insertion time
	if (!incremental)
		std::sort(highestStrains.begin(), highestStrains.end());

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
			highestStrains[highestStrains.size()-i-1] *= lerp<double>(reducedStrainBaseline, 1.0, scale);
		}

		// re-sort
		double reducedSections[reducedSectionCount]; // actualReducedSectionCount <= skillSpecificReducedSectionCount <= reducedSectionCount
		memcpy(reducedSections, &highestStrains[highestStrains.size() - actualReducedSectionCount], actualReducedSectionCount * sizeof(double));
		highestStrains.erase(highestStrains.end() - actualReducedSectionCount, highestStrains.end());
		for (size_t i=0; i<actualReducedSectionCount; i++)
			highestStrains.insert(std::upper_bound(highestStrains.begin(), highestStrains.end(), reducedSections[i]), reducedSections[i]);

		// weigh the top strains
		for (size_t i=0; i<highestStrains.size(); i++)
		{
			double last = difficulty;
			difficulty += highestStrains[highestStrains.size()-i-1] * weight;
			weight *= decay_weight;
			if (std::abs(difficulty - last) < DIFFCALC_EPSILON)
				break;
		}
	}

	// see CountDifficultStrains @ https://github.com/ppy/osu/pull/16280/files#diff-07543a9ffe2a8d7f02cadf8ef7f81e3d7ec795ec376b2fff8bba7b10fb574e19R78
	if (outDifficultStrains)
	{
		if (difficulty == 0.0)
		{
			*outDifficultStrains = difficulty;
		}
		else
		{
			double consistentTopStrain = difficulty / 10.0;
			double tempSum = 0.0;
			if (incremental && std::abs(incremental->consistent_top_strain - consistentTopStrain) < DIFFCALC_EPSILON)
			{
				incremental->difficult_strains += 1.1 / (1 + std::exp(-10 * (dobjects[dobjectCount-1].get_strain(type) / consistentTopStrain - 0.88)));
				tempSum = incremental->difficult_strains;
			}
			else
			{
				for (size_t i=0; i<dobjectCount; i++)
				{
					tempSum += 1.1 / (1 + std::exp(-10 * (dobjects[i].get_strain(type) / consistentTopStrain - 0.88)));
				}
				if (incremental)
				{
					incremental->consistent_top_strain = consistentTopStrain;
					incremental->difficult_strains = tempSum;
				}
			}
			*outDifficultStrains = tempSum;
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
double OsuDifficultyCalculator::DiffObject::spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next, double hitWindow300)
{
	static const double single_spacing_threshold = 125.0;

	static const double min_speed_bonus = 75.0; /* ~200BPM 1/4 streams */
	static const double speed_balancing_factor = 40.0;
	static const double distance_multiplier = 0.94;

	static const int history_time_max = 5000;
	static const int history_objects_max = 32;
	static const double rhythm_overall_multiplier = 0.95;
	static const double rhythm_ratio_multiplier = 12.0;

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

				double strain_time = this->strain_time;
				strain_time /= clamp<double>((strain_time / hitWindow300) / 0.93, 0.92, 1.0);

				double doubletapness = 1.0 - get_doubletapness(next, hitWindow300);

				double speed_bonus = 0.0;
				if (strain_time < min_speed_bonus)
					speed_bonus = 0.75 * std::pow((min_speed_bonus - strain_time) / speed_balancing_factor, 2.0);

				double distance_bonus = std::pow(distance / single_spacing_threshold, 3.95) * distance_multiplier;
				raw_speed_strain = (1.0 + speed_bonus + distance_bonus) * 1000.0 * doubletapness / strain_time;

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

					double currDelta = currObj->strain_time;
					double prevDelta = prevObj->strain_time;
					double lastDelta = lastObj->strain_time;

					// calculate how much current delta difference deserves a rhythm bonus
					// this function is meant to reduce rhythm bonus for deltas that are multiples of each other (i.e 100 and 200)
					double deltaDifferenceRatio = std::min(prevDelta, currDelta) / std::max(prevDelta, currDelta);
					double currRatio = 1.0 + rhythm_ratio_multiplier * std::min(0.5, std::pow(std::sin(PI / deltaDifferenceRatio), 2.0));

					// reduce ratio bonus if delta difference is too big
					double fraction = std::max(prevDelta / currDelta, currDelta / prevDelta);
					double fractionMultiplier = clamp<double>(2.0 - fraction / 8.0, 0.0, 1.0);

					double windowPenalty = std::min(1.0, std::max(0.0, std::abs(prevDelta - currDelta) - deltaDifferenceEpsilon) / deltaDifferenceEpsilon);

					double effectiveRatio = windowPenalty * currRatio * fractionMultiplier;

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

				rhythm = std::sqrt(4.0 + rhythmComplexitySum * rhythm_overall_multiplier) / 2.0;

				return raw_speed_strain;
			}
			break;

		case Skills::Skill::AIM_SLIDERS:
		case Skills::Skill::AIM_NO_SLIDERS:
			{
				// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/AimEvaluator.cs
				static const double wide_angle_multiplier = 1.5;
				static const double acute_angle_multiplier = 1.95;
				static const double slider_multiplier = 1.35;
				static const double velocity_change_multiplier = 0.75;

				const bool withSliders = (diff_type == Skills::Skill::AIM_SLIDERS);

				if (ho->type == OsuDifficultyHitObject::TYPE::SPINNER || prevObjectIndex <= 1 || prev.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
					return 0.0;

				auto calcWideAngleBonus = [] (double angle) {
					return std::pow(std::sin(3.0 / 4.0 * (std::min(5.0 / 6.0 * PI, std::max(PI / 6.0, angle)) - PI / 6.0)), 2.0);
				};
				auto calcAcuteAngleBonus = [=] (double angle) {
					return 1.0 - calcWideAngleBonus(angle);
				};

				const DiffObject *prevPrev = get_previous(1);
				double currVelocity = jumpDistance / strain_time;

				if (prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER && withSliders)
				{
					double travelVelocity = prev.travelDistance / prev.travelTime;
					double movementVelocity = minJumpDistance / minJumpTime;
					currVelocity = std::max(currVelocity, movementVelocity + travelVelocity);
				}
				double aimStrain = currVelocity;

				double prevVelocity = prev.jumpDistance / prev.strain_time;
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

				if (std::max(strain_time, prev.strain_time) < 1.25 * std::min(strain_time, prev.strain_time))
				{
					if (!std::isnan(angle) && !std::isnan(prev.angle) && !std::isnan(prevPrev->angle))
					{
						double angleBonus = std::min(currVelocity, prevVelocity);

						wideAngleBonus = calcWideAngleBonus(angle);
						acuteAngleBonus = strain_time > 100 ? 0.0 : (
							calcAcuteAngleBonus(angle)
							* calcAcuteAngleBonus(prev.angle)
							* std::min(angleBonus, 125.0 / strain_time)
							* std::pow(std::sin(PI / 2.0 * std::min(1.0, (100.0 - strain_time) / 25.0)), 2.0)
							* std::pow(std::sin(PI / 2.0 * (clamp<double>(jumpDistance, 50.0, 100.0) - 50.0) / 50.0), 2.0)
						);

						wideAngleBonus *= angleBonus * (1.0 - std::min(wideAngleBonus, std::pow(calcWideAngleBonus(prev.angle), 3.0)));
						acuteAngleBonus *= 0.5 + 0.5 * (1.0 - std::min(acuteAngleBonus, std::pow(calcAcuteAngleBonus(prevPrev->angle), 3.0)));
					}
				}

				if (std::max(prevVelocity, currVelocity) != 0.0)
				{
					prevVelocity = (prev.jumpDistance + prevPrev->travelDistance) / prev.strain_time;
					currVelocity = (jumpDistance + prev.travelDistance) / strain_time;

					double distRatio = std::pow(std::sin(PI / 2.0 * std::abs(prevVelocity - currVelocity) / std::max(prevVelocity, currVelocity)), 2.0);
					double overlapVelocityBuff = std::min(125.0 / std::min(strain_time, prev.strain_time), std::abs(prevVelocity - currVelocity));
					velocityChangeBonus = overlapVelocityBuff * distRatio * std::pow(std::min(strain_time, prev.strain_time) / std::max(strain_time, prev.strain_time), 2.0);
				}

				if (prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
					sliderBonus = prev.travelDistance / prev.travelTime;

				aimStrain += std::max(acuteAngleBonus * acute_angle_multiplier, wideAngleBonus * wide_angle_multiplier + velocityChangeBonus * velocity_change_multiplier);
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