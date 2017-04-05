//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		score handler (calculation, storage)
//
// $NoKeywords: $osu
//===============================================================================//

#include "OsuScore.h"

#include "ConVar.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuHUD.h"
#include "OsuGameRules.h"

ConVar osu_hiterrorbar_misses("osu_hiterrorbar_misses", true);

OsuScore::OsuScore(Osu *osu)
{
	m_osu = osu;
	reset();
}

void OsuScore::reset()
{
	m_fPPv2 = 0.0f;
	m_grade = OsuScore::GRADE::GRADE_N;
	m_iScore = 0;
	m_iCombo = 0;
	m_iComboMax = 0;
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
	const int scoreComboMultiplier = std::max(m_iCombo-1, 0);

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

	// store the result, get hit value
	int hitValue = 0;
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

	// add hitValue to score
	const float sumDifficultyPoints = beatmap->getCS() + beatmap->getHP() + beatmap->getOD();
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
		m_iScore += hitValue + (int)(hitValue * (scoreComboMultiplier * difficultyMultiplier * m_osu->getScoreMultiplier()) / 25);

	const float totalHitPoints = m_iNum50s*(1.0f/6.0f)+ m_iNum100s*(2.0f/6.0f) + m_iNum300s;
	const float totalNumHits = m_iNumMisses + m_iNum50s + m_iNum100s + m_iNum300s;

	const float percent300s = m_iNum300s / totalNumHits;
	const float percent50s = m_iNum50s / totalNumHits;

	// recalculate accuracy
	if ((totalHitPoints == 0.0f || totalNumHits == 0.0f) && m_hitresults.size() < 1)
		m_fAccuracy = 1.0f;
	else
		m_fAccuracy = totalHitPoints / totalNumHits;

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
	///m_fPPv2 = beatmap->getSelectedDifficulty()->calculatePPv2()
}

void OsuScore::addSliderBreak()
{
	m_iCombo = 0;
	m_iNumSliderBreaks++;
}

void OsuScore::addPoints(int points)
{
	m_iScore += points;
}
