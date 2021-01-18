//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		unfinished spectator client
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSU2_H
#define OSU2_H

#include "App.h"
#include "Osu.h"

class OsuBeatmap;
class OsuDatabaseBeatmap;

class Osu2 : public App
{
public:
	Osu2();
	virtual ~Osu2();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &key);
	virtual void onKeyUp(KeyboardEvent &key);
	virtual void onChar(KeyboardEvent &charCode);

	virtual void onResolutionChanged(Vector2 newResolution);

	virtual void onFocusGained();
	virtual void onFocusLost();

	virtual void onMinimized();
	virtual void onRestored();

	virtual bool onShutdown();

	inline int getNumInstances() const {return m_instances.size();}

private:
	void onSkinChange(UString oldValue, UString newValue);

	Osu *m_osu;
	std::vector<Osu*> m_slaves;
	std::vector<Osu*> m_instances;

	OsuDatabaseBeatmap *m_prevBeatmapDifficulty;
	bool m_bSlavesLoaded;

	// hack
	bool m_bPrevPlayingState;
};

#endif
