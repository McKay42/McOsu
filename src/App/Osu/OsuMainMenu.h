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

class OsuMainMenuPauseButton;
class OsuMainMenuMainButton;
class OsuMainMenuButton;
class OsuUIButton;

class CBaseUIButton;
class CBaseUIContainer;

class ConVar;

class OsuMainMenu : public OsuScreen, public MouseListener
{
public:
	static UString MCOSU_MAIN_BUTTON_TEXT;
	static UString MCOSU_MAIN_BUTTON_SUBTEXT;

	static void openSteamWorkshopInGameOverlay(Osu *osu, bool launchInSteamIfOverlayDisabled = false);
	static void openSteamWorkshopInDefaultBrowser(bool launchInSteam = false);

public:
	friend class OsuMainMenuMainButton;
	friend class OsuMainMenuButton;

	OsuMainMenu(Osu *osu);
	virtual ~OsuMainMenu();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);

	virtual void onLeftChange(bool down){;}
	virtual void onMiddleChange(bool down);
	virtual void onRightChange(bool down){;}

	virtual void onWheelVertical(int delta){;}
	virtual void onWheelHorizontal(int delta){;}

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void setVisible(bool visible);

	inline Osu* getOsu() const {return m_osu;}

private:
	static ConVar *m_osu_universal_offset_ref;
	static ConVar *m_osu_universal_offset_hardcoded_ref;

	void drawVersionInfo(Graphics *g);
	void updateLayout();

	void animMainButton();
	void animMainButtonBack();

	void setMenuElementsVisible(bool visible, bool animate = true);

	void writeVersionFile();

	OsuMainMenuButton *addMainMenuButton(UString text);

	void onMainMenuButtonPressed();
	void onPlayButtonPressed();
	void onEditButtonPressed();
	void onOptionsButtonPressed();
	void onExitButtonPressed();

	void onPausePressed();
	void onUpdatePressed();
	void onSteamWorkshopPressed();
	void onGithubPressed();
	void onVersionPressed();

	float m_fUpdateStatusTime;
	float m_fUpdateButtonTextTime;
	float m_fUpdateButtonAnimTime;
	float m_fUpdateButtonAnim;
	bool m_bHasClickedUpdate;

	Vector2 m_vSize;
	Vector2 m_vCenter;
	float m_fSizeAddAnim;
	float m_fCenterOffsetAnim;

	bool m_bMenuElementsVisible;
	float m_fMainMenuButtonCloseTime;

	CBaseUIContainer *m_container;
	OsuMainMenuMainButton *m_mainButton;
	std::vector<OsuMainMenuButton*> m_menuElements;

	OsuMainMenuPauseButton *m_pauseButton;
	OsuUIButton *m_updateAvailableButton;
	OsuUIButton *m_steamWorkshopButton;
	OsuUIButton *m_githubButton;
	CBaseUIButton *m_versionButton;

	bool m_bDrawVersionNotificationArrow;

	// custom
	float m_fMainMenuAnimTime;
	float m_fMainMenuAnim;
	float m_fMainMenuAnim1;
	float m_fMainMenuAnim2;
	float m_fMainMenuAnim3;
	float m_fMainMenuAnim1Target;
	float m_fMainMenuAnim2Target;
	float m_fMainMenuAnim3Target;
	bool m_bInMainMenuRandomAnim;
	int m_bMainMenuRandomAnimType;

	float m_fShutdownScheduledTime;
	bool m_bWasCleanShutdown;
};

#endif
