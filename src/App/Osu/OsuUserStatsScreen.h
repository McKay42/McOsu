//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		top plays list for weighted pp/acc
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUSERSTATSSCREEN_H
#define OSUUSERSTATSSCREEN_H

#include "OsuScreenBackable.h"

class ConVar;

class CBaseUIContainer;
class CBaseUIScrollView;

class OsuUIContextMenu;
class OsuUISongBrowserUserButton;
class OsuUISongBrowserScoreButton;

class OsuUserStatsScreen : public OsuScreenBackable
{
public:
	OsuUserStatsScreen(Osu *osu);
	virtual ~OsuUserStatsScreen();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void setVisible(bool visible);

private:
	virtual void updateLayout();

	virtual void onBack();

	void rebuildScoreButtons(UString playerName);

	void onUserClicked(CBaseUIButton *button);
	void onUserButtonChange(UString text, int id);
	void onScoreClicked(CBaseUIButton *button);

	ConVar *m_name_ref;

	CBaseUIContainer *m_container;

	OsuUIContextMenu *m_contextMenu;

	OsuUISongBrowserUserButton *m_userButton;

	CBaseUIScrollView *m_scores;
	std::vector<OsuUISongBrowserScoreButton*> m_scoreButtons;
};

#endif
