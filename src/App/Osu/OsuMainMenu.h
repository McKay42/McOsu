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

class OsuBeatmapStandard;
class OsuDatabaseBeatmap;

class OsuHitObject;

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

	void setStartupAnim(bool startupAnim) {m_bStartupAnim = startupAnim; m_fStartupAnim = m_fStartupAnim2 = (m_bStartupAnim ? 0.0f : 1.0f);}

	inline Osu* getOsu() const {return m_osu;}

private:
	static ConVar *m_osu_universal_offset_ref;
	static ConVar *m_osu_universal_offset_hardcoded_ref;
	static ConVar *m_osu_old_beatmap_offset_ref;
	static ConVar *m_win_snd_fallback_dsound_ref;
	static ConVar *m_osu_universal_offset_hardcoded_fallback_dsound_ref;
	static ConVar *m_osu_slider_border_feather_ref;
	static ConVar *m_osu_mod_random_ref;
	static ConVar *m_osu_songbrowser_background_fade_in_duration_ref;

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
	bool m_bDidUserUpdateFromOlderVersion;
	bool m_bDidUserUpdateFromOlderVersionLe3300;
	bool m_bDidUserUpdateFromOlderVersionLe3303;
	bool m_bDidUserUpdateFromOlderVersionLe3308;
	bool m_bDidUserUpdateFromOlderVersionLe3310;
	bool m_bDidUserUpdateFromOlderVersionLe3311;
	bool m_bDidUserUpdateFromOlderVersionLe3312;

	// custom
	float m_fMainMenuAnimTime;
	float m_fMainMenuAnimDuration;
	float m_fMainMenuAnim;
	float m_fMainMenuAnim1;
	float m_fMainMenuAnim2;
	float m_fMainMenuAnim3;
	float m_fMainMenuAnim1Target;
	float m_fMainMenuAnim2Target;
	float m_fMainMenuAnim3Target;
	bool m_bInMainMenuRandomAnim;
	int m_iMainMenuRandomAnimType;
	unsigned int m_iMainMenuAnimBeatCounter;

	bool m_bMainMenuAnimFriend;
	bool m_bMainMenuAnimFadeToFriendForNextAnim;
	bool m_bMainMenuAnimFriendScheduled;
	float m_fMainMenuAnimFriendPercent;
	float m_fMainMenuAnimFriendEyeFollowX;
	float m_fMainMenuAnimFriendEyeFollowY;

	float m_fShutdownScheduledTime;
	bool m_bWasCleanShutdown;

	bool m_bStartupAnim;
	float m_fStartupAnim;
	float m_fStartupAnim2;

	OsuDatabaseBeatmap *m_mainMenuSliderTextDatabaseBeatmap;
	OsuBeatmapStandard *m_mainMenuSliderTextBeatmapStandard;
	std::vector<OsuHitObject*> m_mainMenuSliderTextBeatmapHitObjects;
	float m_fMainMenuSliderTextRawHitCircleDiameter;

	float m_fPrevShuffleTime;
	float m_fBackgroundFadeInTime;
};

#endif
