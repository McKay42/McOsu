//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		main menu
//
// $NoKeywords: $osumain
//===============================================================================//

#ifndef OSUMENUMAIN_H
#define OSUMENUMAIN_H

#include "OsuScreen.h"
#include "MouseListener.h"

class Osu;
class OsuUpdateChecker;

class OsuMainMenuMainButton;
class OsuMainMenuButton;
class OsuUIButton;

class CBaseUIButton;
class CBaseUIContainer;

class OsuMainMenu : public OsuScreen, public MouseListener
{
public:
	static UString MCOSU_MAIN_BUTTON_TEXT;

public:
	friend class OsuMainMenuMainButton;
	friend class OsuMainMenuButton;

	OsuMainMenu(Osu *osu);
	~OsuMainMenu();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);

	void onLeftChange(bool down){;}
	void onMiddleChange(bool down);
	void onRightChange(bool down){;}

	void onWheelVertical(int delta){;}
	void onWheelHorizontal(int delta){;}

	void onResolutionChange(Vector2 newResolution);

	void setVisible(bool visible);

	inline Osu* getOsu() const {return m_osu;}

private:
	void drawVersionInfo(Graphics *g);
	void updateLayout();

	void animMainButton();
	void animMainButtonBack();

	void setMenuElementsVisible(bool visible, bool animate = true);

	OsuMainMenuButton *addMainMenuButton(UString text);

	void onMainMenuButtonPressed();
	void onPlayButtonPressed();
	void onOptionsButtonPressed();
	void onExitButtonPressed();

	void onPausePressed();
	void onUpdatePressed();
	void onDonatePressed();

	Osu *m_osu;

	OsuUpdateChecker *m_updateChecker;
	bool m_bUpdateStatus;
	bool m_bUpdateCheckFinished;
	float m_fUpdateStatusTime;
	float m_fUpdateButtonAnim;
	bool m_bHasClickedUpdate;

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
	OsuUIButton *m_updateAvailableButton;
	OsuUIButton *m_donationButton;

	std::vector<UString> m_todo;

	float m_fMainMenuAnimTime;
	float m_fMainMenuAnim;
	float m_fMainMenuAnim1;
	float m_fMainMenuAnim2;
	float m_fMainMenuAnim3;
	float m_fMainMenuAnim1Target;
	float m_fMainMenuAnim2Target;
	float m_fMainMenuAnim3Target;
};

#endif
