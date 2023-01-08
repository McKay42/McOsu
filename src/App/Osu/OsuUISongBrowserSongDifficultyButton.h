//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap difficulty button (child of OsuUISongBrowserSongButton)
//
// $NoKeywords: $osusbsdb
//===============================================================================//

#ifndef OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H
#define OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H

#include "OsuUISongBrowserSongButton.h"

class ConVar;

class OsuUISongBrowserSongDifficultyButton : public OsuUISongBrowserSongButton
{
public:
	OsuUISongBrowserSongDifficultyButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, OsuDatabaseBeatmap *diff2, OsuUISongBrowserSongButton *parentSongButton);
	virtual ~OsuUISongBrowserSongDifficultyButton();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void updateGrade();

	inline OsuUISongBrowserSongButton *getParentSongButton() const {return m_parentSongButton;}

	bool isIndependentDiffButton() const;

private:
	static ConVar *m_osu_scores_enabled;
	static ConVar *m_osu_songbrowser_dynamic_star_recalc_ref;

	virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);

	UString buildDiffString() {return m_sDiff;}

	UString m_sDiff;

	float m_fDiffScale;
	float m_fOffsetPercentAnim;

	OsuUISongBrowserSongButton *m_parentSongButton;

	bool m_bUpdateGradeScheduled;
	bool m_bPrevOffsetPercentSelectionState;
};

#endif
