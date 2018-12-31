//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		user card/button (shows total weighted pp + user switcher)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISONGBROWSERUSERBUTTON_H
#define OSUUISONGBROWSERUSERBUTTON_H

#include "CBaseUIButton.h"

class Osu;

class OsuUISongBrowserUserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserUserButton(Osu *osu);

	virtual void draw(Graphics *g);
	virtual void update();

	void updateUserStats();

private:
	virtual void onMouseInside();
	virtual void onMouseOutside();

	Osu *m_osu;

	float m_fPP;
	float m_fAcc;
	int m_iLevel;
	float m_fPercentToNextLevel;

	int m_iPPIncrease;

	float m_fHoverAnim;
};

#endif
