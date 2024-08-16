//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by OsuBeatmapStandard for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#include "OsuBackgroundStarCacheLoader.h"

#include "Engine.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuDifficultyCalculator.h"

OsuBackgroundStarCacheLoader::OsuBackgroundStarCacheLoader(OsuBeatmap *beatmap) : Resource()
{
	m_beatmap = beatmap;

	m_bDead = true; // NOTE: start dead! need to revive() before use
	m_iProgress = 0;
}

void OsuBackgroundStarCacheLoader::init()
{
	m_bReady = true;
}

void OsuBackgroundStarCacheLoader::initAsync()
{
	if (m_bDead.load())
	{
		m_bAsyncReady = true;
		return;
	}

	// recalculate star cache
	OsuDatabaseBeatmap *diff2 = m_beatmap->getSelectedDifficulty2();
	if (diff2 != NULL)
	{
		// precalculate cut star values for live pp

		// reset
		m_beatmap->m_aimStarsForNumHitObjects.clear();
		m_beatmap->m_aimSliderFactorForNumHitObjects.clear();
		m_beatmap->m_speedStarsForNumHitObjects.clear();
		m_beatmap->m_speedNotesForNumHitObjects.clear();

		const UString &osuFilePath = diff2->getFilePath();
		const Osu::GAMEMODE gameMode = m_beatmap->getOsu()->getGamemode();
		const float AR = m_beatmap->getAR();
		const float CS = m_beatmap->getCS();
		const float OD = m_beatmap->getOD();
		const float speedMultiplier = m_beatmap->getOsu()->getSpeedMultiplier(); // NOTE: not beatmap->getSpeedMultiplier()!
		const bool relax = m_beatmap->getOsu()->getModRelax();
		const bool touchDevice = m_beatmap->getOsu()->getModTD();

		OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier, false, m_bDead);

		double aimStars = 0.0;
		double aimSliderFactor = 0.0;
		double speedStars = 0.0;
		double speedNotes = 0.0;

		// new fast method  (build full cached DiffObjects once) (1/2)
		std::vector<OsuDifficultyCalculator::DiffObject> cachedDiffObjects;
		std::vector<OsuDifficultyCalculator::DiffObject> diffObjects;
		OsuDifficultyCalculator::calculateStarDiffForHitObjectsInt(cachedDiffObjects, diffObjects, diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aimStars, &aimSliderFactor, &speedStars, &speedNotes, -1, NULL, NULL, m_bDead);

		for (size_t i=0; i<diffres.diffobjects.size(); i++)
		{
			aimStars = 0.0;
			aimSliderFactor = 0.0;
			speedStars = 0.0;
			speedNotes = 0.0;

			// old slow method:
			/*
			double oldAimStars = 0.0;
			double oldAimSliderFactor = 0.0;
			double oldSpeedStars = 0.0;
			double oldSpeedNotes = 0.0;
			OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &oldAimStars, &oldAimSliderFactor, &oldSpeedStars, &oldSpeedNotes, i, NULL, NULL, m_bDead);
			*/
			//OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aimStars, &aimSliderFactor, &speedStars, &speedNotes, i, NULL, NULL, m_bDead);

			// new fast method (reuse cached DiffObjects instead of re-computing them every single iteration for the entire beatmap) (2/2)
			OsuDifficultyCalculator::calculateStarDiffForHitObjectsInt(cachedDiffObjects, diffObjects, diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aimStars, &aimSliderFactor, &speedStars, &speedNotes, i, NULL, NULL, m_bDead);
			/*
			const double deltaOldAimStars = std::abs(aimStars - oldAimStars);
			const double deltaOldAimSliderFactor = std::abs(aimSliderFactor - oldAimSliderFactor);
			const double deltaOldSpeedStars = std::abs(speedStars - oldSpeedStars);
			const double deltaOldSpeedNotes = std::abs(speedNotes - oldSpeedNotes);

			if (deltaOldAimStars > 0.00001f
			 || deltaOldAimSliderFactor > 0.00001f
			 || deltaOldSpeedStars > 0.00001f
			 || deltaOldSpeedNotes > 0.00001f)
			{
				debugLog("deltaOldAimStars = %f\n", deltaOldAimStars);
				debugLog("deltaOldAimSliderFactor = %f\n", deltaOldAimSliderFactor);
				debugLog("deltaOldSpeedStars = %f\n", deltaOldSpeedStars);
				debugLog("deltaOldSpeedNotes = %f\n", deltaOldSpeedNotes);
			}
			*/

			m_beatmap->m_aimStarsForNumHitObjects.push_back(aimStars);
			m_beatmap->m_aimSliderFactorForNumHitObjects.push_back(aimSliderFactor);
			m_beatmap->m_speedStarsForNumHitObjects.push_back(speedStars);
			m_beatmap->m_speedNotesForNumHitObjects.push_back(speedNotes);

			m_iProgress = i;

			if (m_bDead.load())
			{
				m_beatmap->m_aimStarsForNumHitObjects.clear();
				m_beatmap->m_aimSliderFactorForNumHitObjects.clear();
				m_beatmap->m_speedStarsForNumHitObjects.clear();
				m_beatmap->m_speedNotesForNumHitObjects.clear();

				break;
			}
		}
	}

	m_bAsyncReady = true;
}
