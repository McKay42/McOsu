//================ Copyright (c) 2019, PG & Francesco149 & Khangaroo & Givikap120, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//=======================================================================================================================//

#include "OsuDifficultyCalculator.h"

#include "Engine.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuGameRules.h"
#include "OsuReplay.h"

#include "OsuSliderCurves.h"

// early declaration for OsuLazerAdapter.h usage
ConVar osu_stars_slider_curve_points_separation("osu_stars_slider_curve_points_separation", 1.0f, FCVAR_NONE, "massively reduce curve accuracy for star calculations to save memory/performance");

#include "OsuLazerAdapter.h"
#include "OsuLazerDifficulty.h"
#include "OsuLazerPerformance.h"

#include <climits> // for INT_MAX

ConVar osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled("osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled", true, FCVAR_NONE, "disables relax/autopilot nerfs in star and pp calculations. since mcosu has always allowed these, the default is to not nerf them.");

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

	this->baseTime = time;
	this->baseEndTime = endTime;

	this->curve = NULL;
	this->scheduledCurveAlloc = false;
	this->scheduledCurveAllocStackOffset = 0.0f;
	this->repeats = repeats;

	this->stack = 0;
	this->originalPos = this->pos;
	this->sortHack = sortHackCounter++;

	// build slider curve, if this is a (valid) slider
	if (this->type == TYPE::SLIDER && controlPoints.size() > 1)
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
	this->baseTime = dobj.baseTime;
	this->baseEndTime = dobj.baseEndTime;
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
	this->baseTime = dobj.baseTime;
	this->baseEndTime = dobj.baseEndTime;
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


void OsuDifficultyCalculator::DifficultyAttributes::clear()
{
	lazer = OsuLazer::Attributes{};
	AimDifficulty = 0;
	AimDifficultSliderCount = 0;

	SpeedDifficulty = 0;
	SpeedNoteCount = 0;

	SliderFactor = 0;

	AimTopWeightedSliderFactor = 0;
	SpeedTopWeightedSliderFactor = 0;
	
	AimDifficultStrainCount = 0;
	SpeedDifficultStrainCount = 0;

	NestedScorePerObject = 0;
	LegacyScoreBaseMultiplier = 0;
	MaximumLegacyComboScore = 0;

	ApproachRate = 0;
	OverallDifficulty = 0;
	SliderCount = 0;
}

OsuDifficultyCalculator::BeatmapDiffcalcData::BeatmapDiffcalcData(const OsuBeatmap* beatmap, std::vector<OsuDifficultyHitObject> &loadedSortedHitObjects): sortedHitObjects(loadedSortedHitObjects)
{
	CS = beatmap->getCS();
	HP = beatmap->getHP();
	AR = beatmap->getAR(false);
	OD = beatmap->getOD(false);

	speedMultiplier = beatmap->getOsu()->getSpeedMultiplier(); // NOTE: not beatmap->getSpeedMultiplier()!
	hidden = beatmap->getOsu()->getModHD();
	touchDevice = beatmap->getOsu()->getModTD();
	relax = beatmap->getOsu()->getModRelax();
	autopilot = beatmap->getOsu()->getModAutopilot();

	breakDuration = beatmap->getBreakDurationTotal();
	playableLength = beatmap->getLengthPlayable();
}

OsuDifficultyCalculator::BeatmapDiffcalcData::BeatmapDiffcalcData(const OsuDatabaseBeatmap* beatmap, const OsuDatabase::Score &score, std::vector<OsuDifficultyHitObject> &loadedSortedHitObjects) : sortedHitObjects(loadedSortedHitObjects)
{
	const OsuReplay::BEATMAP_VALUES legacyValues = OsuReplay::getBeatmapValuesForModsLegacy(score.modsLegacy, beatmap->getAR(), beatmap->getCS(), beatmap->getOD(), beatmap->getHP());
	AR = (score.isLegacyScore ? legacyValues.AR : score.AR);
	CS = (score.isLegacyScore ? legacyValues.CS : score.CS);
	OD = (score.isLegacyScore ? legacyValues.OD : score.OD);
	HP = (score.isLegacyScore ? legacyValues.HP : score.HP);

	speedMultiplier = (score.isLegacyScore ? legacyValues.speedMultiplier : score.speedMultiplier);
	hidden = score.modsLegacy & OsuReplay::Mods::Hidden;
	relax = score.modsLegacy & OsuReplay::Mods::Relax;
	autopilot = score.modsLegacy & OsuReplay::Mods::Relax2;
	touchDevice = score.modsLegacy & OsuReplay::Mods::TouchDevice;
	flashlight = score.modsLegacy & OsuReplay::Mods::Flashlight;

	breakDuration = beatmap->getBreakDurationTotal();

	if (sortedHitObjects.size() > 0)
		playableLength = (unsigned long)(sortedHitObjects[sortedHitObjects.size()-1].baseEndTime - sortedHitObjects[0].baseTime);
	else
		playableLength = beatmap->getLengthMS();
}

double OsuDifficultyCalculator::calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains)
{
	std::atomic<bool> dead;
	dead = false;
	return calculateDifficultyAttributes(outAttributes, beatmapData, upToObjectIndex, outAimStrains, outSpeedStrains, dead);
}

double OsuDifficultyCalculator::calculateDifficultyAttributes(DifficultyAttributes &outAttributes, const BeatmapDiffcalcData &beatmapData, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead)
{
	outAttributes.clear();
	OsuLazer::Mods mods;
	mods.Hidden = beatmapData.hidden;
	mods.Flashlight = beatmapData.flashlight;
	mods.TouchDevice = beatmapData.touchDevice;
	mods.Relax = beatmapData.relax && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool();
	mods.Autopilot = beatmapData.autopilot && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool();

	auto objects = OsuLazer::preprocess(beatmapData, dead);
	if (dead.load()) return 0.0;
	if (upToObjectIndex >= 0 && objects.size() > (size_t)upToObjectIndex)
		objects.resize((size_t)upToObjectIndex);

	auto result = OsuLazer::Difficulty::calculate(objects, mods, dead);
	if (dead.load()) return 0.0;

	const size_t objectCount = upToObjectIndex < 0
		? beatmapData.sortedHitObjects.size()
		: std::min(beatmapData.sortedHitObjects.size(), (size_t)upToObjectIndex + 1);
	calculateScoreV1Attributes(outAttributes, beatmapData, (int)objectCount);
	result.attributes.NestedScorePerObject = outAttributes.NestedScorePerObject;
	result.attributes.LegacyScoreBaseMultiplier = outAttributes.LegacyScoreBaseMultiplier;
	result.attributes.MaximumLegacyComboScore = outAttributes.MaximumLegacyComboScore;
	result.attributes.HitCircleCount = result.attributes.SliderCount = result.attributes.SpinnerCount = result.attributes.MaxCombo = 0;
	for (size_t i = 0; i < objectCount; ++i)
	{
		const auto &h = beatmapData.sortedHitObjects[i];
		if (h.type == OsuDifficultyHitObject::TYPE::CIRCLE) result.attributes.HitCircleCount++;
		else if (h.type == OsuDifficultyHitObject::TYPE::SLIDER) result.attributes.SliderCount++;
		else if (h.type == OsuDifficultyHitObject::TYPE::SPINNER) result.attributes.SpinnerCount++;
		result.attributes.MaxCombo += 1 + (h.type == OsuDifficultyHitObject::TYPE::SLIDER ? (int)h.scoringTimes.size() : 0);
	}
	outAttributes.lazer = result.attributes;
	outAttributes.AimDifficulty = result.attributes.AimDifficulty;
	outAttributes.SpeedDifficulty = result.attributes.SpeedDifficulty;
	outAttributes.AimDifficultSliderCount = result.attributes.AimDifficultSliderCount;
	outAttributes.SpeedNoteCount = result.attributes.SpeedNoteCount;
	outAttributes.SliderFactor = result.attributes.SliderFactor;
	outAttributes.AimDifficultStrainCount = result.attributes.AimDifficultStrainCount;
	outAttributes.SpeedDifficultStrainCount = result.attributes.SpeedDifficultStrainCount;
	outAttributes.AimTopWeightedSliderFactor = result.attributes.AimTopWeightedSliderFactor;
	outAttributes.SpeedTopWeightedSliderFactor = result.attributes.SpeedTopWeightedSliderFactor;
	outAttributes.SliderCount = result.attributes.SliderCount;

	// per-object strains are not used for drawing the strain graph
	//if (outAimStrains) *outAimStrains = std::move(result.aimStrains);
	//if (outSpeedStrains) *outSpeedStrains = std::move(result.speedStrains);
	// instead, the legacy method is used
	struct LegacyFixedStrainCalc
	{
		static std::vector<double> calculate_difficulty(const std::vector<OsuDifficultyHitObject> &dobjects, const size_t objectCount, const std::vector<double> &inStrains, Skills::Skill skill)
		{
			static const double strain_step = 400.0;	// the length of each strain section
			static const double decay_base[Skills::NUM_SKILLS] = {0.3, 0.2, 0.2};

			std::vector<double> highestStrains;
			if (dobjects.size() < 2 || inStrains.size() < 1) return highestStrains;

			double interval_end = (std::ceil((double)dobjects[0].time / strain_step) * strain_step);
			double max_strain = 0.0;

			for (size_t i=1; i<objectCount; i++) // NOTE: starts at 1!
			{
				const int curIndex = (int)i;
				const int prevIndex = (int)i - 1;

				const auto &cur = dobjects[curIndex];
				const auto &prev = dobjects[prevIndex];

				// make previous peak strain decay until the current object
				while (cur.time > interval_end)
				{
					highestStrains.push_back(max_strain);

					// skip calculating strain decay for very long breaks (e.g. beatmap upload size limit hack diffs)
					// strainDecay with a base of 0.3 at 60 seconds is 4.23911583e-32, well below any meaningful difference even after being multiplied by object strain
					double strainDelta = interval_end - (double)prev.time;
					if (strainDelta > 600000.0) // !prev
						max_strain = 0.0;
					else
						max_strain = (prevIndex - 1 >= 0 ? inStrains[prevIndex - 1] : 0.0) * std::pow(decay_base[static_cast<int>(skill)], strainDelta / 1000.0);

					interval_end += strain_step;
				}

				// calculate max strain for this interval
				max_strain = std::max(max_strain, (curIndex - 1 >= 0 ? inStrains[curIndex - 1] : 0.0));
			}

		    // the peak strain will not be saved for the last section in the above loop
			highestStrains.push_back(max_strain);

			return highestStrains;
		}
	};
	if (outAimStrains != NULL)
		(*outAimStrains) = LegacyFixedStrainCalc::calculate_difficulty(beatmapData.sortedHitObjects, objectCount, result.aimStrains, Skills::Skill::AIM_SLIDERS);
	if (outSpeedStrains != NULL)
		(*outSpeedStrains) = LegacyFixedStrainCalc::calculate_difficulty(beatmapData.sortedHitObjects, objectCount, result.speedStrains, Skills::Skill::SPEED);

	return result.attributes.StarRating;
}

std::vector<OsuDifficultyCalculator::DifficultyAttributes> OsuDifficultyCalculator::calculateLazerLiveDifficultyAttributes(const BeatmapDiffcalcData &beatmapData, std::atomic<int> &progress, const std::atomic<bool> &dead)
{
	std::vector<DifficultyAttributes> result;
	if (beatmapData.sortedHitObjects.empty()) return result;

	OsuLazer::Mods mods;
	mods.Hidden = beatmapData.hidden;
	mods.Flashlight = beatmapData.flashlight;
	mods.TouchDevice = beatmapData.touchDevice;
	mods.Relax = beatmapData.relax && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool();
	mods.Autopilot = beatmapData.autopilot && !osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool();
	const auto objects = OsuLazer::preprocess(beatmapData, dead);
	if (dead.load()) return result;
	const auto live = OsuLazer::Difficulty::calculateLive(objects, mods, progress, dead);
	if (dead.load()) return result;

	result.resize(beatmapData.sortedHitObjects.size());
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (dead.load()) return {};
		DifficultyAttributes &attributes = result[i];
		attributes.clear();
		if (i > 0 && i - 1 < live.size()) attributes.lazer = live[i - 1];
		calculateScoreV1Attributes(attributes, beatmapData, (int)i + 1);
		attributes.lazer.NestedScorePerObject = attributes.NestedScorePerObject;
		attributes.lazer.LegacyScoreBaseMultiplier = attributes.LegacyScoreBaseMultiplier;
		attributes.lazer.MaximumLegacyComboScore = attributes.MaximumLegacyComboScore;
		attributes.lazer.HitCircleCount = attributes.lazer.SliderCount = attributes.lazer.SpinnerCount = attributes.lazer.MaxCombo = 0;
		for (size_t j = 0; j <= i; ++j)
		{
			const auto &h = beatmapData.sortedHitObjects[j];
			if (h.type == OsuDifficultyHitObject::TYPE::CIRCLE) attributes.lazer.HitCircleCount++;
			else if (h.type == OsuDifficultyHitObject::TYPE::SLIDER) attributes.lazer.SliderCount++;
			else if (h.type == OsuDifficultyHitObject::TYPE::SPINNER) attributes.lazer.SpinnerCount++;
			attributes.lazer.MaxCombo += 1 + (h.type == OsuDifficultyHitObject::TYPE::SLIDER ? (int)h.scoringTimes.size() : 0);
		}
		attributes.AimDifficulty = attributes.lazer.AimDifficulty;
		attributes.SpeedDifficulty = attributes.lazer.SpeedDifficulty;
		attributes.AimDifficultSliderCount = attributes.lazer.AimDifficultSliderCount;
		attributes.SpeedNoteCount = attributes.lazer.SpeedNoteCount;
		attributes.SliderFactor = attributes.lazer.SliderFactor;
		attributes.AimDifficultStrainCount = attributes.lazer.AimDifficultStrainCount;
		attributes.SpeedDifficultStrainCount = attributes.lazer.SpeedDifficultStrainCount;
		attributes.AimTopWeightedSliderFactor = attributes.lazer.AimTopWeightedSliderFactor;
		attributes.SpeedTopWeightedSliderFactor = attributes.lazer.SpeedTopWeightedSliderFactor;
		attributes.SliderCount = attributes.lazer.SliderCount;
	}
	return result;
}

void OsuDifficultyCalculator::calculateScoreV1Attributes(DifficultyAttributes &attributes, const BeatmapDiffcalcData &b, int upToObjectIndex)
{
	// Nested score per object
	const double big_tick_score = 30;
	const double small_tick_score = 10;

	double sliderScore = 0;
	double spinnerScore = 0;

	if (upToObjectIndex < 1)
		upToObjectIndex = (int)b.sortedHitObjects.size();

	for (int i=0; i<upToObjectIndex; i++)
	{
		const auto &hitObject = b.sortedHitObjects[i];

		if (hitObject.type == OsuDifficultyHitObject::TYPE::SLIDER)
		{
			// 1 for head, 1 for tail
			int amountOfBigTicks = 2;

			// Add slider repeats
			amountOfBigTicks += hitObject.repeats - 1;

			int amountOfSmallTicks = 0;

			// Slider ticks
			for (const auto &nestedHitObject : hitObject.scoringTimes)
			{
				if (nestedHitObject.type == OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::TICK)
					amountOfSmallTicks++;
			}

			sliderScore += amountOfBigTicks * big_tick_score + amountOfSmallTicks * small_tick_score;
		}
		else if (hitObject.type == OsuDifficultyHitObject::TYPE::SPINNER)
			spinnerScore += calculateScoreV1SpinnerScore(hitObject.baseEndTime - hitObject.baseTime);
	}

	attributes.NestedScorePerObject = (sliderScore + spinnerScore) / (double)std::max(upToObjectIndex, 1);

	// Legacy score base multiplier
	const unsigned long breakTimeMS = b.breakDuration;
	const unsigned long drainLength = std::max(b.playableLength - std::min(breakTimeMS, b.playableLength), (unsigned long)1000) / 1000;
	attributes.LegacyScoreBaseMultiplier = (int)std::round((b.CS + b.HP + b.OD + clamp<float>((float)b.sortedHitObjects.size() / (float)drainLength * 8.0f, 0.0f, 16.0f)) / 38.0f * 5.0f);

	// Maximum combo score
	const unsigned long score_increase = 300;
	int combo = 0;
	attributes.MaximumLegacyComboScore = 0;

	for (int i=0; i<upToObjectIndex; i++)
	{
		const auto &hitObject = b.sortedHitObjects[i];
		
		if (hitObject.type == OsuDifficultyHitObject::TYPE::SLIDER)
		{
			// Increase combo for each nested object
			combo += hitObject.scoringTimes.size();

			// For sliders we need to increase combo BEFORE giving score
			combo++;
		}

		attributes.MaximumLegacyComboScore += (int)(std::max(0, combo - 1) * (score_increase / 25 * attributes.LegacyScoreBaseMultiplier));
		
		// We have already increased combo for slider
		if (hitObject.type != OsuDifficultyHitObject::TYPE::SLIDER)
			combo++;
	}
}

double OsuDifficultyCalculator::calculateScoreV1SpinnerScore(double spinnerDuration)
{
	const int spin_score = 100;
	const int bonus_spin_score = 1000;

	// The spinner object applies a lenience because gameplay mechanics differ from osu-stable.
	// We'll redo the calculations to match osu-stable here...
	const double maximum_rotations_per_second = 477.0 / 60;

	// Normally, this value depends on the final overall difficulty. For simplicity, we'll only consider the worst case that maximises bonus score.
	// As we're primarily concerned with computing the maximum theoretical final score,
	// this will have the final effect of slightly underestimating bonus score achieved on stable when converting from score V1.
	const double minimum_rotations_per_second = 3.0;

	double secondsDuration = spinnerDuration / 1000.0;

	// The total amount of half spins possible for the entire spinner.
	int totalHalfSpinsPossible = (int)(secondsDuration * maximum_rotations_per_second * 2);
	// The amount of half spins that are required to successfully complete the spinner (i.e. get a 300).
	int halfSpinsRequiredForCompletion = (int)(secondsDuration * minimum_rotations_per_second);
	// To be able to receive bonus points, the spinner must be rotated another 1.5 times.
	int halfSpinsRequiredBeforeBonus = halfSpinsRequiredForCompletion + 3;

	long score = 0;

	int fullSpins = (totalHalfSpinsPossible / 2);

	// Normal spin score
	score += spin_score * fullSpins;

	int bonusSpins = (totalHalfSpinsPossible - halfSpinsRequiredBeforeBonus) / 2;

	// Reduce amount of bonus spins because we want to represent the more average case, rather than the best one.
	bonusSpins = std::max(0, bonusSpins - fullSpins / 2);
	score += bonus_spin_score * bonusSpins;

	return score;
}

double OsuDifficultyCalculator::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, unsigned long legacyTotalScore)
{
	// NOTE: depends on active mods + OD + AR

	// get runtime mods
	int modsLegacy = osu->getScore()->getModsLegacy();

	return calculatePPv2(false, OsuScore::VERSION, modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(false), beatmap->getOD(false), attributes, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo, combo, misses, c300, c100, c50, legacyTotalScore);
}

double OsuDifficultyCalculator::calculatePPv2(bool isImportedLegacyScore, int version, int modsLegacy, double timescale, double ar, double od, DifficultyAttributes attributes, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50, unsigned long legacyTotalScore)
{
	OsuLazer::Attributes a = attributes.lazer;
	a.HitCircleCount = numCircles; a.SliderCount = numSliders; a.SpinnerCount = numSpinners; a.MaxCombo = maxPossibleCombo;
	OsuLazer::Score s;
	s.Mods = modsLegacy;
	if (osu_stars_and_pp_lazer_relax_autopilot_nerf_disabled.getBool())
		s.Mods &= ~(OsuReplay::Mods::Relax | OsuReplay::Mods::Relax2);
	s.ClockRate = timescale; s.AR = ar; s.OD = od;
	s.Great = c300 < 0 ? std::max(0, numHitObjects-c100-c50-misses) : c300;
	s.Ok = c100; s.Meh = c50; s.Miss = misses;
	s.MaxCombo = combo < 0 ? maxPossibleCombo : combo;
	s.ScoreV2 = (modsLegacy & OsuReplay::Mods::ScoreV2) != 0;
	s.LegacyTotalScore = legacyTotalScore;
	return OsuLazer::performance(a, s);
}
