//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap difficulty button (child of OsuUISongBrowserSongButton)
//
// $NoKeywords: $osusbsdb
//===============================================================================//

#ifndef OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H
#define OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H

#include "OsuUISongBrowserSongButton.h"

class OsuUISongBrowserSongDifficultyButton : public OsuUISongBrowserSongButton
{
public:
	OsuUISongBrowserSongDifficultyButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff);
	virtual ~OsuUISongBrowserSongDifficultyButton() {;}

	virtual void draw(Graphics *g);

private:
	static OsuUISongBrowserSongDifficultyButton *previousButton;

	virtual void onSelected(bool wasSelected);
	virtual void onDeselected();

	UString buildDiffString()
	{
		return m_sDiff;
	}

	OsuBeatmap *m_beatmap;

	UString m_sDiff;

	float m_fDiffScale;
};

#endif
