//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by OsuBeatmapStandard for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#include "OsuBackgroundStarCacheLoader.h"

#include "Engine.h"
#include "Timer.h"
#include "ConVar.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuDifficultyCalculator.h"

ConVar osu_debug_background_star_cache_loader("osu_debug_background_star_cache_loader", false, FCVAR_NONE, "prints the time it took to build the cache");

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
		m_beatmap->m_attributesForNumHitObjects.clear();

		const UString &osuFilePath = diff2->getFilePath();
		const Osu::GAMEMODE gameMode = m_beatmap->getOsu()->getGamemode();
		const float AR = m_beatmap->getAR(false);
		const float CS = m_beatmap->getCS();
		const float speedMultiplier = m_beatmap->getOsu()->getSpeedMultiplier(); // NOTE: not beatmap->getSpeedMultiplier()!

		OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier, false, m_bDead);

		// Create beatmap data from beatmap and init difficulty attributes
		OsuDifficultyCalculator::BeatmapDiffcalcData beatmapData(m_beatmap, diffres.diffobjects);
        Timer timer;
        timer.start();
        if (diffres.errorCode == 0)
        {
            auto attributes = OsuDifficultyCalculator::calculateLazerLiveDifficultyAttributes(beatmapData, m_bDead);
            if (!m_bDead.load())
            {
                m_beatmap->m_attributesForNumHitObjects.swap(attributes);
                m_iProgress = diffres.diffobjects.size();
            }
        }
        timer.update();
        if (osu_debug_background_star_cache_loader.getBool())
            debugLog("OsuBackgroundStarCacheLoader: Took %f sec\n", timer.getElapsedTime());

	}

	m_bAsyncReady = true;
}
