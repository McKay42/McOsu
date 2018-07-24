//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		pause menu (while playing)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUPAUSEMENU_H
#define OSUPAUSEMENU_H

#include "OsuScreen.h"

class Osu;
class OsuSongBrowser;
class CBaseUIContainer;
class OsuUIPauseMenuButton;

class OsuPauseMenu : public OsuScreen
{
public:
	OsuPauseMenu(Osu *osu);
	virtual ~OsuPauseMenu();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);
	void onKeyUp(KeyboardEvent &e);
	void onChar(KeyboardEvent &e);

	void onResolutionChange(Vector2 newResolution);

	void setVisible(bool visible);
	void setContinueEnabled(bool continueEnabled);

private:
	void updateLayout();
	void updateButtons();
	OsuUIPauseMenuButton *addButton();
	void setButton(int i, Image *img);

	void onContinueClicked();
	void onRetryClicked();
	void onBackClicked();

	void onSelectionChange();

	void scheduleVisibilityChange(bool visible);

	CBaseUIContainer *m_container;
	bool m_bScheduledVisibilityChange;
	bool m_bScheduledVisibility;

	std::vector<OsuUIPauseMenuButton*> m_buttons;
	OsuUIPauseMenuButton *m_selectedButton;
	float m_fWarningArrowsAnimStartTime;
	float m_fWarningArrowsAnimAlpha;
	float m_fWarningArrowsAnimX;
	float m_fWarningArrowsAnimY;
	bool m_bInitialWarningArrowFlyIn;

	bool m_bContinueEnabled;
	bool m_bClick1Down;
	bool m_bClick2Down;
};

#endif
