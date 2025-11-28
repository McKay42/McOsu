//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		score handler (calculation, storage)
//
// $NoKeywords: $osu
//===============================================================================//

#include "OsuScore.h"

#include "Engine.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuMultiplayer.h"
#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuDifficultyCalculator.h"
#include "OsuHUD.h"
#include "OsuGameRules.h"
#include "OsuReplay.h"
#include "OsuHitObject.h"

ConVar osu_hiterrorbar_misses("osu_hiterrorbar_misses", true, FCVAR_NONE);
ConVar osu_debug_pp("osu_debug_pp", false, FCVAR_NONE);

ConVar osu_hud_statistics_hitdelta_chunksize("osu_hud_statistics_hitdelta_chunksize", 30, FCVAR_NONE, "how many recent hit deltas to average (-1 = all)");

ConVar osu_drain_vr_multiplier("osu_drain_vr_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_drain_vr_300("osu_drain_vr_300", 0.035f, FCVAR_NONE);
ConVar osu_drain_vr_100("osu_drain_vr_100", -0.10f, FCVAR_NONE);
ConVar osu_drain_vr_50("osu_drain_vr_50", -0.125f, FCVAR_NONE);
ConVar osu_drain_vr_miss("osu_drain_vr_miss", -0.15f, FCVAR_NONE);
ConVar osu_drain_vr_sliderbreak("osu_drain_vr_sliderbreak", -0.10f, FCVAR_NONE);

ConVar osu_drain_stable_hpbar_maximum("osu_drain_stable_hpbar_maximum", 200.0f, FCVAR_NONE);

ConVar osu_drain_lazer_multiplier("osu_drain_lazer_multiplier", 0.05f, FCVAR_NONE, "DEFAULT_MAX_HEALTH_INCREASE, expressed as a percentage of full health");
ConVar osu_drain_lazer_300("osu_drain_lazer_300", 1.0f, FCVAR_NONE);
ConVar osu_drain_lazer_100("osu_drain_lazer_100", 0.5f, FCVAR_NONE);
ConVar osu_drain_lazer_50("osu_drain_lazer_50", -0.05f, FCVAR_NONE);
ConVar osu_drain_lazer_miss("osu_drain_lazer_miss", -1.0f, FCVAR_NONE);

ConVar osu_drain_lazer_2018_multiplier("osu_drain_lazer_2018_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_drain_lazer_2018_300("osu_drain_lazer_2018_300", 0.01f, FCVAR_NONE);
ConVar osu_drain_lazer_2018_100("osu_drain_lazer_2018_100", 0.01f, FCVAR_NONE);
ConVar osu_drain_lazer_2018_50("osu_drain_lazer_2018_50", 0.01f, FCVAR_NONE);
ConVar osu_drain_lazer_2018_miss("osu_drain_lazer_2018_miss", -0.02f, FCVAR_NONE);

ConVar *OsuScore::m_osu_draw_statistics_pp_ref = NULL;
ConVar *OsuScore::m_osu_drain_type_ref = NULL;

OsuScore::OsuScore(Osu *osu)
{
	m_osu = osu;
	reset();

	if (m_osu_draw_statistics_pp_ref == NULL)
		m_osu_draw_statistics_pp_ref = convar->getConVarByName("osu_draw_statistics_pp");
	if (m_osu_drain_type_ref == NULL)
		m_osu_drain_type_ref = convar->getConVarByName("osu_drain_type");
}

void OsuScore::reset()
{
	m_hitresults = std::vector<HIT>();
	m_hitdeltas = std::vector<int>();

	m_grade = OsuScore::GRADE::GRADE_N;

	m_fStarsTomTotal = 0.0f;
	m_fStarsTomAim = 0.0f;
	m_fStarsTomSpeed = 0.0f;
	m_fPPv2 = 0.0f;
	m_iIndex = -1;

	m_iScoreV1 = 0;
	m_iScoreV2 = 0;
	m_iScoreV2ComboPortion = 0;
	m_iBonusPoints = 0;
	m_iCombo = 0;
	m_iComboMax = 0;
	m_iComboFull = 0;
	m_iComboEndBitmask = 0;
	m_fAccuracy = 1.0f;
	m_fHitErrorAvgMin = 0.0f;
	m_fHitErrorAvgMax = 0.0f;
	m_fHitErrorAvgCustomMin = 0.0f;
	m_fHitErrorAvgCustomMax = 0.0f;
	m_fUnstableRate = 0.0f;

	m_iNumMisses = 0;
	m_iNumSliderBreaks = 0;
	m_iNum50s = 0;
	m_iNum100s = 0;
	m_iNum100ks = 0;
	m_iNum300s = 0;
	m_iNum300gs = 0;

	m_bDead = false;
	m_bDied = false;

	m_iNumK1 = 0;
	m_iNumK2 = 0;
	m_iNumM1 = 0;
	m_iNumM2 = 0;

	m_iNumEZRetries = 2;
	m_bIsUnranked = false;

	onScoreChange();
}

void OsuScore::addHitResult(OsuBeatmap *beatmap, OsuHitObject *hitObject, HIT hit, long delta, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore)
{
	const int scoreComboMultiplier = std::max(m_iCombo - 1, 0); // current combo, excluding the current hitobject which caused the addHitResult() call

	// handle hits (and misses)
	if (hit != OsuScore::HIT::HIT_MISS)
	{
		if (!ignoreOnHitErrorBar)
		{
			m_hitdeltas.push_back((int)delta);
			m_osu->getHUD()->addHitError(delta);
		}

		if (!ignoreCombo)
		{
			m_iCombo++;
			m_osu->getHUD()->animateCombo();
		}
	}
	else // misses
	{
		if (osu_hiterrorbar_misses.getBool() && !ignoreOnHitErrorBar && delta <= (long)OsuGameRules::getHitWindow50(beatmap))
			m_osu->getHUD()->addHitError(delta, true);

		m_iCombo = 0;
	}

	// keep track of the maximum possible combo at this time, for live pp calculation
	if (!ignoreCombo)
		m_iComboFull++;

	// store the result, get hit value
	unsigned long long hitValue = 0;
	if (!hitErrorBarOnly)
	{
		m_hitresults.push_back(hit);

		switch (hit)
		{
		case OsuScore::HIT::HIT_MISS:
			m_iNumMisses++;
			m_iComboEndBitmask |= 2;
			break;

		case OsuScore::HIT::HIT_50:
			m_iNum50s++;
			hitValue = 50;
			m_iComboEndBitmask |= 2;
			break;

		case OsuScore::HIT::HIT_100:
			m_iNum100s++;
			hitValue = 100;
			m_iComboEndBitmask |= 1;
			break;

		case OsuScore::HIT::HIT_300:
			m_iNum300s++;
			hitValue = 300;
			break;
		}
	}

	// add hitValue to score, recalculate score v1
	const unsigned long breakTimeMS = beatmap->getBreakDurationTotal();
	const unsigned long drainLength = std::max(beatmap->getLengthPlayable() - std::min(breakTimeMS, beatmap->getLengthPlayable()), (unsigned long)1000) / 1000;
	const int difficultyMultiplier = (int)std::round((beatmap->getSelectedDifficulty2()->getCS() + beatmap->getSelectedDifficulty2()->getHP() + beatmap->getSelectedDifficulty2()->getOD() + clamp<float>((float)beatmap->getSelectedDifficulty2()->getNumObjects() / (float)drainLength * 8.0f, 0.0f, 16.0f)) / 38.0f * 5.0f);
	if (!ignoreScore)
		m_iScoreV1 += hitValue + ((hitValue * (unsigned long long)((double)scoreComboMultiplier * (double)difficultyMultiplier * (double)m_osu->getScoreMultiplier())) / (unsigned long long)25);

	const float totalHitPoints = m_iNum50s*(1.0f/6.0f)+ m_iNum100s*(2.0f/6.0f) + m_iNum300s;
	const float totalNumHits = m_iNumMisses + m_iNum50s + m_iNum100s + m_iNum300s;

	const float percent300s = m_iNum300s / totalNumHits;
	const float percent50s = m_iNum50s / totalNumHits;

	// recalculate accuracy
	if (((totalHitPoints == 0.0f || totalNumHits == 0.0f) && m_hitresults.size() < 1) || totalNumHits <= 0.0f)
		m_fAccuracy = 1.0f;
	else
		m_fAccuracy = totalHitPoints / totalNumHits;

	// recalculate score v2
	m_iScoreV2ComboPortion += (unsigned long long)((double)hitValue * (1.0 + (double)scoreComboMultiplier / 10.0));
	if (m_osu->getModScorev2())
	{
		const int numHitObjects = beatmap->getSelectedDifficulty2()->getNumObjects();
		const double maximumAccurateHits = numHitObjects;

		if (totalNumHits > 0)
			m_iScoreV2 = (unsigned long long)(((double)m_iScoreV2ComboPortion / (double)beatmap->getScoreV2ComboPortionMaximum() * 700000.0 + std::pow((double)m_fAccuracy, 10.0) * ((double)totalNumHits / maximumAccurateHits) * 300000.0 + (double)m_iBonusPoints) * (double)m_osu->getScoreMultiplier());

		///debugLog("%i / %i, combo = %ix\n", (int)m_iScoreV2ComboPortion, (int)beatmap->getSelectedDifficulty()->getScoreV2ComboPortionMaximum(), m_iCombo);
	}

	// recalculate grade
	m_grade = OsuScore::GRADE::GRADE_D;
	if (percent300s > 0.6f)
		m_grade = OsuScore::GRADE::GRADE_C;
	if ((percent300s > 0.7f && m_iNumMisses == 0) || (percent300s > 0.8f))
		m_grade = OsuScore::GRADE::GRADE_B;
	if ((percent300s > 0.8f && m_iNumMisses == 0) || (percent300s > 0.9f))
		m_grade = OsuScore::GRADE::GRADE_A;
	if (percent300s > 0.9f && percent50s <= 0.01f && m_iNumMisses == 0)
		m_grade = m_osu->getModHD() /* || m_osu->getModFlashlight() */ ? OsuScore::GRADE::GRADE_SH : OsuScore::GRADE::GRADE_S;
	if (m_iNumMisses == 0 && m_iNum50s == 0 && m_iNum100s == 0)
		m_grade = m_osu->getModHD() /* || m_osu->getModFlashlight() */ ? OsuScore::GRADE::GRADE_XH : OsuScore::GRADE::GRADE_X;

	// recalculate unstable rate
	float averageDelta = 0.0f;
	m_fUnstableRate = 0.0f;
	m_fHitErrorAvgMin = 0.0f;
	m_fHitErrorAvgMax = 0.0f;
	m_fHitErrorAvgCustomMin = 0.0f;
	m_fHitErrorAvgCustomMax = 0.0f;
	if (m_hitdeltas.size() > 0)
	{
		int numPositives = 0;
		int numNegatives = 0;
		int numCustomPositives = 0;
		int numCustomNegatives = 0;

		const int customStartIndex = (osu_hud_statistics_hitdelta_chunksize.getInt() < 0 ? 0 : std::max(0, (int)m_hitdeltas.size() - osu_hud_statistics_hitdelta_chunksize.getInt())) - 1;

		for (int i=0; i<m_hitdeltas.size(); i++)
		{
			averageDelta += (float)m_hitdeltas[i];

			if (m_hitdeltas[i] > 0)
			{
				// positive
				m_fHitErrorAvgMax += (float)m_hitdeltas[i];
				numPositives++;

				if (i > customStartIndex)
				{
					m_fHitErrorAvgCustomMax += (float)m_hitdeltas[i];
					numCustomPositives++;
				}
			}
			else if (m_hitdeltas[i] < 0)
			{
				// negative
				m_fHitErrorAvgMin += (float)m_hitdeltas[i];
				numNegatives++;

				if (i > customStartIndex)
				{
					m_fHitErrorAvgCustomMin += (float)m_hitdeltas[i];
					numCustomNegatives++;
				}
			}
			else
			{
				// perfect
				numPositives++;
				numNegatives++;

				if (i > customStartIndex)
				{
					numCustomPositives++;
					numCustomNegatives++;
				}
			}
		}
		averageDelta /= (float)m_hitdeltas.size();
		m_fHitErrorAvgMin = (numNegatives > 0 ? m_fHitErrorAvgMin / (float)numNegatives : 0.0f);
		m_fHitErrorAvgMax = (numPositives > 0 ? m_fHitErrorAvgMax / (float)numPositives : 0.0f);
		m_fHitErrorAvgCustomMin = (numCustomNegatives > 0 ? m_fHitErrorAvgCustomMin / (float)numCustomNegatives : 0.0f);
		m_fHitErrorAvgCustomMax = (numCustomPositives > 0 ? m_fHitErrorAvgCustomMax / (float)numCustomPositives : 0.0f);

		for (int i=0; i<m_hitdeltas.size(); i++)
		{
			m_fUnstableRate += ((float)m_hitdeltas[i] - averageDelta)*((float)m_hitdeltas[i] - averageDelta);
		}
		m_fUnstableRate /= (float)m_hitdeltas.size();
		m_fUnstableRate = std::sqrt(m_fUnstableRate)*10;

		// compensate for speed
		m_fUnstableRate /= beatmap->getSpeedMultiplier();
	}

	// recalculate max combo
	if (m_iCombo > m_iComboMax)
		m_iComboMax = m_iCombo;

	// recalculate pp
	if (m_osu_draw_statistics_pp_ref->getBool()) // sanity + performance
	{
		OsuBeatmapStandard *standardPointer = dynamic_cast<OsuBeatmapStandard*>(beatmap);
		if (standardPointer != NULL && beatmap->getSelectedDifficulty2() != NULL)
		{
			OsuDifficultyCalculator::DifficultyAttributes attributes = standardPointer->getDifficultyAttributes();

			//int numHitObjects = standardPointer->getNumHitObjects();
			int maxPossibleCombo = beatmap->getMaxPossibleCombo();
			int numCircles = beatmap->getSelectedDifficulty2()->getNumCircles();
			int numSliders = beatmap->getSelectedDifficulty2()->getNumSliders();
			int numSpinners = beatmap->getSelectedDifficulty2()->getNumSpinners();

			// real (simulate beatmap being cut off after current hit, thus changing aimStars and speedStars on every hit)
			{
				const int curHitobjectIndex = beatmap->getHitObjectIndexForCurrentTime(); // current index of last hitobject to just finish at this time (e.g. if the first OsuCircle just finished and called addHitResult(), this would be 0)
				maxPossibleCombo = m_iComboFull; // current maximum possible combo at this time

				// NOTE: all pre-lazer-20220902 star/pp algorithm versions only ever considered everything a circle (except for spinners ofc), which is why simply "cheating" and adding a hardcoded +1 to the beatmap->getNumCirclesForCurrentTime() worked for live pp
				// NOTE: the reason for the +1 is that in the mcosu update loop, when we get called where we are inside addHitResult(), the beatmap hitobject counters have not yet been updated.
				// NOTE: we can not early update the counters since we don't yet know whether the hitobject will actually be "finished" in that part of the update loop.
				// NOTE: now, since the algorithms do require each individual hitobject type counts (and not JUST the "circle" count), this workaround no longer works.
				// NOTE: therefore, we simply query the hitobject type right here and add the +1 where relevant (because we know that we would only be at this point if the hitobject is actually "finished" now, as before when only circles existed)
				// NOTE: of course, objects with "length", like sliders, call addHitResult() multiple times while they are being "finished". star and pp algorithms ONLY want FULLY FINISHED hitobjects in these counters, so we have to additionally check that.
				// WARNING: hitObject can be NULL in some very rare cases (like the nightmare mod has some addHitResult(NULL, ...) calls just to drain more hp in some places)
				numCircles = clamp<int>(beatmap->getNumCirclesForCurrentTime() + (hitObject != NULL && hitObject->isCircle() ? 1 : 0), 0, beatmap->getSelectedDifficulty2()->getNumCircles()); // current maximum number of fully finished (!) circles at this time
				numSliders = clamp<int>(beatmap->getNumSlidersForCurrentTime() + (hitObject != NULL && hitObject->isSlider() && hitObject->isFinished() ? 1 : 0), 0, beatmap->getSelectedDifficulty2()->getNumSliders()); // current maximum number of fully finished (!) sliders at this time
				numSpinners = clamp<int>(beatmap->getNumSpinnersForCurrentTime() + (hitObject != NULL && hitObject->isSpinner() && hitObject->isFinished() ? 1 : 0), 0, beatmap->getSelectedDifficulty2()->getNumSpinners()); // current maximum number of fully finished (!) spinners at this time

				//beatmap->getSelectedDifficulty()->calculateStarDiff(beatmap, &aimStars, &speedStars, curHitobjectIndex); // recalculating this live costs too much time
				attributes = beatmap->getDifficultyAttributesForUpToHitObjectIndex(curHitobjectIndex);

				m_fPPv2 = OsuDifficultyCalculator::calculatePPv2(m_osu, beatmap, attributes, -1, numCircles, numSliders, numSpinners, maxPossibleCombo, m_iComboMax, m_iNumMisses, m_iNum300s, m_iNum100s, m_iNum50s);

				if (osu_debug_pp.getBool())
					debugLog("pp = %f, aimstars = %f, aimsliderfactor = %f, speedstars = %f, speednotes = %f, curindex = %i, maxPossibleCombo = %i, numCircles = %i, numSliders = %i, numSpinners = %i\n", m_fPPv2, attributes.AimDifficulty, attributes.SliderFactor, attributes.SpeedDifficulty, attributes.SpeedNoteCount, curHitobjectIndex, maxPossibleCombo, numCircles, numSliders, numSpinners);
			}
		}
		else
			m_fPPv2 = 0.0f;
	}

	onScoreChange();
}

void OsuScore::addHitResultComboEnd(OsuScore::HIT hit)
{
	switch (hit)
	{
	case OsuScore::HIT::HIT_100K:
	case OsuScore::HIT::HIT_300K:
		m_iNum100ks++;
		break;

	case OsuScore::HIT::HIT_300G:
		m_iNum300gs++;
		break;
	}
}

void OsuScore::addSliderBreak()
{
	m_iCombo = 0;
	m_iNumSliderBreaks++;

	onScoreChange();
}

void OsuScore::addPoints(int points, bool isSpinner)
{
	m_iScoreV1 += (unsigned long long)points;

	if (isSpinner)
		m_iBonusPoints += points; // only used for scorev2 calculation currently

	onScoreChange();
}

void OsuScore::setDead(bool dead)
{
	if (m_bDead == dead) return;

	m_bDead = dead;

	if (m_bDead)
		m_bDied = true;

	onScoreChange();
}

void OsuScore::addKeyCount(int key)
{
	switch (key)
	{
	case 1:
		m_iNumK1++;
		break;
	case 2:
		m_iNumK2++;
		break;
	case 3:
		m_iNumM1++;
		break;
	case 4:
		m_iNumM2++;
		break;
	}
}

double OsuScore::getHealthIncrease(OsuBeatmap *beatmap, HIT hit)
{
	return getHealthIncrease(hit, beatmap->getHP(), beatmap->getHPMultiplierNormal(), beatmap->getHPMultiplierComboEnd(), (double)osu_drain_stable_hpbar_maximum.getFloat());
}

double OsuScore::getHealthIncrease(OsuScore::HIT hit, double HP, double hpMultiplierNormal, double hpMultiplierComboEnd, double hpBarMaximumForNormalization)
{
	const int drainType = m_osu_drain_type_ref->getInt();

	if (drainType == 1) // VR
	{
		switch (hit)
		{
		case OsuScore::HIT::HIT_MISS:
			return osu_drain_vr_miss.getFloat() * osu_drain_vr_multiplier.getFloat();

		case OsuScore::HIT::HIT_50:
			return osu_drain_vr_50.getFloat() * osu_drain_vr_multiplier.getFloat();

		case OsuScore::HIT::HIT_100:
			return osu_drain_vr_100.getFloat() * osu_drain_vr_multiplier.getFloat();

		case OsuScore::HIT::HIT_300:
		case OsuScore::HIT::HIT_SLIDER30:
			return osu_drain_vr_300.getFloat() * osu_drain_vr_multiplier.getFloat();



		case OsuScore::HIT::HIT_MISS_SLIDERBREAK:
			return osu_drain_vr_sliderbreak.getFloat() * osu_drain_vr_multiplier.getFloat();
		}
	}
	else if (drainType == 2) // osu!stable
	{
		switch (hit)
		{
		case OsuScore::HIT::HIT_MISS:
			return (OsuGameRules::mapDifficultyRangeDouble(HP, -6.0, -25.0, -40.0) / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_50:
			return (hpMultiplierNormal * OsuGameRules::mapDifficultyRangeDouble(HP, 0.4 * 8.0, 0.4, 0.4) / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_100:
			return (hpMultiplierNormal * OsuGameRules::mapDifficultyRangeDouble(HP, 2.2 * 8.0, 2.2, 2.2) / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_300:
			return (hpMultiplierNormal * 6.0 / hpBarMaximumForNormalization);



		case OsuScore::HIT::HIT_MISS_SLIDERBREAK:
			return (OsuGameRules::mapDifficultyRangeDouble(HP, -4.0, -15.0, -28.0) / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_MU:
			return (hpMultiplierComboEnd * 6.0 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_100K:
			return (hpMultiplierComboEnd * 10.0 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_300K:
			return (hpMultiplierComboEnd * 10.0 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_300G:
			return (hpMultiplierComboEnd * 14.0 / hpBarMaximumForNormalization);



		case OsuScore::HIT::HIT_SLIDER10:
			return (hpMultiplierNormal * 3.0 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_SLIDER30:
			return (hpMultiplierNormal * 4.0 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_SPINNERSPIN:
			return (hpMultiplierNormal * 1.7 / hpBarMaximumForNormalization);

		case OsuScore::HIT::HIT_SPINNERBONUS:
			return (hpMultiplierNormal * 2.0 / hpBarMaximumForNormalization);
		}
	}
	else if (drainType == 3) // osu!lazer 2020
	{
		switch (hit)
		{
		case HIT::HIT_MISS:
		case HIT::HIT_MISS_SLIDERBREAK:
			return osu_drain_lazer_miss.getFloat() * osu_drain_lazer_multiplier.getFloat();

		case HIT::HIT_50:
			return osu_drain_lazer_50.getFloat() * osu_drain_lazer_multiplier.getFloat();

		case HIT::HIT_100:
			return osu_drain_lazer_100.getFloat() * osu_drain_lazer_multiplier.getFloat();

		case HIT::HIT_300:
		case HIT::HIT_SLIDER10:
		case HIT::HIT_SLIDER30:
			return osu_drain_lazer_300.getFloat() * osu_drain_lazer_multiplier.getFloat();
		}
	}
	else if (drainType == 4) // osu!lazer 2018
	{
		switch (hit)
		{
		case HIT::HIT_MISS:
		case HIT::HIT_MISS_SLIDERBREAK:
			return (HP) * osu_drain_lazer_2018_miss.getFloat() * osu_drain_lazer_2018_multiplier.getFloat();

		case HIT::HIT_50:
			return (4.0 - HP) * osu_drain_lazer_2018_50.getFloat() * osu_drain_lazer_2018_multiplier.getFloat();

		case HIT::HIT_100:
			return (8.0 - HP) * osu_drain_lazer_2018_100.getFloat() * osu_drain_lazer_2018_multiplier.getFloat();

		case HIT::HIT_300:
		case HIT::HIT_SLIDER10:
		case HIT::HIT_SLIDER30:
			return (10.2 - std::min(HP, 10.0)) * osu_drain_lazer_2018_300.getFloat() * osu_drain_lazer_2018_multiplier.getFloat();
		}
	}

	return 0.0f;
}

int OsuScore::getKeyCount(int key)
{
	switch (key)
	{
	case 1:
		return m_iNumK1;
	case 2:
		return m_iNumK2;
	case 3:
		return m_iNumM1;
	case 4:
		return m_iNumM2;
	}

	return 0;
}

int OsuScore::getModsLegacy()
{
	int modsLegacy = 0;

	modsLegacy |= (m_osu->getModAuto() ? OsuReplay::Mods::Autoplay : 0);
	modsLegacy |= (m_osu->getModAutopilot() ? OsuReplay::Mods::Relax2 : 0);
	modsLegacy |= (m_osu->getModRelax() ? OsuReplay::Mods::Relax : 0);
	modsLegacy |= (m_osu->getModSpunout() ? OsuReplay::Mods::SpunOut : 0);
	modsLegacy |= (m_osu->getModTarget() ? OsuReplay::Mods::Target : 0);
	modsLegacy |= (m_osu->getModScorev2() ? OsuReplay::Mods::ScoreV2 : 0);
	modsLegacy |= (m_osu->getModDT() ? OsuReplay::Mods::DoubleTime : 0);
	modsLegacy |= (m_osu->getModNC() ? OsuReplay::Mods::Nightcore : 0);
	modsLegacy |= (m_osu->getModNF() ? OsuReplay::Mods::NoFail : 0);
	modsLegacy |= (m_osu->getModHT() ? OsuReplay::Mods::HalfTime : 0);
	modsLegacy |= (m_osu->getModDC() ? OsuReplay::Mods::HalfTime : 0);
	modsLegacy |= (m_osu->getModHD() ? OsuReplay::Mods::Hidden : 0);
	modsLegacy |= (m_osu->getModHR() ? OsuReplay::Mods::HardRock : 0);
	modsLegacy |= (m_osu->getModEZ() ? OsuReplay::Mods::Easy : 0);
	modsLegacy |= (m_osu->getModSD() ? OsuReplay::Mods::SuddenDeath : 0);
	modsLegacy |= (m_osu->getModSS() ? OsuReplay::Mods::Perfect : 0);
	modsLegacy |= (m_osu->getModNM() ? OsuReplay::Mods::Nightmare : 0);
	modsLegacy |= (m_osu->getModTD() ? OsuReplay::Mods::TouchDevice : 0);

	return modsLegacy;
}

UString OsuScore::getModsStringForRichPresence()
{
	UString modsString;

	if (m_osu->getModNF())
		modsString.append("NF");
	if (m_osu->getModEZ())
		modsString.append("EZ");
	if (m_osu->getModHD())
		modsString.append("HD");
	if (m_osu->getModHR())
		modsString.append("HR");
	if (m_osu->getModSD())
		modsString.append("SD");
	if (m_osu->getModDT())
		modsString.append("DT");
	if (m_osu->getModRelax())
		modsString.append("RX");
	if (m_osu->getModHT())
		modsString.append("HT");
	if (m_osu->getModNC())
		modsString.append("NC");
	if (m_osu->getModAuto())
		modsString.append("AT");
	if (m_osu->getModSpunout())
		modsString.append("SO");
	if (m_osu->getModAutopilot())
		modsString.append("AP");
	if (m_osu->getModSS())
		modsString.append("PF");
	if (m_osu->getModScorev2())
		modsString.append("v2");
	if (m_osu->getModTarget())
		modsString.append("TP");
	if (m_osu->getModNM())
		modsString.append("NM");
	if (m_osu->getModTD())
		modsString.append("TD");

	return modsString;
}

unsigned long long OsuScore::getScore()
{
	return m_osu->getModScorev2() ? m_iScoreV2 : m_iScoreV1;
}

void OsuScore::onScoreChange()
{
	if (m_osu->getMultiplayer() != NULL)
		m_osu->getMultiplayer()->onClientScoreChange(getCombo(), getAccuracy(), getScore(), isDead());

	// only used to block local scores for people who think they are very clever by quickly disabling auto just before the end of a beatmap
	m_bIsUnranked |= (m_osu->getModAuto() || (m_osu->getModAutopilot() && m_osu->getModRelax()));
}

float OsuScore::calculateAccuracy(int num300s, int num100s, int num50s, int numMisses)
{
	const float totalHitPoints = num50s*(1.0f/6.0f)+ num100s*(2.0f/6.0f) + num300s;
	const float totalNumHits = numMisses + num50s + num100s + num300s;

	if (totalNumHits > 0.0f)
		return (totalHitPoints / totalNumHits);

	return 0.0f;
}

OsuScore::GRADE OsuScore::calculateGrade(int num300s, int num100s, int num50s, int numMisses, bool modHidden, bool modFlashlight)
{
	const float totalNumHits = numMisses + num50s + num100s + num300s;

	float percent300s = 0.0f;
	float percent50s = 0.0f;
	if (totalNumHits > 0.0f)
	{
		percent300s = num300s / totalNumHits;
		percent50s = num50s / totalNumHits;
	}

	GRADE grade = OsuScore::GRADE::GRADE_D;
	if (percent300s > 0.6f)
		grade = OsuScore::GRADE::GRADE_C;
	if ((percent300s > 0.7f && numMisses == 0) || (percent300s > 0.8f))
		grade = OsuScore::GRADE::GRADE_B;
	if ((percent300s > 0.8f && numMisses == 0) || (percent300s > 0.9f))
		grade = OsuScore::GRADE::GRADE_A;
	if (percent300s > 0.9f && percent50s <= 0.01f && numMisses == 0)
		grade = ((modHidden || modFlashlight) ? OsuScore::GRADE::GRADE_SH : OsuScore::GRADE::GRADE_S);
	if (numMisses == 0 && num50s == 0 && num100s == 0)
		grade = ((modHidden || modFlashlight) ? OsuScore::GRADE::GRADE_XH : OsuScore::GRADE::GRADE_X);

	return grade;
}
