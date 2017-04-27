//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		example usage of the beatmap class for a custom gamemode
//
// $NoKeywords: $osubmex
//===============================================================================//

#include "OsuBeatmapExample.h"

#include "Engine.h"
#include "ResourceManager.h"

OsuBeatmapExample::OsuBeatmapExample(Osu *osu) : OsuBeatmap(osu)
{
	m_bFakeExtraLoading = true;
	m_fFakeExtraLoadingTime = 0.0f;
}

void OsuBeatmapExample::draw(Graphics *g)
{
	OsuBeatmap::draw(g);
	if (!canDraw()) return;

	if (m_bFakeExtraLoading)
	{
		g->pushTransform();
		g->translate(200, 200);
		g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("Fake Loading Time: %f", m_fFakeExtraLoadingTime - engine->getTime()));
		g->popTransform();
	}

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw all hitobjects and stuff
}

void OsuBeatmapExample::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	OsuBeatmap::drawVR(g, mvp, vr);
	if (!canDraw()) return;

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw all hitobjects and stuff
}

void OsuBeatmapExample::update()
{
	if (!canUpdate())
	{
		OsuBeatmap::update();
		return;
	}

	// baseclass call (does the actual hitobject updates among other things)
	OsuBeatmap::update();

	if (engine->getTime() > m_fFakeExtraLoadingTime)
	{
		m_bFakeExtraLoading = false;
	}

	if (isLoading()) return; // only continue if we have loaded everything

	// update everything else here
}

void OsuBeatmapExample::onModUpdate()
{
	debugLog("OsuBeatmapExample::onModUpdate()\n");
}

bool OsuBeatmapExample::isLoading()
{
	return OsuBeatmap::isLoading() || m_bFakeExtraLoading;
}

void OsuBeatmapExample::onBeforeLoad()
{
	// called before any hitobjects are loaded from disk
	debugLog("OsuBeatmapExample::onBeforeLoad()\n");

	m_bFakeExtraLoading = false;
}

void OsuBeatmapExample::onLoad()
{
	// called after all hitobjects have been loaded + created (m_hitobjects is now filled!)

	// this allows beatmaps to load extra things, like custom skin elements (see isLoading())
	// OsuBeatmap will show the loading spinner as long as isLoading() returns true, and delay the play start
	m_bFakeExtraLoading = true;
	m_fFakeExtraLoadingTime = engine->getTime() + 5.0f;
}

void OsuBeatmapExample::onPlayStart()
{
	// called at the exact moment when the player starts playing (after the potential wait-time due to early hitobjects!)
	// (is NOT called immediately when you click on a beatmap diff)
	debugLog("OsuBeatmapExample::onPlayStart()\n");
}

void OsuBeatmapExample::onBeforeStop(bool quit)
{
	// called before unloading all hitobjects, when the player stops playing
	debugLog("OsuBeatmapExample::onBeforeStop()\n");
}

void OsuBeatmapExample::onStop(bool quit)
{
	// called after unloading all hitobjects, when the player stops playing this beatmap and returns to the songbrowser
	debugLog("OsuBeatmapExample::onStop()\n");
}

void OsuBeatmapExample::onPaused()
{
	// called when the player pauses the game
	debugLog("OsuBeatmapExample::onPaused()\n");
}
