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

ConVar osu_stars_xexxar_angles_sliders("osu_stars_xexxar_angles_sliders", true, "completely enables/disables the new star/pp calc algorithm");
ConVar osu_stars_slider_curve_points_separation("osu_stars_slider_curve_points_separation", 20.0f, "massively reduce curve accuracy for star calculations to save memory/performance");



unsigned long long OsuDifficultyHitObject::sortHackCounter = 0;

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time) : OsuDifficultyHitObject(type, pos, time, time)
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime) : OsuDifficultyHitObject(type, pos, time, endTime, 0.0f, '\0', std::vector<Vector2>(), 0.0f, std::vector<long>(), true)
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<long> scoringTimes, bool calculateSliderCurveInConstructor)
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

        float progress = (float)(clamp<long>(pos, time, endTime)) / spanDuration;
        if (std::fmod(progress, 2.0f) >= 1.0f)
            progress = 1.0f - std::fmod(progress, 1.0f);
        else
            progress = std::fmod(progress, 1.0f);

        const Vector2 originalPointAt = curve->originalPointAt(progress);

        /*
        // MCKAY:
        {
        	// and delete it immediately afterwards (2)
        	if (scheduledCurveAlloc)
        		SAFE_DELETE(curve);
        }
		*/

        return originalPointAt;
	}
}



ConVar *OsuDifficultyCalculator::m_osu_slider_scorev2_ref = NULL;

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, double *aim, double *speed, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains)
{
	// NOTE: depends on speed multiplier + CS

	// NOTE: upToObjectIndex is applied way below, during the construction of the 'dobjects'

	// NOTE: osu always returns 0 stars for beatmaps with only 1 object, except if that object is a slider
	if (sortedHitObjects.size() < 2)
	{
		if (sortedHitObjects.size() < 1) return 0.0;
		if (sortedHitObjects[0].type != OsuDifficultyHitObject::TYPE::SLIDER) return 0.0;
	}

	// global independent variables/constants
	const float circleRadiusInOsuPixels = OsuGameRules::getRawHitCircleDiameter(clamp<float>(CS, 0.0f, 12.142f)) / 2.0f; // NOTE: clamped CS because McOsu allows CS > ~12.1429 (at which point the diameter becomes negative)

	// ****************************************************************************************************************************************** //



	// see setDistances() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

	static const float normalized_radius = 52.0f;		// normalization factor
	static const float circlesize_buff_treshold = 30;	// non-normalized diameter where the circlesize buff starts

	// multiplier to normalize positions so that we can calc as if everything was the same circlesize.
	// also handle high CS bonus

	float radius_scaling_factor = normalized_radius / circleRadiusInOsuPixels;
	if (circleRadiusInOsuPixels < circlesize_buff_treshold)
	{
		const float smallCircleBonus = std::min(circlesize_buff_treshold - circleRadiusInOsuPixels, 5.0f) / 50.0f;
		radius_scaling_factor *= 1.0f + smallCircleBonus;
	}



	static const int NUM_SKILLS = 2;

	class Skills
	{
	public:
		enum class Skill
		{
			SPEED,
			AIM
		};

		static int skillToIndex(const Skill skill)
		{
			switch (skill)
			{
			case Skill::SPEED:
				return 0;
			case Skill::AIM:
				return 1;
			}

			return 0;
		}
	};



	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs

	static const double decay_base[NUM_SKILLS] = {0.3, 0.15};			// how much strains decay per interval (if the previous interval's peak strains after applying decay are still higher than the current one's, they will be used as the peak strains).
	static const double weight_scaling[NUM_SKILLS] = {1400.0, 26.25};	// used to keep speed and aim balanced between eachother



	class DiffObject
	{
	public:
		OsuDifficultyHitObject *ho;

		double strains[NUM_SKILLS];

		Vector2 norm_start;		// start position normalized on radius

		double angle;			// precalc

		double jumpDistance;	// precalc
		double travelDistance;	// precalc

		double delta_time;		// strain temp

		bool lazyCalcFinished;	// precalc temp
		Vector2 lazyEndPos;		// precalc temp
		double lazyTravelDist;	// precalc temp

		DiffObject(OsuDifficultyHitObject *base_object, float radius_scaling_factor)
		{
			ho = base_object;

			// strains start at 1
			for (int i=0; i<NUM_SKILLS; i++)
			{
				strains[i] = 1.0;
			}

			norm_start = ho->pos * radius_scaling_factor;

			angle = std::numeric_limits<float>::quiet_NaN();

			jumpDistance = 0.0;
			travelDistance = 0.0;

			delta_time = 0.0;

			lazyCalcFinished = false;
			lazyEndPos = ho->pos;
			lazyTravelDist = 0.0;
		}

		void calculate_strains(const DiffObject &prev)
		{
			calculate_strain(prev, Skills::Skill::SPEED);
			calculate_strain(prev, Skills::Skill::AIM);
		}

		void calculate_strain(const DiffObject &prev, const Skills::Skill dtype)
		{
			double currentStrainOfDiffObject = 0;

			const long time_elapsed = ho->time - prev.ho->time;

			// update our delta time
			delta_time = (double)time_elapsed;

			switch (ho->type)
			{
				case OsuDifficultyHitObject::TYPE::SLIDER:
				case OsuDifficultyHitObject::TYPE::CIRCLE:

					if (!osu_stars_xexxar_angles_sliders.getBool())
						currentStrainOfDiffObject = spacing_weight1((norm_start - prev.norm_start).length(), dtype);
					else
						currentStrainOfDiffObject = spacing_weight2(dtype, jumpDistance, travelDistance, delta_time, prev.jumpDistance, prev.travelDistance, prev.delta_time, angle);

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
				currentStrain *= strainDecay(dtype, delta_time);
				currentStrain += currentStrainOfDiffObject * weight_scaling[Skills::skillToIndex(dtype)];
			}
			strains[Skills::skillToIndex(dtype)] = currentStrain;
		}

		static double calculate_difficulty(const Skills::Skill type, const std::vector<DiffObject> &dobjects, std::vector<double> *outStrains = NULL)
		{
			// (old) see https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
			// (new) see https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/StrainSkill.cs

			static const double strain_step = 400.0;	// the length of each strain section
			static const double decay_weight = 0.9;		// max strains are weighted from highest to lowest, and this is how much the weight decays.

			if (dobjects.size() < 1) return 0.0;

			double interval_end = std::ceil((double)dobjects[0].ho->time / strain_step) * strain_step;
			double max_strain = 0.0;

			std::vector<double> highestStrains;
			for (size_t i=0; i<dobjects.size(); i++)
			{
				const DiffObject &cur = dobjects[i];
				const DiffObject &prev = dobjects[i > 0 ? i - 1 : i];

				// make previous peak strain decay until the current object
				while (cur.ho->time > interval_end)
				{
					highestStrains.push_back(max_strain);

					if (i < 1) // !prev
						max_strain = 0.0;
					else
						max_strain = prev.strains[Skills::skillToIndex(type)] * strainDecay(type, (interval_end - (double)prev.ho->time));

					interval_end += strain_step;
				}

				// calculate max strain for this interval
				max_strain = std::max(max_strain, cur.strains[Skills::skillToIndex(type)]);
			}

			// the peak strain will not be saved for the last section in the above loop
			highestStrains.push_back(max_strain);

			if (outStrains != NULL)
				(*outStrains) = highestStrains; // save a copy



			// (old) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
			// (new) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/StrainSkill.cs
			// (new) see DifficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/OsuStrainSkill.cs

			static const size_t reducedSectionCount = 10;
			static const double reducedStrainBaseline = 0.75;
			static const double difficultyMultiplier = 1.06;

			double difficulty = 0.0;
			double weight = 1.0;

			// sort strains from greatest to lowest
			std::sort(highestStrains.begin(), highestStrains.end(), std::greater<double>());

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
					}
				}

				// "We are reducing the highest strains first to account for extreme difficulty spikes"
				for (size_t i=0; i<std::min(highestStrains.size(), skillSpecificReducedSectionCount); i++)
				{
					const double scale = std::log10(lerp<double>(1.0, 10.0, clamp<double>((double)i / (double)skillSpecificReducedSectionCount, 0.0, 1.0)));
					highestStrains[i] *= lerp<double>(reducedStrainBaseline, 1.0, scale);
				}

				// re-sort
				std::sort(highestStrains.begin(), highestStrains.end(), std::greater<double>());

				// weigh the top strains
				for (size_t i=0; i<highestStrains.size(); i++)
				{
					difficulty += highestStrains[i] * weight;
					weight *= decay_weight;
				}
			}

			double skillSpecificDifficultyMultiplier = difficultyMultiplier;
			{
				switch (type)
				{
				case Skills::Skill::SPEED:
					skillSpecificDifficultyMultiplier = 1.04;
					break;
				}
			}

			return difficulty * skillSpecificDifficultyMultiplier;
		}

		// old implementation (ppv2.0)
		static double spacing_weight1(const double distance, const Skills::Skill diff_type)
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

				case Skills::Skill::AIM:
					return std::pow(distance, 0.99);
			}

			return 0.0;
		}

		// new implementation, Xexxar, (ppv2.1), see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/
		static double spacing_weight2(const Skills::Skill diff_type, double jump_distance, double travel_distance, double delta_time, double prev_jump_distance, double prev_travel_distance, double prev_delta_time, double angle)
		{
			static const double single_spacing_threshold = 125.0;

			static const double pi_over_4 = PI / 4.0;
			static const double pi_over_2 = PI / 2.0;

			static const double max_speed_bonus = 45.0; /* ~330BPM 1/4 streams */
			static const double min_speed_bonus = 75.0; /* ~200BPM 1/4 streams */
			static const double speed_balancing_factor = 40.0;

			static const double angle_bonus_scale = 90.0;
			static const double aim_timing_threshold = 107.0;
			static const double speed_angle_bonus_begin = (5.0 * PI / 6.0);
			static const double aim_angle_bonus_begin = (PI / 3.0);

			// "Every strain interval is hard capped at the equivalent of 375 BPM streaming speed as a safety measure"
			const double strain_time = std::max(delta_time, 50.0);
			const double prev_strain_time = std::max(prev_delta_time, 50.0);

			double angle_bonus = 1.0;

			switch (diff_type)
			{
				case Skills::Skill::SPEED:
					{
						// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs

						const double distance = std::min(single_spacing_threshold, travel_distance + jump_distance);
						delta_time = std::max(delta_time, max_speed_bonus);

						double speed_bonus = 1.0;
						if (delta_time < min_speed_bonus)
							speed_bonus = 1.0 + std::pow((min_speed_bonus - delta_time) / speed_balancing_factor, 2.0);

						if (!std::isnan(angle) && angle < speed_angle_bonus_begin)
						{
							angle_bonus = 1.0 + std::pow(std::sin(1.5 * (speed_angle_bonus_begin - angle)), 2.0) / 3.57;

							if (angle < pi_over_2)
							{
								angle_bonus = 1.28;
								if (distance < angle_bonus_scale && angle < pi_over_4)
									angle_bonus += (1.0 - angle_bonus) * std::min((angle_bonus_scale - distance) / 10.0, 1.0);
								else if (distance < angle_bonus_scale)
									angle_bonus += (1.0 - angle_bonus) * std::min((angle_bonus_scale - distance) / 10.0, 1.0) * std::sin((pi_over_2 - angle) / pi_over_4);
							}
						}

						return (1.0 + (speed_bonus - 1.0) * 0.75) * angle_bonus * (0.95 + speed_bonus * std::pow(distance / single_spacing_threshold, 3.5)) / strain_time;
					}
					break;

				case Skills::Skill::AIM:
					{
						// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs

						double result = 0.0;

						if (!std::isnan(angle) && angle > aim_angle_bonus_begin)
						{
							angle_bonus = std::sqrt(
									std::max(prev_jump_distance - angle_bonus_scale, 0.0)
									* std::pow(std::sin(angle - aim_angle_bonus_begin), 2.0)
									* std::max(jump_distance - angle_bonus_scale, 0.0)
							);

							result = 1.4 * applyDiminishingExp(std::max(0.0, angle_bonus)) / std::max(aim_timing_threshold, prev_strain_time); // reduced from 1.5 to 1.4 in https://github.com/ppy/osu/pull/13483/
						}

						const double jumpDistanceExp = applyDiminishingExp(jump_distance);
						const double travelDistanceExp = applyDiminishingExp(travel_distance);

						const double sqrtTravelMulJump = std::sqrt(travelDistanceExp * jumpDistanceExp);

						return std::max(
								result + (jumpDistanceExp + travelDistanceExp + sqrtTravelMulJump) / std::max(strain_time, aim_timing_threshold),
										 (jumpDistanceExp + travelDistanceExp + sqrtTravelMulJump) / strain_time
						);
					}
					break;
			}

			return 0.0;
		}

		inline static double applyDiminishingExp(double val)
		{
			return std::pow(val, 0.99);
		}

		inline static double strainDecay(Skills::Skill type, double ms)
		{
			return std::pow(decay_base[Skills::skillToIndex(type)], ms / 1000.0);
		}
	};

	// ****************************************************************************************************************************************** //

	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

	class DistanceCalc
	{
	public:
		static void computeSliderCursorPosition(DiffObject &slider, float circleRadius)
		{
			if (slider.lazyCalcFinished || slider.ho->curve == NULL) return;

			// (slider.lazyEndPos is already initialized to ho->pos in DiffObject constructor)
			const float approxFollowCircleRadius = (float)(circleRadius * 3.0f); // NOTE: this should actually be * 2.4, i.e. osu_slider_followcircle_size_multiplier

			const size_t numScoringTimes = slider.ho->scoringTimes.size();
			for (size_t i=0; i<numScoringTimes; i++)
			{
                float progress = (float)(clamp<long>(slider.ho->scoringTimes[i] - slider.ho->time, 0, slider.ho->getDuration())) / slider.ho->spanDuration;
                if (std::fmod(progress, 2.0f) >= 1.0f)
                    progress = 1.0f - std::fmod(progress, 1.0f);
                else
                    progress = std::fmod(progress, 1.0f);

                Vector2 diff = slider.ho->curve->pointAt(progress) - slider.lazyEndPos;
                float dist = diff.length();

                if (dist > approxFollowCircleRadius)
                {
                    // the cursor would be outside the follow circle, we need to move it
                    diff.normalize(); // obtain direction of diff
                    dist -= approxFollowCircleRadius;
                    slider.lazyEndPos += diff * dist;
                    slider.lazyTravelDist += dist;
                }
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
	std::vector<DiffObject> diffObjects;
	diffObjects.reserve((upToObjectIndex < 0) ? sortedHitObjects.size() : upToObjectIndex+1);
	for (size_t i=0; i<sortedHitObjects.size() && (upToObjectIndex < 0 || i < upToObjectIndex+1); i++) // respect upToObjectIndex!
	{
		diffObjects.push_back(DiffObject(&sortedHitObjects[i], radius_scaling_factor)); // this already initializes the angle to NaN
	}

	const int numDiffObjects = diffObjects.size();

	// calculate angles and travel/jump distances (before calculating strains)
	if (osu_stars_xexxar_angles_sliders.getBool())
	{
		const float starsSliderCurvePointsSeparation = osu_stars_slider_curve_points_separation.getFloat();
		for (size_t i=0; i<numDiffObjects; i++)
		{
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

				if (prev1.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
				{
					DistanceCalc::computeSliderCursorPosition(prev1, circleRadiusInOsuPixels);
					cur.travelDistance = prev1.lazyTravelDist * radius_scaling_factor;
				}

				const Vector2 lastCursorPosition = DistanceCalc::getEndCursorPosition(prev1, circleRadiusInOsuPixels);

				// don't need to jump to reach spinners
				if (cur.ho->type != OsuDifficultyHitObject::TYPE::SPINNER)
					cur.jumpDistance = (cur.norm_start - lastCursorPosition*radius_scaling_factor).length();

				// calculate angles
				if (i > 1)
				{
					DiffObject &prev2 = diffObjects[i - 2];

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

	// calculate strains/skills
	for (size_t i=1; i<numDiffObjects; i++) // NOTE: start at 1
	{
		diffObjects[i].calculate_strains(diffObjects[i - 1]);
	}

	// calculate final difficulty (weigh strains)
	*aim = DiffObject::calculate_difficulty(Skills::Skill::AIM, diffObjects, outAimStrains);
	*speed = DiffObject::calculate_difficulty(Skills::Skill::SPEED, diffObjects, outSpeedStrains);

	static const double star_scaling_factor = 0.0675;

	*aim = std::sqrt(*aim) * star_scaling_factor;
	*speed = std::sqrt(*speed) * star_scaling_factor;

	return calculateTotalStarsFromSkills(*aim, *speed);
}

double OsuDifficultyCalculator::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, int numHitObjects, int numCircles, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
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

	return calculatePPv2(modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(), beatmap->getOD(), aim, speed, numHitObjects, numCircles, numSpinners, maxPossibleCombo, combo, misses, c300, c100, c50);
}

double OsuDifficultyCalculator::calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double speed, int numHitObjects, int numCircles, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
{
	// NOTE: depends on active mods + OD + AR

	// not sure what's going on here, but osu is using some strange incorrect rounding (e.g. 13.5 ms for OD 11.08333 (for OD 10 with DT), which is incorrect because the 13.5 should get rounded down to 13 ms)
	/*
	// these would be the raw unrounded values (would need an extra function in OsuGameRules to calculate this with rounding for the pp calculation)
	double od = OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap);
	double ar = OsuGameRules::getApproachRateForSpeedMultiplier(beatmap);
	*/
	// so to get the correct pp values that players expect, with certain mods we use the incorrect method of calculating ar/od so that the final pp value will be """correct"""
	// thankfully this was already included in the oppai code. note that these incorrect values are only used while map-changing mods are active!
	if (timescale != 1.0f
			|| (modsLegacy & OsuReplay::Mods::Easy)
			|| (modsLegacy & OsuReplay::Mods::HardRock)
			|| (modsLegacy & OsuReplay::Mods::DoubleTime)
			|| (modsLegacy & OsuReplay::Mods::Nightcore)
			|| (modsLegacy & OsuReplay::Mods::HalfTime)) // if map-changing mods are active, use incorrect calculations
	{
		const float	od0_ms = OsuGameRules::getMinHitWindow300(),
					od10_ms = OsuGameRules::getMaxHitWindow300(),
					ar0_ms = OsuGameRules::getMinApproachTime(),
					ar5_ms = OsuGameRules::getMidApproachTime(),
					ar10_ms = OsuGameRules::getMaxApproachTime();

		const float	od_ms_step = (od0_ms - od10_ms) / 10.0f,
					ar_ms_step1 = (ar0_ms - ar5_ms) / 5.0f,
					ar_ms_step2 = (ar5_ms - ar10_ms) / 5.0f;

		// stats must be capped to 0-10 before HT/DT which bring them to a range of -4.42 to 11.08 for OD and -5 to 11 for AR
		float odms = od0_ms - std::ceil(od_ms_step * od);
		float arms = (ar <= 5.0 ? (ar0_ms - ar_ms_step1 *  ar) : (ar5_ms - ar_ms_step2 * (ar - 5)));
		odms = std::min(od0_ms, std::max(od10_ms, odms));
		arms = std::min(ar0_ms, std::max(ar10_ms, arms));

		// apply speed-changing mods
		odms /= timescale;
		arms /= timescale;

		// convert OD and AR back into their stat form
		od = (od0_ms - odms) / od_ms_step;
		ar = (arms > ar5_ms ? ((-(arms - ar0_ms)) / ar_ms_step1) : (5.0f + (-(arms - ar5_ms)) / ar_ms_step2));
	}

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
			score.amountHitObjectsWithAccuracy = (modsLegacy & OsuReplay::ScoreV2 ? score.totalHits : numCircles);
		}
	}

	Attributes attributes;
	{
		attributes.AimStrain = aim;
		attributes.SpeedStrain = speed;
		attributes.ApproachRate = ar;
		attributes.OverallDifficulty = od;
	}

	// custom multipliers for nofail and spunout
	double multiplier = 1.12; // keep final pp normalized across changes
	{
		if (modsLegacy & OsuReplay::Mods::NoFail)
			multiplier *= std::max(0.9, 1.0 - 0.02 * score.countMiss); // see https://github.com/ppy/osu-performance/pull/127/files

		if ((modsLegacy & OsuReplay::Mods::SpunOut) && score.totalHits > 0)
			multiplier *= 1.0 - std::pow((double)numSpinners / (double)score.totalHits, 0.85); // see https://github.com/ppy/osu-performance/pull/110/
	}

	const double aimValue = computeAimValue(score, attributes);
	const double speedValue = computeSpeedValue(score, attributes);
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
	return (aim + speed + std::fabs(aim - speed) * 0.5);
}



// https://github.com/ppy/osu-performance/blob/master/src/performance/osu/OsuScore.cpp
// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuPerformanceCalculator.cs

double OsuDifficultyCalculator::computeAimValue(const ScoreData &score, const OsuDifficultyCalculator::Attributes &attributes)
{
	double rawAim = attributes.AimStrain;

	// touch nerf
	if (score.modsLegacy & OsuReplay::Mods::TouchDevice)
		rawAim = std::pow(rawAim, 0.8);

	double aimValue = std::pow(5.0 * std::max(1.0, rawAim / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	aimValue *= 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);

	// see https://github.com/ppy/osu-performance/pull/129/
	// Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any # of misses.
	if (score.countMiss > 0 && score.totalHits > 0)
		aimValue *= 0.97 * std::pow(1.0 - std::pow((double)score.countMiss / (double)score.totalHits, 0.775), (double)score.countMiss);

	// combo scaling
	if (score.beatmapMaxCombo > 0)
		aimValue *= std::min(std::pow((double)score.scoreMaxCombo, 0.8) / std::pow((double)score.beatmapMaxCombo, 0.8), 1.0);

	// ar bonus
	double approachRateFactor = 0.0; // see https://github.com/ppy/osu-performance/pull/125/
	if (attributes.ApproachRate > 10.33)
		approachRateFactor = attributes.ApproachRate - 10.33; // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/
	else if (attributes.ApproachRate < 8.0)
		approachRateFactor = 0.025 * (8.0 - attributes.ApproachRate); // from 0.01 to 0.1 see https://github.com/ppy/osu-performance/pull/125/ // and back again from 0.1 to 0.01 see https://github.com/ppy/osu-performance/pull/133/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/

	double approachRateTotalHitsFactor = 1.0 / (1.0 + std::exp(-(0.007 * (static_cast<double>(score.totalHits) - 400.0)))); // see https://github.com/ppy/osu-performance/pull/135/

	double approachRateBonus = 1.0 + (0.03 + 0.37 * approachRateTotalHitsFactor) * approachRateFactor; // see https://github.com/ppy/osu-performance/pull/135/ // see https://github.com/ppy/osu-performance/pull/137/

	// hidden
	if (score.modsLegacy & OsuReplay::Mods::Hidden)
		aimValue *= 1.0 + 0.04 * (std::max(12.0 - attributes.ApproachRate, 0.0)); // NOTE: clamped to 0 because McOsu allows AR > 12

	// flashlight
	double flashlightBonus = 1.0; // see https://github.com/ppy/osu-performance/pull/137/
	if (score.modsLegacy & OsuReplay::Mods::Flashlight)
	{
		flashlightBonus = 1.0 + 0.35 * std::min(1.0, (double)score.totalHits / 200.0)
			+ (score.totalHits > 200 ? 0.3 * std::min(1.0, (double)(score.totalHits - 200) / 300.0)
			+ (score.totalHits > 500 ? (double)(score.totalHits - 500) / 1200.0 : 0.0) : 0.0);
	}

	aimValue *= std::max(flashlightBonus, approachRateBonus); // see https://github.com/ppy/osu-performance/pull/137/

	// scale aim with acc slightly
	aimValue *= 0.5 + score.accuracy / 2.0;
	// also consider acc difficulty when doing that
	aimValue *= 0.98 + std::pow(attributes.OverallDifficulty, 2.0) / 2500.0;

	return aimValue;
}

double OsuDifficultyCalculator::computeSpeedValue(const ScoreData &score, const Attributes &attributes)
{
	double speedValue = std::pow(5.0 * std::max(1.0, attributes.SpeedStrain / 0.0675) - 4.0, 3.0) / 100000.0;

	// length bonus
	speedValue *= 0.95 + 0.4 * std::min(1.0, ((double)score.totalHits / 2000.0))
		+ (score.totalHits > 2000 ? std::log10(((double)score.totalHits / 2000.0)) * 0.5 : 0.0);

	// see https://github.com/ppy/osu-performance/pull/129/
	// Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any # of misses.
	if (score.countMiss > 0 && score.totalHits > 0)
		speedValue *= 0.97 * std::pow(1.0 - std::pow((double)score.countMiss / (double)score.totalHits, 0.775), std::pow((double)score.countMiss, 0.875));

	// combo scaling
	if (score.beatmapMaxCombo > 0)
		speedValue *= std::min(std::pow((double)score.scoreMaxCombo, 0.8) / std::pow((double)score.beatmapMaxCombo, 0.8), 1.0);

	// ar bonus
	double approachRateFactor = 0.0; // see https://github.com/ppy/osu-performance/pull/125/
	if (attributes.ApproachRate > 10.33)
		approachRateFactor = attributes.ApproachRate - 10.33; // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and completely changed the logic in https://github.com/ppy/osu-performance/pull/135/

	double approachRateTotalHitsFactor = 1.0 / (1.0 + std::exp(-(0.007 * (static_cast<double>(score.totalHits) - 400.0)))); // see https://github.com/ppy/osu-performance/pull/135/

	speedValue *= 1.0 + (0.03 + 0.37 * approachRateTotalHitsFactor) * approachRateFactor; // see https://github.com/ppy/osu-performance/pull/135/

	// hidden
	if (score.modsLegacy & OsuReplay::Mods::Hidden)
		speedValue *= 1.0 + 0.04 * (std::max(12.0 - attributes.ApproachRate, 0.0)); // NOTE: clamped to 0 because McOsu allows AR > 12

	// see https://github.com/ppy/osu-performance/pull/128/
	// Scale the speed value with accuracy and OD
	speedValue *= (0.95 + std::pow(attributes.OverallDifficulty, 2.0) / 750.0) * std::pow(score.accuracy, (14.5 - std::max(attributes.OverallDifficulty, 8.0)) / 2.0);
	// Scale the speed value with # of 50s to punish doubletapping.
	speedValue *= std::pow(0.98, score.countMeh < (score.totalHits / 500.0) ? 0.0 : score.countMeh - (score.totalHits / 500.0));

	return speedValue;
}

double OsuDifficultyCalculator::computeAccuracyValue(const ScoreData &score, const Attributes &attributes)
{
	double betterAccuracyPercentage;

	if (score.amountHitObjectsWithAccuracy > 0)
		betterAccuracyPercentage = ((double)(score.countGreat - (score.totalHits - score.amountHitObjectsWithAccuracy)) * 6 + score.countGood * 2 + score.countMeh) / (double)(score.amountHitObjectsWithAccuracy * 6);
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

