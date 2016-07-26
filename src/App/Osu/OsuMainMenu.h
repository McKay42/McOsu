//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		main menu
//
// $NoKeywords: $osumain
//===============================================================================//

#ifndef OSUMENUMAIN_H
#define OSUMENUMAIN_H

#include "OsuScreen.h"

class Osu;
class OsuMainMenuMainButton;
class OsuMainMenuButton;

class CBaseUIButton;
class CBaseUIContainer;

class OsuMainMenu : public OsuScreen
{
public:
	friend class OsuMainMenuMainButton;
	friend class OsuMainMenuButton;

	OsuMainMenu(Osu *osu);
	~OsuMainMenu();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);

	void onResolutionChange(Vector2 newResolution);

	void setVisible(bool visible);

	inline Osu* getOsu() const {return m_osu;}

private:
	void drawVersionInfo(Graphics *g);
	void updateLayout();

	void setMenuElementsVisible(bool visible, bool animate = true);

	OsuMainMenuButton *addMainMenuButton(UString text);

	void onMainMenuButtonPressed();
	void onPlayButtonPressed();
	void onOptionsButtonPressed();
	void onExitButtonPressed();

	void onPausePressed();

	Osu *m_osu;

	Vector2 m_vSize;
	Vector2 m_vCenter;
	float m_fSizeAddAnim;
	float m_fCenterOffsetAnim;

	bool m_bMenuElementsVisible;
	float m_fMainMenuButtonCloseTime;

	float m_fShutdownScheduledTime;

	CBaseUIContainer *m_container;
	OsuMainMenuMainButton *m_mainButton;
	std::vector<OsuMainMenuButton*> m_menuElements;

	CBaseUIButton *m_pauseButton;

	std::vector<UString> m_todo;
};

#endif
