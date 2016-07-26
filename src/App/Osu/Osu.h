//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#ifndef OSU_H
#define OSU_H

#include "App.h"
#include "MouseListener.h"

class OsuMainMenu;
class OsuPauseMenu;
class OsuOptionsMenu;
class OsuModSelector;
class OsuSongBrowser;
class OsuSongBrowser2;
class OsuNotificationOverlay;
class OsuTooltipOverlay;
class OsuBeatmap;
class OsuScreen;
class OsuSkin;
class OsuHUD;

class ConVar;
class Image;
class McFont;
class RenderTarget;

class Osu : public App, public MouseListener
{
public:
	static inline Vector2 getScreenSize() {return g_vInternalResolution;}
	static inline int getScreenWidth() {return (int)g_vInternalResolution.x;}
	static inline int getScreenHeight() {return (int)g_vInternalResolution.y;}

	static Vector2 osuBaseResolution;

	static float getImageScaleToFitResolution(Image *img, Vector2 resolution);
	static float getImageScaleToFitResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Image *img, Vector2 resolution);
	static float getImageScale(Vector2 size, float osuSize);
	static float getImageScale(Image *img, float osuSize);
	static float getUIScale(float osuResolutionRatio);

	static bool findIgnoreCase(const std::string &haystack, const std::string &needle);

public:
	Osu();
	virtual ~Osu();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	void onLeftChange(bool down);
	void onMiddleChange(bool down){;}
	void onRightChange(bool down);

	void onWheelVertical(int delta){;}
	void onWheelHorizontal(int delta){;}

	virtual void onResolutionChanged(Vector2 newResolution);
	virtual void onFocusGained();
	virtual void onFocusLost();

	void onBeforePlayStart();			// called just before OsuBeatmap->play()
	void onPlayStart();					// called when a beatmap has successfully started playing
	void onPlayEnd(bool quit = true);	// called when a beatmap is finished playing (or the player quit)

	void toggleModSelection(bool waitForF1KeyUp = false);
	void toggleSongBrowser();
	void toggleOptionsMenu();

	void volumeDown();
	void volumeUp();

	void reloadSkin() {onSkinReload();}

	float getCursorScaleFactor();

	inline OsuSkin *getSkin() {return m_skin;}
	inline OsuHUD *getHUD() {return m_hud;}
	inline OsuNotificationOverlay *getNotificationOverlay() {return m_notificationOverlay;}
	inline OsuTooltipOverlay *getTooltipOverlay() {return m_tooltipOverlay;}
	inline OsuModSelector *getModSelector() {return m_modSelector;}

	inline RenderTarget *getFrameBuffer() {return m_frameBuffer;}
	inline McFont *getTitleFont() {return m_titleFont;}
	inline McFont *getSubTitleFont() {return m_subTitleFont;}
	inline McFont *getSongBrowserFont() {return m_songBrowserFont;}
	inline McFont *getSongBrowserFontBold() {return m_songBrowserFontBold;}

	OsuBeatmap *getBeatmap();

	float getDifficultyMultiplier();
	float getCSDifficultyMultiplier();
	float getRawSpeedMultiplier();	// without override
	float getSpeedMultiplier();		// with override
	float getPitchMultiplier();

	inline bool getModAuto() {return m_bModAuto;}
	inline bool getModAutopilot() {return m_bModAutopilot;}
	inline bool getModRelax() {return m_bModRelax;}
	inline bool getModSpunout() {return m_bModSpunout;}
	inline bool getModDT() {return m_bModDT;}
	inline bool getModNC() {return m_bModNC;}
	inline bool getModHT() {return m_bModHT;}
	inline bool getModHD() {return m_bModHD;}
	inline bool getModHR() {return m_bModHR;}
	inline bool getModEZ() {return m_bModEZ;}
	inline bool getModSD() {return m_bModSD;}
	inline bool getModSS() {return m_bModSS;}
	inline bool getModNM() {return m_bModNM;}

	bool isInPlayMode();
	OsuBeatmap *getSelectedBeatmap();

	void updateMods();
	void updateConfineCursor();

private:
	static Vector2 g_vInternalResolution;

	void updateModsForConVarTemplate(UString oldValue, UString newValue) {updateMods();}

	// callbacks
	void onInternalResolutionChanged(UString oldValue, UString args);

	void onSkinReload();
	void onSkinChange(UString oldValue, UString newValue);

	void onMasterVolumeChange(UString oldValue, UString newValue);
	void onMusicVolumeChange(UString oldValue, UString newValue);
	void onSpeedChange(UString oldValue, UString newValue);
	void onPitchChange(UString oldValue, UString newValue);

	void onLetterboxingChange(UString oldValue, UString newValue);

	void onConfineCursorWindowedChange(UString oldValue, UString newValue);
	void onConfineCursorFullscreenChange(UString oldValue, UString newValue);

	void onKey1Change(bool pressed, bool mouse);
	void onKey2Change(bool pressed, bool mouse);

	// convar refs
	ConVar *m_osu_folder_ref;
	ConVar *m_osu_draw_hud_ref;
	ConVar *m_osu_mod_fps_ref;

	// interfaces
	OsuMainMenu *m_mainMenu;
	OsuOptionsMenu *m_optionsMenu;
	OsuSongBrowser *m_songBrowser;
	OsuSongBrowser2 *m_songBrowser2;
	OsuModSelector *m_modSelector;
	OsuPauseMenu *m_pauseMenu;
	OsuSkin *m_skin;
	OsuHUD *m_hud;
	OsuTooltipOverlay *m_tooltipOverlay;
	OsuNotificationOverlay *m_notificationOverlay;

	std::vector<OsuScreen*> m_screens;

	// rendering
	RenderTarget *m_frameBuffer;
	RenderTarget *m_backBuffer;
	Vector2 m_vInternalResolution;

	// mods
	bool m_bModAuto;
	bool m_bModAutopilot;
	bool m_bModRelax;
	bool m_bModSpunout;
	bool m_bModDT;
	bool m_bModNC;
	bool m_bModHT;
	bool m_bModHD;
	bool m_bModHR;
	bool m_bModEZ;
	bool m_bModSD;
	bool m_bModSS;
	bool m_bModNM;

	// keys
	bool m_bF1;
	bool m_bUIToggleCheck;
	bool m_bTab;
	bool m_bEscape;
	bool m_bKey1;
	bool m_bKey2;
	bool m_bSkipScheduled;
	bool m_bQuickRetryDown;
	float m_fQuickRetryTime;

	// async toggles
	bool m_bToggleModSelectionScheduled;
	bool m_bToggleSongBrowserScheduled;
	bool m_bToggleOptionsMenuScheduled;

	// cursor
	bool m_bShouldCursorBeVisible;

	// global resources
	McFont *m_titleFont;
	McFont *m_subTitleFont;
	McFont *m_songBrowserFont;
	McFont *m_songBrowserFontBold;
};

#endif
