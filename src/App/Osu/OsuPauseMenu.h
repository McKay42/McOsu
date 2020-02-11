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

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void setVisible(bool visible);

	void setContinueEnabled(bool continueEnabled);

private:
	void updateLayout();

	void onContinueClicked();
	void onRetryClicked();
	void onBackClicked();

	void onSelectionChange();

	void scheduleVisibilityChange(bool visible);

	OsuUIPauseMenuButton *addButton(std::function<Image*()> getImageFunc);

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

	float m_fDimAnim;
};

#endif
