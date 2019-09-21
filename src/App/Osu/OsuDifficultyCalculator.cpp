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
bool tdEnabled = 0;

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time) : OsuDifficultyHitObject(type, pos, time, time)
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime) : OsuDifficultyHitObject(type, pos, time, endTime, 0.0f, '\0', std::vector<Vector2>(), 0.0f, std::vector<long>())
{
}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<long> scoringTimes)
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
	this->stack = 0;
	this->originalPos = this->pos;
	this->sortHack = sortHackCounter++;

	// build slider curve, if this is a (valid) slider
	if (this->type == TYPE::SLIDER && osu_stars_xexxar_angles_sliders.getBool() && controlPoints.size() > 1)
		this->curve = OsuSliderCurve::createCurve(this->osuSliderCurveType, controlPoints, this->pixelLength, osu_stars_slider_curve_points_separation.getFloat());
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
	this->stack = dobj.stack;
	this->originalPos = dobj.originalPos;
	this->sortHack = dobj.sortHack;

	// reset source
	dobj.curve = NULL;
}

void OsuDifficultyHitObject::updateStackPosition(float stackOffset)
{
	pos = originalPos - Vector2(stack * stackOffset, stack * stackOffset);

	if (curve != NULL)
		curve->updateStackPosition(stack * stackOffset, false);
}

Vector2 OsuDifficultyHitObject::getOriginalRawPosAt(long pos)
{
	if (type != TYPE::SLIDER || curve == NULL)
		return originalPos;
	else
	{
        float progress = (float)(clamp<long>(pos, time, endTime)) / spanDuration;
        if (std::fmod(progress, 2.0f) >= 1.0f)
            progress = 1.0f - std::fmod(progress, 1.0f);
        else
            progress = std::fmod(progress, 1.0f);

        return curve->originalPointAt(progress);
	}
}



ConVar *OsuDifficultyCalculator::m_osu_slider_scorev2_ref = NULL;

double OsuDifficultyCalculator::calculateStarDiffForHitObjects(std::vector<std::shared_ptr<OsuDifficultyHitObject>> &sortedHitObjects, float CS, double *aim, double *speed, int upToObjectIndex, bool hasTD)
{
	// NOTE: depends on speed multiplier + CS

	// NOTE: upToObjectIndex is applied way below, during the construction of the 'dobjects'

	// NOTE: osu always returns 0 stars for beatmaps with only 1 object, except if that object is a slider
	if (sortedHitObjects.size() < 2)
	{
		if (sortedHitObjects.size() < 1) return 0.0;
		if (sortedHitObjects[0]->type != OsuDifficultyHitObject::TYPE::SLIDER) return 0.0;
	}

	const float circleRadiusInOsuPixels = OsuGameRules::getRawHitCircleDiameter(CS) / 2.0f;

	// ****************************************************************************************************************************************** //

	// based on tom94's osu!tp aimod

	// how much strains decay per interval (if the previous interval's peak
	// strains after applying decay are still higher than the current one's,
	// they will be used as the peak strains).
	static const double decay_base[] = {0.3, 0.15};

	// almost the normalized circle diameter (104px)
	static const double almost_diameter = 90;

	// arbitrary tresholds to determine when a stream is spaced enough that is
	// becomes hard to alternate.
	static const double stream_spacing = 110;
	static const double single_spacing = 125;

	// used to keep speed and aim balanced between eachother
	static const double weight_scaling[] = {1400, 26.25};

	// non-normalized diameter where the circlesize buff starts
	static const float circlesize_buff_treshold = 30;

	// multiplier to normalize positions so that we can calc as if everything was the same circlesize. also handle high cs bonus.
	float radius_scaling_factor = 52.0f / circleRadiusInOsuPixels;
	if (circleRadiusInOsuPixels < circlesize_buff_treshold)
		radius_scaling_factor *= 1.0f + std::min((circlesize_buff_treshold - circleRadiusInOsuPixels), 5.0f) / 50.f;

	class cdiff
	{
	public:
		enum diff
		{
			speed = 0,
			aim = 1
		};

		static unsigned int diffToIndex(diff d)
		{
			switch (d)
			{
			case speed:
				return 0;
			case aim:
				return 1;
			}

			return 0;
		}
	};

	// diffcalc hit object
	class DiffObject
	{
	public:
		std::shared_ptr<OsuDifficultyHitObject> ho;

		double strains[2];

		Vector2 norm_start;		// start position normalized on radius

		double angle;			// precalc

		double jumpDistance;	// precalc
		double travelDistance;	// precalc

		float delta_time;		// strain temp

		bool lazyCalcFinished;	// precalc temp
		Vector2 lazyEndPos;		// precalc temp
		double lazyTravelDist;	// precalc temp

		DiffObject(std::shared_ptr<OsuDifficultyHitObject> base_object, float radius_scaling_factor)
		{
			ho = base_object;

			// strains start at 1
			strains[0] = 1.0;
			strains[1] = 1.0;

			norm_start = ho->pos * radius_scaling_factor;

			angle = std::numeric_limits<float>::quiet_NaN();

			jumpDistance = 0.0;
			travelDistance = 0.0;

			delta_time = 0.0f;

			lazyCalcFinished = false;
			lazyEndPos = ho->pos;
			lazyTravelDist = 0.0;
		}

		void calculate_strains(DiffObject &prev)
		{
			calculate_strain(prev, cdiff::diff::speed);
			calculate_strain(prev, cdiff::diff::aim);
		}

		void calculate_strain(DiffObject &prev, cdiff::diff dtype)
		{
			double res = 0;

			const long time_elapsed = ho->time - prev.ho->time;
			const double decay = std::pow(decay_base[cdiff::diffToIndex(dtype)], (double)time_elapsed / 1000.0);
			const double scaling = weight_scaling[cdiff::diffToIndex(dtype)];

			// update our delta time
			delta_time = time_elapsed;

			switch (ho->type) {
				case OsuDifficultyHitObject::TYPE::SLIDER:
				case OsuDifficultyHitObject::TYPE::CIRCLE:

					if (!osu_stars_xexxar_angles_sliders.getBool())
						res = spacing_weight1((norm_start - prev.norm_start).length(), dtype);
					else
						res = spacing_weight2(dtype, jumpDistance, travelDistance, delta_time, prev.jumpDistance, prev.travelDistance, prev.delta_time, angle);

					break;

				case OsuDifficultyHitObject::TYPE::SPINNER:
					break;

				case OsuDifficultyHitObject::TYPE::INVALID:
					// NOTE: silently ignore
					return;
			}

			res *= scaling;

			if (!osu_stars_xexxar_angles_sliders.getBool())
				res /= (double)std::max(time_elapsed, (long)50); // this has been removed, see https://github.com/Francesco149/oppai-ng/commit/5a1787ec0bd91b2bf686964b154228c68a99bf73

			strains[cdiff::diffToIndex(dtype)] = prev.strains[cdiff::diffToIndex(dtype)] * decay + res;
		}

		// old implementation (ppv2.0)
		double spacing_weight1(double distance, cdiff::diff diff_type)
		{
			switch (diff_type)
			{
				case cdiff::diff::speed:
					if (distance > single_spacing)
					{
						return 2.5;
					}
					else if (distance > stream_spacing)
					{
						return 1.6 + 0.9 *
							(distance - stream_spacing) /
							(single_spacing - stream_spacing);
					}
					else if (distance > almost_diameter)
					{
						return 1.2 + 0.4 * (distance - almost_diameter)
							/ (stream_spacing - almost_diameter);
					}
					else if (distance > almost_diameter / 2.0)
					{
						return 0.95 + 0.25 *
							(distance - almost_diameter / 2.0) /
							(almost_diameter / 2.0);
					}
					return 0.95;

				case cdiff::diff::aim:
					return std::pow(distance, 0.99);

				default:
					return 0.0;
			}
		}

		// new implementation, Xexxar, (ppv2.1), see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/
		static double spacing_weight2(cdiff::diff diff_type, float jump_distance, float travel_distance, float delta_time, float prev_jump_distance, float prev_travel_distance, float prev_delta_time, float angle)
		{
			const double pi_over_4 = PI / 4.0;
			const double pi_over_2 = PI / 2.0;

			const double max_speed_bonus = 45.0; /* ~330BPM 1/4 streams */
			const double min_speed_bonus = 75.0; /* ~200BPM 1/4 streams */
			const double speed_balancing_factor = 40.0;

			const double angle_bonus_scale = 90.0;
			const double aim_timing_threshold = 107.0;
			const double speed_angle_bonus_begin = (5.0 * PI / 6.0);
			const double aim_angle_bonus_begin = (PI / 3.0);

			const double strain_time = std::max(delta_time, 50.0f);

			double angle_bonus = 1.0;

			switch (diff_type)
			{
				case cdiff::diff::speed:
					{
						double speed_bonus = 1.0;

						const double distance = std::min((float)single_spacing, travel_distance + jump_distance);
						delta_time = std::max(delta_time, (float)max_speed_bonus);

						if (delta_time < min_speed_bonus)
							speed_bonus = 1.0 + std::pow((min_speed_bonus - delta_time) / speed_balancing_factor, 2.0);

						if (!std::isnan(angle) && angle < speed_angle_bonus_begin)
						{
							const double s = std::sin(1.5 * (speed_angle_bonus_begin - angle));

							angle_bonus = 1.0 + std::pow((double)s, 2.0) / 3.57;

							if (angle < pi_over_2)
							{
								angle_bonus = 1.28;
								if (distance < angle_bonus_scale && angle < pi_over_4)
								{
									angle_bonus += (1.0 - angle_bonus)
											* std::min((angle_bonus_scale - distance) / 10.0, 1.0);
								}
								else if (distance < angle_bonus_scale)
								{
									angle_bonus += (1.0 - angle_bonus)
											* std::min((angle_bonus_scale - distance) / 10.0, 1.0)
											* std::sin((pi_over_2 - angle) / pi_over_4);
								}
							}
						}

						return (1.0 + (speed_bonus - 1.0) * 0.75)
								* angle_bonus
								* (0.95 + speed_bonus * std::pow(distance / single_spacing, 3.5))
								/ strain_time;
					}
					break;

				case cdiff::diff::aim:
					{
						const double prev_strain_time = std::max(prev_delta_time, 50.0f);

						double result = 0.0;

						if (!std::isnan(angle) && angle > aim_angle_bonus_begin)
						{
							angle_bonus = std::sqrt(
									std::max(prev_jump_distance - angle_bonus_scale, 0.0)
									* std::pow(std::sin(angle - aim_angle_bonus_begin), 2.0)
									* std::max(jump_distance - angle_bonus_scale, 0.0)
							);

							result = 1.5 * std::pow((double)std::max(0.0, angle_bonus), 0.99) / std::max(aim_timing_threshold, prev_strain_time);
						}

						const double jumpDistanceExp = std::pow((double)jump_distance, 0.99);
						const double travelDistanceExp = std::pow((double)travel_distance, 0.99);

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
	};

	// ****************************************************************************************************************************************** //

	static const double star_scaling_factor = 0.0675;

	// strains are calculated by analyzing the map in chunks and then taking the peak strains in each chunk.
	// this is the length of a strain interval in milliseconds.
	static const double strain_step = 400.0;

	// max strains are weighted from highest to lowest, and this is how much the weight decays.
	static const double decay_weight = 0.9;

	class DiffCalc
	{
	public:
		static double calculate_difficulty(cdiff::diff type, std::vector<DiffObject> &dobjects)
		{
			if (dobjects.size() < 1) return 0.0;

			std::vector<double> highestStrains;

			// see Calculate() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuDifficultyCalculator.cs
			double interval_end = std::ceil((double)dobjects[0].ho->time / strain_step) * strain_step;
			double max_strain = 0.0;

			for (size_t i=0; i<dobjects.size(); i++)
			{
				DiffObject &cur = dobjects[i];
				DiffObject &prev = dobjects[i > 0 ? i - 1 : i];

				// make previous peak strain decay until the current object
				while (cur.ho->time > interval_end)
				{
					highestStrains.push_back(max_strain);

					if (i < 1) // !prev
						max_strain = 0.0;
					else
					{
						const double decay = std::pow(decay_base[cdiff::diffToIndex(type)], (interval_end - (double)prev.ho->time) / 1000.0);
						max_strain = prev.strains[cdiff::diffToIndex(type)] * decay;
					}

					interval_end += strain_step;
				}

				// calculate max strain for this interval
				max_strain = std::max(max_strain, cur.strains[cdiff::diffToIndex(type)]);
			}

			// the peak strain will not be saved for the last section in the above loop
			highestStrains.push_back(max_strain);

			// see difficultyValue() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Skill.cs
			double difficulty = 0.0;
			double weight = 1.0;

			// sort strains from greatest to lowest
			std::sort(highestStrains.begin(), highestStrains.end(), std::greater<double>());

			// weigh the top strains
			for (size_t i=0; i<highestStrains.size(); i++)
			{
				difficulty += weight * highestStrains[i];
				weight *= decay_weight;
			}

			return difficulty;
		}
	};

	// ****************************************************************************************************************************************** //

	class DistanceCalc // see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs
	{
	public:
		static void computeSliderCursorPosition(DiffObject &slider, float circleRadius)
		{
			if (slider.lazyCalcFinished || slider.ho->curve == NULL) return;

			// (slider.lazyEndPos is already initialized to ho->pos in DiffObject constructor)
			const float approxFollowCircleRadius = (float)(circleRadius * 3.0f); // NOTE: this should actually be * 2.4, i.e. osu_slider_followcircle_size_multiplier

			const int numScoringTimes = slider.ho->scoringTimes.size();
			for (int i=0; i<numScoringTimes; i++)
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
		diffObjects.push_back(DiffObject(sortedHitObjects[i], radius_scaling_factor)); // this already initializes the angle to NaN
	}

	const int numDiffObjects = diffObjects.size();

	// calculate angles and travel/jump distances (before calculating strains)
	if (osu_stars_xexxar_angles_sliders.getBool())
	{
		for (size_t i=0; i<numDiffObjects; i++)
		{
			// see setDistances() @ https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs
			if (i > 0)
			{
				// calculate travel/jump distances
				DiffObject &cur = diffObjects[i];
				DiffObject &prev1 = diffObjects[i - 1];

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

					const Vector2 v1 = lastLastCursorPosition - prev1.ho->pos;
					const Vector2 v2 = cur.ho->pos - lastCursorPosition;

					const double dot = v1.dot(v2);
					const double det = (v1.x * v2.y) - (v1.y * v2.x);

					cur.angle = std::fabs(std::atan2(det, dot));
				}
			}
		}
	}

	// calculate strains
	for (size_t i=1; i<numDiffObjects; i++) // start at 1
	{
		diffObjects[i].calculate_strains(diffObjects[i - 1]);
	}

	// calculate diff
	*aim = DiffCalc::calculate_difficulty(cdiff::diff::aim, diffObjects);
	*speed = DiffCalc::calculate_difficulty(cdiff::diff::speed, diffObjects);

	*aim = std::sqrt(*aim) * star_scaling_factor;
	*speed = std::sqrt(*speed) * star_scaling_factor;

	// TODO: touch nerf goes here
	if (hasTD)
		*aim = pow(*aim, 0.8f);

	return calculateTotalStarsFromSkills(*aim, *speed);
}

double OsuDifficultyCalculator::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50)
{
	// NOTE: depends on active mods + OD + AR

	if (m_osu_slider_scorev2_ref == NULL)
		m_osu_slider_scorev2_ref = convar->getConVarByName("osu_slider_scorev2");

	// get runtime score version
	const SCORE_VERSION scoreVersion = (m_osu_slider_scorev2_ref->getBool() || osu->getModScorev2()) ? SCORE_VERSION::SCORE_V2 : SCORE_VERSION::SCORE_V1;

	// get runtime mods
	int modsLegacy = 0;
	modsLegacy |= (osu->getModEZ() ? OsuReplay::Mods::Easy : 0);
	modsLegacy |= (osu->getModHD() ? OsuReplay::Mods::Hidden : 0);
	modsLegacy |= (osu->getModHR() ? OsuReplay::Mods::HardRock : 0);
	modsLegacy |= (osu->getModDT() ? OsuReplay::Mods::DoubleTime : 0);
	modsLegacy |= (osu->getModNC() ? OsuReplay::Mods::Nightcore : 0);
	modsLegacy |= (osu->getModHT() || osu->getModDC() ? OsuReplay::Mods::HalfTime : 0);
	modsLegacy |= (osu->getModNF() ? OsuReplay::Mods::NoFail : 0);
	modsLegacy |= (osu->getModSpunout() ? OsuReplay::Mods::SpunOut : 0);
	modsLegacy |= (osu->getModTD() ? OsuReplay::Mods::TouchDevice : 0);
	///modsLegacy |= (osu->getModFL() ? OsuReplay::Mods::Flashlight : 0); // TODO: not yet implemented

	return calculatePPv2(modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(), beatmap->getOD(), aim, speed, numHitObjects, numCircles, maxPossibleCombo, combo, misses, c300, c100, c50, scoreVersion);
}

double OsuDifficultyCalculator::calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, SCORE_VERSION scoreVersion)
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

		const float	od_ms_step = (od0_ms-od10_ms)/10.0f,
					ar_ms_step1 = (ar0_ms-ar5_ms)/5.0f,
					ar_ms_step2 = (ar5_ms-ar10_ms)/5.0f;

		// stats must be capped to 0-10 before HT/DT which bring them to a range of -4.42 to 11.08 for OD and -5 to 11 for AR
		float odms = od0_ms - std::ceil(od_ms_step * od);
		float arms = ar <= 5 ? (ar0_ms - ar_ms_step1 *  ar) : (ar5_ms - ar_ms_step2 * (ar - 5));
		odms = std::min(od0_ms, std::max(od10_ms, odms));
		arms = std::min(ar0_ms, std::max(ar10_ms, arms));

		// apply speed-changing mods
		odms /= timescale;
		arms /= timescale;

		// convert OD and AR back into their stat form
		od = (od0_ms - odms) / od_ms_step;
		ar = arms > ar5_ms ? ((-(arms - ar0_ms)) / ar_ms_step1) : (5.0f + (-(arms - ar5_ms)) / ar_ms_step2);
	}

	if (c300 < 0)
		c300 = numHitObjects - c100 - c50 - misses;

	if (combo < 0)
		combo = maxPossibleCombo;

	if (combo < 1)
		return 0.0f;

	int total_hits = c300 + c100 + c50 + misses;

	// accuracy (between 0 and 1)
	double acc = calculateAcc(c300, c100, c50, misses);

	// aim pp ------------------------------------------------------------------
	double aim_value = calculateBaseStrain(aim);

	// length bonus (reused in speed pp)
	double total_hits_over_2k = (double)total_hits / 2000.0;
	double length_bonus = 0.95
		+ 0.4 * std::min(1.0, total_hits_over_2k)
		+ (total_hits > 2000 ? std::log10(total_hits_over_2k) * 0.5 : 0.0);

	// miss penalty (reused in speed pp)
	double miss_penality = std::pow(0.97, (double)misses);

	// combo break penalty (reused in speed pp)
	double combo_break = std::min(std::pow((double)combo, 0.8) / std::pow((double)maxPossibleCombo, 0.8), 1.0);

	aim_value *= length_bonus;
	aim_value *= miss_penality;
	aim_value *= combo_break;

	double ar_bonus = 1.0;

	// high AR bonus
	if (ar > 10.33)
	{
		// https://github.com/ppy/osu-performance/pull/76/

		/*
		ar_bonus += 0.45 * (ar - 10.33);
		*/

		ar_bonus += 0.3 * (ar - 10.33);
	}
	else if (ar < 8.0) // low ar bonus
	{
		// https://github.com/ppy/osu-performance/pull/72/

		/*
		if (modsLegacy & OsuReplay::Mods::Hidden)
			low_ar_bonus *= 2.0;
		*/

		ar_bonus += 0.01 * (8.0 - ar);
	}

	aim_value *= ar_bonus;

	// hidden
	if (modsLegacy & OsuReplay::Mods::Hidden)
	{
		// https://github.com/ppy/osu-performance/pull/47/
		// https://github.com/ppy/osu-performance/pull/72/

		/*
		aim_value *= (1.02 + std::max(11.0 - ar, 0.0) / 50.0);
		*/

		aim_value *= 1.0 + 0.04 * (std::max(12.0 - ar, 0.0));
	}

	// flashlight
	// TODO: not yet implemented // TODO: https://github.com/ppy/osu-performance/pull/71/
	/*
	if (modsLegacy & OsuReplay::Mods::Flashlight)
		aim_value *= 1.45 * length_bonus;
	*/

	// acc bonus (bad aim can lead to bad acc, reused in speed for same reason)
	double acc_bonus = 0.5 + acc / 2.0;

	// od bonus (low od is easy to acc even with shit aim, reused in speed ...)
	double od_bonus = 0.98 + std::pow(od, 2.0) / 2500.0;

	aim_value *= acc_bonus;
	aim_value *= od_bonus;

	// speed pp ----------------------------------------------------------------
	double speed_value = calculateBaseStrain(speed);

	speed_value *= length_bonus;
	speed_value *= miss_penality;
	speed_value *= combo_break;

	ar_bonus = 1.0; // reset

	// https://github.com/ppy/osu-performance/pull/76/
	// high AR bonus
	if (ar > 10.33)
		ar_bonus += 0.3 * (ar - 10.33);

	speed_value *= ar_bonus;

	// hidden
	if (modsLegacy & OsuReplay::Mods::Hidden)
	{
		// https://github.com/ppy/osu-performance/pull/42/
		// https://github.com/ppy/osu-performance/pull/72/

		/*
		speed_value *= 1.18;
		*/

		// "We want to give more reward for lower AR when it comes to speed and HD. This nerfs high AR and buffs lower AR."
		speed_value *= 1.0 + 0.04 * (std::max(12.0 - ar, 0.0));
	}

	// https://github.com/ppy/osu-performance/pull/74/
	/*
	speed_value *= acc_bonus;
	speed_value *= od_bonus;
	*/
	speed_value *= 0.02 + acc;
	speed_value *= 0.96 + std::pow(od, 2.0) / 1600.0;



	// acc pp ------------------------------------------------------------------
	double real_acc = 0.0; // accuracy calculation changes from scorev1 to scorev2

	if (scoreVersion == SCORE_VERSION::SCORE_V2)
	{
		numCircles = total_hits;
		real_acc = acc;
	}
	else
	{
		// scorev1 ignores sliders since they are free 300s
		if (numCircles)
		{
			real_acc = (
					(c300 - (total_hits - numCircles)) * 300.0
					+ c100 * 100.0
					+ c50 * 50.0
					)
					/ (numCircles * 300);
		}

		// can go negative if we miss everything
		real_acc = std::max(0.0, real_acc);
	}

	// arbitrary values tom crafted out of trial and error
	double acc_value = std::pow(1.52163, od) * std::pow(real_acc, 24.0) * 2.83;

	// length bonus (not the same as speed/aim length bonus)
	acc_value *= std::min(1.15, std::pow(numCircles / 1000.0, 0.3));

	// hidden bonus
	if (modsLegacy & OsuReplay::Mods::Hidden)
	{
		// https://github.com/ppy/osu-performance/pull/72/

		/*
		acc_value *= 1.02;
		*/

		acc_value *= 1.08;
	}

	// flashlight bonus
	// TODO: not yet implemented
	/*
	if (modsLegacy & OsuReplay::Mods::Flashlight)
		acc_value *= 1.02;
	*/

	// total pp ----------------------------------------------------------------
	double final_multiplier = 1.12;

	// nofail
	if (modsLegacy & OsuReplay::Mods::NoFail)
		final_multiplier *= 0.90;

	// spunout
	if (modsLegacy & OsuReplay::Mods::SpunOut)
		final_multiplier *= 0.95;

	return	std::pow(
			std::pow(aim_value, 1.1) +
			std::pow(speed_value, 1.1) +
			std::pow(acc_value, 1.1),
			1.0 / 1.1
		) * final_multiplier;
}

double OsuDifficultyCalculator::calculateAcc(int c300, int c100, int c50, int misses)
{
	int total_hits = c300 + c100 + c50 + misses;
	double acc = 0.0f;

	if (total_hits > 0)
		acc = ((double)c50 * 50.0 + (double)c100 * 100.0 + (double)c300 * 300.0) / ((double)total_hits * 300.0);

	return acc;
}

double OsuDifficultyCalculator::calculateBaseStrain(double strain)
{
	return std::pow(5.0 * std::max(1.0, strain / 0.0675) - 4.0, 3.0) / 100000.0;
}

double OsuDifficultyCalculator::calculateTotalStarsFromSkills(double aim, double speed)
{
	static const double extreme_scaling_factor = 0.5;

	return (aim + speed + std::fabs(aim - speed) * extreme_scaling_factor);
}

