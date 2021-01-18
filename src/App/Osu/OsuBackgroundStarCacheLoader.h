//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by OsuBeatmapStandard for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#ifndef OSUBACKGROUNDSTARCACHELOADER_H
#define OSUBACKGROUNDSTARCACHELOADER_H

#include "Resource.h"

class OsuBeatmap;

class OsuBackgroundStarCacheLoader : public Resource
{
public:
	OsuBackgroundStarCacheLoader(OsuBeatmap *beatmap);

	bool isDead() {return m_bDead.load();}
	void kill() {m_bDead = true; m_iProgress = 0;}
	void revive() {m_bDead = false; m_iProgress = 0;}

	inline int getProgress() const {return m_iProgress.load();}

protected:
	virtual void init();
	virtual void initAsync();
	virtual void destroy() {;}

private:
	OsuBeatmap *m_beatmap;

	std::atomic<bool> m_bDead;
	std::atomic<int> m_iProgress;
};

#endif
