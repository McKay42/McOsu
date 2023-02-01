//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by OsuBeatmapStandard for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#include "OsuBackgroundStarCacheLoader.h"

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
		m_beatmap->m_speedStarsForNumHitObjects.clear();

		const UString &osuFilePath = diff2->getFilePath();
		const Osu::GAMEMODE gameMode = m_beatmap->getOsu()->getGamemode();
		const float AR = m_beatmap->getAR();
		const float CS = m_beatmap->getCS();
		const float OD = m_beatmap->getOD();
		const float speedMultiplier = m_beatmap->getOsu()->getSpeedMultiplier(); // NOTE: not beatmap->getSpeedMultiplier()!

		OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier);

		for (size_t i=0; i<diffres.diffobjects.size(); i++)
		{
			double aimStars = 0.0;
			double speedStars = 0.0;

			OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, &aimStars, &speedStars, i);

			m_beatmap->m_aimStarsForNumHitObjects.push_back(aimStars);
			m_beatmap->m_speedStarsForNumHitObjects.push_back(speedStars);

			m_iProgress = i;

			if (m_bDead.load())
			{
				m_beatmap->m_aimStarsForNumHitObjects.clear();
				m_beatmap->m_speedStarsForNumHitObjects.clear();

				break;
			}
		}
	}

	m_bAsyncReady = true;
}
