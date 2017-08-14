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
class OsuDatabase;
class OsuBeatmapDifficulty;

class OsuUIContextMenu;
class OsuUISelectionButton;
class OsuUISongBrowserInfoLabel;
class OsuUISongBrowserButton;
class OsuUISongBrowserSongButton;
class OsuUISongBrowserCollectionButton;

class OsuUISongBrowserDifficultyCollectionButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;

class McFont;
class ConVar;

class OsuSongBrowser2 : public OsuScreenBackable, public MouseListener
{
public:
	static void drawSelectedBeatmapBackgroundImage(Graphics *g, Osu *osu);

	struct SORTING_COMPARATOR
	{
		virtual bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const = 0;
	};

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

	void onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool play = false);

	void playNextRandomBeatmap() {selectRandomBeatmap();playSelectedDifficulty();}

	void refreshBeatmaps();
	void scrollToSongButton(OsuUISongBrowserButton *songButton, bool alignOnTop = false);
	void scrollToSelectedSongButton();
	void rebuildSongButtons(bool unloadAllThumbnails = true);
	void updateSongButtonLayout();

	OsuUISongBrowserButton* findCurrentlySelectedSongButton() const;

	void setVisible(bool visible);

	inline bool hasSelectedAndIsPlaying() {return m_bHasSelectedAndIsPlaying;}
	inline OsuBeatmap *getSelectedBeatmap() const {return m_selectedBeatmap;}

private:
	static bool searchMatcher(OsuBeatmap *beatmap, UString searchString);
	static bool findSubstringInDifficulty(OsuBeatmapDifficulty *diff, UString searchString);

	enum class GROUP
	{
		GROUP_NO_GROUPING,
		GROUP_COLLECTIONS
		// incomplete
	};

	enum class SORT
	{
		SORT_ARTIST,
		SORT_BPM,
		SORT_CREATOR,
		SORT_DATEADDED,
		SORT_DIFFICULTY,
		SORT_LENGTH,
		SORT_RANKACHIEVED,
		SORT_TITLE
	};

	struct SORTING_METHOD
	{
		SORT type;
		UString name;
		SORTING_COMPARATOR *comparator;
	};

	virtual void updateLayout();
	virtual void onBack();

	void scheduleSearchUpdate(bool immediately = false);

	OsuUISelectionButton *addBottombarNavButton();
	CBaseUIButton *addTopBarRightTabButton(UString text);
	CBaseUIButton *addTopBarRightSortButton(UString text);

	void onDatabaseLoadingFinished();

	void onSortClicked(CBaseUIButton *button);
	void onSortChange(UString text);

	void onGroupNoGrouping(CBaseUIButton *b);
	void onGroupCollections(CBaseUIButton *b);

	void onAfterSortingOrGroupChange(CBaseUIButton *b);

	void onSelectionMode();
	void onSelectionMods();
	void onSelectionRandom();
	void onSelectionOptions();

	void onModeChange(UString text);

	void selectSongButton(OsuUISongBrowserButton *songButton);
	void selectRandomBeatmap();
	void selectPreviousRandomBeatmap();
	void playSelectedDifficulty();

	ConVar *m_fps_max_ref;

	Osu *m_osu;
	std::mt19937 m_rngalg;
	GROUP m_group;
	SORT m_sortingMethod;
	std::vector<SORTING_METHOD> m_sortingMethods;

	// top bar
	float m_fSongSelectTopScale;

	// top bar left
	CBaseUIContainer *m_topbarLeft;
	OsuUISongBrowserInfoLabel *m_songInfo;

	// top bar right
	CBaseUIContainer *m_topbarRight;
	std::vector<CBaseUIButton*> m_topbarRightTabButtons;
	std::vector<CBaseUIButton*> m_topbarRightSortButtons;
	CBaseUILabel *m_groupLabel;
	CBaseUIButton *m_noGroupingButton;
	CBaseUILabel *m_sortLabel;
	CBaseUIButton *m_sortButton;
	OsuUIContextMenu *m_contextMenu;

	// bottom bar
	CBaseUIContainer *m_bottombar;
	std::vector<OsuUISelectionButton*> m_bottombarNavButtons;

	// song browser
	CBaseUIScrollView *m_songBrowser;

	// beatmap database
	OsuDatabase *m_db;
	std::vector<OsuBeatmap*> m_beatmaps;
	std::vector<OsuUISongBrowserButton*> m_visibleSongButtons;
	std::vector<OsuUISongBrowserSongButton*> m_songButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_collectionButtons;
	std::vector<OsuUISongBrowserDifficultyCollectionButton*> m_difficultyCollectionButtons;
	bool m_bBeatmapRefreshScheduled;
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
	UString m_sSearchString;
	float m_fSearchWaitTime;
	bool m_bInSearch;
};

#endif
