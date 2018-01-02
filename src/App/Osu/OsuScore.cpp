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
#include "OsuBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuHUD.h"
#include "OsuGameRules.h"

ConVar osu_hiterrorbar_misses("osu_hiterrorbar_misses", true);
ConVar osu_pp_live_type("osu_pp_live_type", 2, "type of algorithm to use for live unfinished beatmap pp calculation: 0 = 'vanilla', 1 = 'fake' total acc interp, 2 = 'real' cut off beatmap at current point (default)");
ConVar osu_debug_pp("osu_debug_pp", false);

ConVar *OsuScore::m_osu_draw_statistics_pp = NULL;

OsuScore::OsuScore(Osu *osu)
{
	m_osu = osu;
	reset();

	if (m_osu_draw_statistics_pp == NULL)
		m_osu_draw_statistics_pp = convar->getConVarByName("osu_draw_statistics_pp");
}

void OsuScore::reset()
{
	m_fStarsTomTotal = 0.0f;
	m_fStarsTomAim = 0.0f;
	m_fStarsTomSpeed = 0.0f;
	m_fPPv2 = 0.0f;
	m_grade = OsuScore::GRADE::GRADE_N;
	m_iScoreV1 = 0;
	m_iScoreV2 = 0;
	m_iScoreV2ComboPortion = 0;
	m_iBonusPoints = 0;
	m_iCombo = 0;
	m_iComboMax = 0;
	m_iComboFull = 0;
	m_fAccuracy = 1.0f;
	m_fHitErrorAvgMin = 0.0f;
	m_fHitErrorAvgMax = 0.0f;
	m_fUnstableRate = 0.0f;
	m_iNumMisses = 0;
	m_iNumSliderBreaks = 0;
	m_iNum50s = 0;
	m_iNum100s = 0;
	m_iNum100ks = 0;
	m_iNum300s = 0;
	m_iNum300gs = 0;
	m_hitresults = std::vector<HIT>();
	m_hitdeltas = std::vector<int>();
}

void OsuScore::addHitResult(OsuBeatmap *beatmap, HIT hit, long delta, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore)
{
	const int scoreComboMultiplier = std::max(m_iCombo-1, 0); // current combo, excluding the current hitobject which caused the addHitResult() call

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
			break;
		case OsuScore::HIT::HIT_50:
			m_iNum50s++;
			hitValue = 50;
			break;
		case OsuScore::HIT::HIT_100:
			m_iNum100s++;
			hitValue = 100;
			break;
		case OsuScore::HIT::HIT_300:
			m_iNum300s++;
			hitValue = 300;
			break;
		}
	}

	// add hitValue to score, recalculate scoreV1
	const float sumDifficultyPoints = beatmap->getSelectedDifficulty()->CS + beatmap->getSelectedDifficulty()->HP + beatmap->getSelectedDifficulty()->OD;
	int difficultyMultiplier = 2;
	if (sumDifficultyPoints > 5.0f)
		difficultyMultiplier = 3;
	if (sumDifficultyPoints > 12.0f)
		difficultyMultiplier = 4;
	if (sumDifficultyPoints > 17.0f)
		difficultyMultiplier = 5;
	if (sumDifficultyPoints > 24.0f)
		difficultyMultiplier = 6;
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

	// recalculate scoreV2
	m_iScoreV2ComboPortion += (unsigned long long)((double)hitValue * (1.0 + (double)scoreComboMultiplier / 10.0));
	if (m_osu->getModScorev2())
	{
		const int numHitObjects = beatmap->getSelectedDifficulty()->hitcircles.size() + beatmap->getSelectedDifficulty()->sliders.size() + beatmap->getSelectedDifficulty()->spinners.size();
		const double maximumAccurateHits = numHitObjects;

		// TODO: this should also scale and respect all of these with combo: sliderticks 10, sliderend 30, sliderrepeats 30
		// currently they are completely ignored (don't count towards score v2 score at all)

		if (totalNumHits > 0)
			m_iScoreV2 = (unsigned long long)(((double)m_iScoreV2ComboPortion / (double)beatmap->getSelectedDifficulty()->getScoreV2ComboPortionMaximum() * 700000.0 + std::pow((double)m_fAccuracy, 10.0) * ((double)totalNumHits / maximumAccurateHits) * 300000.0 + (double)m_iBonusPoints) * (double)m_osu->getScoreMultiplier());

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
	if (m_hitdeltas.size() > 0)
	{
		int numPositives = 0;
		int numNegatives = 0;
		for (int i=0; i<m_hitdeltas.size(); i++)
		{
			averageDelta += (float)m_hitdeltas[i];

			if (m_hitdeltas[i] > 0)
			{
				// positive
				m_fHitErrorAvgMax += (float)m_hitdeltas[i];
				numPositives++;
			}
			else
			{
				// negative
				m_fHitErrorAvgMin += (float)m_hitdeltas[i];
				numNegatives++;
			}
		}
		averageDelta /= (float)m_hitdeltas.size();
		m_fHitErrorAvgMin = (numNegatives > 0 ? m_fHitErrorAvgMin / (float)numNegatives : 0.0f);
		m_fHitErrorAvgMax = (numPositives > 0 ? m_fHitErrorAvgMax / (float)numPositives : 0.0f);

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
	if (m_osu_draw_statistics_pp->getBool()) // sanity + performance
	{
		OsuBeatmapStandard *standardPointer = dynamic_cast<OsuBeatmapStandard*>(beatmap);
		if (standardPointer != NULL && beatmap->getSelectedDifficulty() != NULL)
		{
			double aimStars = standardPointer->getAimStars();
			double speedStars = standardPointer->getSpeedStars();

			int numHitObjects = standardPointer->getNumHitObjects();
			int maxPossibleCombo = beatmap->getSelectedDifficulty()->getMaxCombo();
			int numCircles = beatmap->getSelectedDifficulty()->hitcircles.size();

			// three methods for calculating unfinished pp:

			if (osu_pp_live_type.getInt() == 0)
			{
				// vanilla
				m_fPPv2 = beatmap->getSelectedDifficulty()->calculatePPv2(m_osu, beatmap, aimStars, speedStars, numHitObjects, numCircles, maxPossibleCombo, m_iComboMax, m_iNumMisses, m_iNum300s, m_iNum100s, m_iNum50s);
			}
			else if (osu_pp_live_type.getInt() == 1)
			{
				// fake acc (acc grows with hits from 0 to max 1, similar to scorev2 score)
				const double fakeTotalAccuracy = (double)totalHitPoints / (double)numHitObjects;
				m_fPPv2 = beatmap->getSelectedDifficulty()->calculatePPv2Acc(m_osu, beatmap, aimStars, speedStars, fakeTotalAccuracy, numHitObjects, numCircles, maxPossibleCombo, m_iComboMax, m_iNumMisses);
			}
			else if (osu_pp_live_type.getInt() == 2)
			{
				// real (simulate beatmap being cut off after current hit, thus changing aimStars and speedStars on every hit)
				int curHitobjectIndex = beatmap->getHitObjectIndexForCurrentTime(); // current index of last hitobject to just finish at this time (e.g. if the first OsuCircle just finished and called addHitResult(), this would be 0)
				maxPossibleCombo = m_iComboFull; // current maximum possible combo at this time
				numCircles = clamp<int>(beatmap->getNumCirclesForCurrentTime() + 1, 0, beatmap->getSelectedDifficulty()->hitcircles.size()); // current maximum number of circles at this time (+1 because of 1 frame delay in update())

				///beatmap->getSelectedDifficulty()->calculateStarDiff(beatmap, &aimStars, &speedStars, curHitobjectIndex); // recalculating this live costs too much time
				aimStars = beatmap->getSelectedDifficulty()->getAimStarsForUpToHitObjectIndex(curHitobjectIndex);
				speedStars = beatmap->getSelectedDifficulty()->getSpeedStarsForUpToHitObjectIndex(curHitobjectIndex);

				m_fPPv2 = beatmap->getSelectedDifficulty()->calculatePPv2(m_osu, beatmap, aimStars, speedStars, -1, numCircles, maxPossibleCombo, m_iComboMax, m_iNumMisses, m_iNum300s, m_iNum100s, m_iNum50s);

				if (osu_debug_pp.getBool())
					debugLog("pp = %f, aimstars = %f, speedstars = %f, curindex = %i, maxpossiblecombo = %i, numcircles = %i\n", m_fPPv2, aimStars, speedStars, curHitobjectIndex, maxPossibleCombo, numCircles);
			}
		}
		else
			m_fPPv2 = 0.0f;
	}
}

void OsuScore::addSliderBreak()
{
	m_iCombo = 0;
	m_iNumSliderBreaks++;
}

void OsuScore::addPoints(int points, bool isSpinner)
{
	m_iScoreV1 += (unsigned long long)points;

	if (isSpinner)
		m_iBonusPoints += points; // only used for scorev2 calculation currently
}

unsigned long long OsuScore::getScore()
{
	return m_osu->getModScorev2() ? m_iScoreV2 : m_iScoreV1;
}
