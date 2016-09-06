//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap browser and selector
//
// $NoKeywords: $osusb
//===============================================================================//

#ifndef OSUSONGBROWSER2_H
#define OSUSONGBROWSER2_H

#include "OsuScreenBackable.h"
#include "MouseListener.h"

class Osu;
class OsuBeatmap;
class OsuBeatmapDifficulty;

class OsuUISelectionButton;
class OsuUISongBrowserInfoLabel;
class OsuUISongBrowserButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUIScrollView;

class McFont;
class ConVar;
class Timer;

class OsuSongBrowser2 : public OsuScreenBackable, public MouseListener
{
public:
	OsuSongBrowser2(Osu *osu);
	virtual ~OsuSongBrowser2();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);
	void onKeyUp(KeyboardEvent &e);
	void onChar(KeyboardEvent &e);

	void onLeftChange(bool down);
	void onMiddleChange(bool down){;}
	void onRightChange(bool down);

	void onWheelVertical(int delta){;}
	void onWheelHorizontal(int delta){;}

	void onResolutionChange(Vector2 newResolution);

	void onPlayEnd(bool quit = true);	// called when a beatmap is finished playing (or the player quit)

	void refreshBeatmaps();

	void onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool fromClick = false, bool play = false);
	void scrollToSongButton(OsuUISongBrowserButton *songButton);

	void setVisible(bool visible);

	inline bool hasSelectedAndIsPlaying() {return m_bHasSelectedAndIsPlaying;}
	inline OsuBeatmap *getSelectedBeatmap() const {return m_selectedBeatmap;}

	static bool searchMatcher(OsuBeatmap *beatmap, UString searchString);

private:
	virtual void updateLayout();
	void scheduleSearchUpdate(bool immediately = false);

	OsuUISelectionButton *addBottombarNavButton();

	ConVar *m_fps_max_ref;
	ConVar *m_osu_songbrowser_bottombar_percent_ref;

	virtual void onBack();
	void onSelectionMods();
	void onSelectionRandom();
	void onSelectionOptions();

	void selectSongButton(OsuUISongBrowserButton *songButton);
	void selectRandomBeatmap();
	void selectPreviousRandomBeatmap();

	static bool findSubstringInDifficulty(OsuBeatmapDifficulty *diff, UString searchString);

	Osu *m_osu;

	// top bar
	float m_fSongSelectTopScale;

	// top bar left
	CBaseUIContainer *m_topbarLeft;
	OsuUISongBrowserInfoLabel *m_songInfo;

	// top bar right
	CBaseUIContainer *m_topbarRight;

	// bottom bar
	CBaseUIContainer *m_bottombar;
	std::vector<OsuUISelectionButton*> m_bottombarNavButtons;

	// song browser
	CBaseUIScrollView *m_songBrowser;

	// beatmap database
	Timer *m_importTimer;
	std::vector<OsuBeatmap*> m_beatmaps;
	std::vector<OsuUISongBrowserButton*> m_songButtons;
	bool m_bBeatmapRefreshScheduled;
	bool m_bBeatmapRefreshNeedsFileRefresh;
	int m_iCurRefreshNumBeatmaps;
	int m_iCurRefreshBeatmap;
	UString m_sCurRefreshOsuSongFolder;
	std::vector<UString> m_refreshBeatmaps;
	UString m_sLastOsuFolder;

	// keys
	bool m_bF1Pressed;
	bool m_bF2Pressed;
	bool m_bShiftPressed;
	bool m_bLeft;
	bool m_bRight;
	bool m_bRandomBeatmapScheduled;
	bool m_bPreviousRandomBeatmapScheduled;

	// behaviour
	OsuBeatmap *m_selectedBeatmap;
	bool m_bHasSelectedAndIsPlaying;
	float m_fPulseAnimation;
	std::vector<OsuBeatmap*> m_previousRandomBeatmaps;

	// search
	std::vector<OsuUISongBrowserButton*> m_visibleSongButtons;
	UString m_sSearchString;
	float m_fSearchWaitTime;
};

#endif
