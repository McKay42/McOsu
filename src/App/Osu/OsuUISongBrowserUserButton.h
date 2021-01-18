//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		user card/button (shows total weighted pp + user switcher)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISONGBROWSERUSERBUTTON_H
#define OSUUISONGBROWSERUSERBUTTON_H

#include "CBaseUIButton.h"

class ConVar;

class Osu;

class OsuUISongBrowserUserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserUserButton(Osu *osu);

	virtual void draw(Graphics *g);
	virtual void update();

	void updateUserStats();

	void addTooltipLine(UString text) {m_vTooltipLines.push_back(text);}

private:
	virtual void onMouseInside();
	virtual void onMouseOutside();

	ConVar *m_osu_scores_enabled_ref;

	Osu *m_osu;

	float m_fPP;
	float m_fAcc;
	int m_iLevel;
	float m_fPercentToNextLevel;

	float m_fPPDelta;
	float m_fPPDeltaAnim;

	float m_fHoverAnim;
	std::vector<UString> m_vTooltipLines;
};

#endif
