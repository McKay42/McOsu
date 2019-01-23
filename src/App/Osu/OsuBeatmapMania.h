//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!mania gamemode
//
// $NoKeywords: $osumania
//===============================================================================//

#ifndef OSU_OSUBEATMAPMANIA_H
#define OSU_OSUBEATMAPMANIA_H

#include "OsuBeatmap.h"

class OsuBeatmapMania : public OsuBeatmap
{
public:
	OsuBeatmapMania(Osu *osu);

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &key);
	virtual void onKeyUp(KeyboardEvent &key);

	inline Vector2 getPlayfieldSize() {return m_vPlayfieldSize;}
	inline Vector2 getPlayfieldCenter() {return m_vPlayfieldCenter;}

	int getNumColumns();

private:
	static int getColumnForKey(int numColumns, KeyboardEvent &key);

	Vector2 m_vPlayfieldSize;
	Vector2 m_vPlayfieldCenter;

	bool m_bColumnKeyDown[128];

	Vector2 m_vRotation;
	Vector2 m_vMouseBackup;
	float m_fZoom;
};

#endif
