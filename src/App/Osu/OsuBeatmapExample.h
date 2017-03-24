//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		example usage of the beatmap class for a custom gamemode
//
// $NoKeywords: $osubmex
//===============================================================================//

#ifndef OSUBEATMAPEXAMPLE_H
#define OSUBEATMAPEXAMPLE_H

#include "OsuBeatmap.h"

class OsuBeatmapExample : public OsuBeatmap
{
public:
	OsuBeatmapExample(Osu *osu);

	virtual void draw(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void update();

	virtual void onModUpdate();
	virtual bool isLoading();

private:
	virtual void onBeforeLoad();
	virtual void onLoad();
	virtual void onPlayStart();
	virtual void onBeforeStop();
	virtual void onStop();
	virtual void onPaused();

	bool m_bFakeExtraLoading;
	float m_fFakeExtraLoadingTime;
};

#endif
