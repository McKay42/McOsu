//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		debug beatmap browser and selector
//
// $NoKeywords: $osusbdbg
//===============================================================================//

#ifndef OSUSONGBROWSER_H
#define OSUSONGBROWSER_H

#include "OsuScreenBackable.h"
#include "MouseListener.h"

class Osu;
class OsuBeatmap;
class CBaseUIContainer;
class OsuUISelectionButton;
class CBaseUIImageButton;
class McFont;
class ConVar;

class OsuSongBrowser : public OsuScreenBackable, public MouseListener
{
public:
	OsuSongBrowser(Osu *osu);
	virtual ~OsuSongBrowser();

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

	void onStartDebugMap();

	void refreshBeatmaps();

	void setVisible(bool visible);

	inline bool hasSelectedAndIsPlaying() {return m_bHasSelectedAndIsPlaying;}
	inline bool isBeatmapRefreshScheduled() {return m_bBeatmapRefreshScheduled;}
	inline bool hasSearchTextEntered() {return m_sSearchString.length() > 0;}
	inline OsuBeatmap *getSelectedBeatmap() {return m_selectedBeatmap;}

private:
	void updateLayout();

	OsuUISelectionButton *addBottombarNavButton();

	void onBack();
	void onSelectionMods();
	void onSelectionRandom();
	void onSelectionOptions();

	void selectBeatmap(OsuBeatmap *selectedBeatmap, bool isFromClick = false);
	void scheduleSearchUpdate();
	void selectRandomBeatmap();
	void selectPreviousRandomBeatmap();
	void scrollToBeatmap(OsuBeatmap *beatmap);

	ConVar *m_fps_max_ref;

	Osu *m_osu;
	McFont *m_songFont;

	bool m_bHasSelectedAndIsPlaying;

	float m_fSongScrollPos;
	float m_fSongScrollVelocity;
	float m_fSongTextHeight;
	float m_fSongTextOffset;

	bool m_bLeftMouse;
	bool m_bLeftMouseDown;
	bool m_bScrollGrab;
	float m_fPrevSongScrollPos;
	Vector2 m_vPrevMousePos;
	Vector2 m_vMouseBackup2;
	Vector2 m_vKineticAverage;
	Vector2 m_vScrollStartMousePos;
	bool m_bDisableClick;
	UString m_sSearchString;
	float m_fSearchWaitTime;
	bool m_bF1Pressed;
	bool m_bF2Pressed;
	bool m_bShiftPressed;
	std::vector<int> m_previousRandomBeatmaps;

	bool m_bRandomBeatmapScheduled;
	bool m_bPreviousRandomBeatmapScheduled;

	bool m_bBeatmapRefreshScheduled;
	bool m_bBeatmapRefreshNeedsFileRefresh;
	int m_iCurRefreshNumBeatmaps;
	int m_iCurRefreshBeatmap;
	UString m_sCurRefreshOsuSongFolder;
	std::vector<UString> m_refreshBeatmaps;
	UString m_sLastOsuFolder;

	OsuBeatmap *m_selectedBeatmap;
	std::vector<OsuBeatmap*> m_beatmaps;
	std::vector<OsuBeatmap*> m_visibleBeatmaps;

	// bottom bar
	CBaseUIContainer *m_bottombar;
	std::vector<OsuUISelectionButton*> m_bottombarNavButtons;
};

#endif
