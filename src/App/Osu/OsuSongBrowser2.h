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
class OsuDatabaseBeatmap;

class OsuUIContextMenu;
class OsuUISearchOverlay;
class OsuUISelectionButton;
class OsuUISongBrowserInfoLabel;
class OsuUISongBrowserUserButton;
class OsuUISongBrowserScoreButton;
class OsuUISongBrowserButton;
class OsuUISongBrowserSongButton;
class OsuUISongBrowserSongDifficultyButton;
class OsuUISongBrowserCollectionButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;

class McFont;
class ConVar;

class OsuSongBrowser2 : public OsuScreenBackable
{
public:
	static void drawSelectedBeatmapBackgroundImage(Graphics *g, Osu *osu, float alpha = 1.0f);

	struct SORTING_COMPARATOR
	{
		virtual ~SORTING_COMPARATOR() {;}
		virtual bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const = 0;
	};

public:
	OsuSongBrowser2(Osu *osu);
	virtual ~OsuSongBrowser2();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void setVisible(bool visible);

	void onPlayEnd(bool quit = true); // called when a beatmap is finished playing (or the player quit)

	void onSelectionChange(OsuUISongBrowserButton *button, bool rebuild);
	void onDifficultySelected(OsuDatabaseBeatmap *diff2, bool play = false, bool mp = false);
	void onDifficultySelectedMP(OsuDatabaseBeatmap *diff2, bool play = false);
	void selectBeatmapMP(OsuDatabaseBeatmap *diff2);

	void onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, UString text);

	void highlightScore(uint64_t unixTimestamp);
	void playNextRandomBeatmap() {selectRandomBeatmap();playSelectedDifficulty();}

	void refreshBeatmaps();
	void addBeatmap(OsuDatabaseBeatmap *beatmap);

	void scrollToSongButton(OsuUISongBrowserButton *songButton, bool alignOnTop = false);
	void scrollToSelectedSongButton();
	void rebuildSongButtons();
	void rebuildScoreButtons();
	void updateSongButtonLayout();
	void updateSongButtonSorting();

	OsuUISongBrowserButton *findCurrentlySelectedSongButton() const;

	inline bool hasSelectedAndIsPlaying() const {return m_bHasSelectedAndIsPlaying;}
	inline OsuDatabase *getDatabase() const {return m_db;}
	inline OsuBeatmap *getSelectedBeatmap() const {return m_selectedBeatmap;}

	inline OsuUISongBrowserInfoLabel *getInfoLabel() {return m_songInfo;}

private:
	static bool searchMatcher(OsuDatabaseBeatmap *databaseBeatmap, UString searchString);
	static bool findSubstringInDifficulty(OsuDatabaseBeatmap *diff, const UString &searchString);

	enum class GROUP
	{
		GROUP_NO_GROUPING,
		GROUP_ARTIST,
		GROUP_BPM,
		GROUP_CREATOR,
		GROUP_DATEADDED,
		GROUP_DIFFICULTY,
		GROUP_LENGTH,
		GROUP_TITLE,
		GROUP_COLLECTIONS
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

	struct GROUPING
	{
		GROUP type;
		UString name;
	};

	virtual void updateLayout();
	virtual void onBack();

	void updateScoreBrowserLayout();

	void scheduleSearchUpdate(bool immediately = false);

	OsuUISelectionButton *addBottombarNavButton(std::function<Image*()> getImageFunc, std::function<Image*()> getImageOverFunc);
	CBaseUIButton *addTopBarRightTabButton(UString text);
	CBaseUIButton *addTopBarRightGroupButton(UString text);
	CBaseUIButton *addTopBarRightSortButton(UString text);
	CBaseUIButton *addTopBarLeftTabButton(UString text);
	CBaseUIButton *addTopBarLeftButton(UString text);

	void onDatabaseLoadingFinished();

	void onSearchUpdate();

	void onSortScoresClicked(CBaseUIButton *button);
	void onSortScoresChange(UString text, int id = -1);
	void onWebClicked(CBaseUIButton *button);

	void onGroupClicked(CBaseUIButton *button);
	void onGroupChange(UString text, int id = -1);

	void onSortClicked(CBaseUIButton *button);
	void onSortChange(UString text, int id = -1);

	void onGroupNoGrouping(CBaseUIButton *b);
	void onGroupCollections(CBaseUIButton *b);
	void onGroupArtist(CBaseUIButton *b);
	void onGroupDifficulty(CBaseUIButton *b);
	void onGroupBPM(CBaseUIButton *b);
	void onGroupCreator(CBaseUIButton *b);
	void onGroupDateadded(CBaseUIButton *b);
	void onGroupLength(CBaseUIButton *b);
	void onGroupTitle(CBaseUIButton *b);

	void onAfterSortingOrGroupChange(CBaseUIButton *groupingButton);

	void onSelectionMode();
	void onSelectionMods();
	void onSelectionRandom();
	void onSelectionOptions();

	void onModeChange(UString text);
	void onModeChange2(UString text, int id = -1);

	void onUserButtonClicked();
	void onUserButtonChange(UString text, int id);

	void onScoreClicked(CBaseUIButton *button);

	void selectSongButton(OsuUISongBrowserButton *songButton);
	void selectRandomBeatmap();
	void selectPreviousRandomBeatmap();
	void playSelectedDifficulty();

	ConVar *m_fps_max_ref;
	ConVar *m_osu_database_dynamic_star_calculation_ref;
	ConVar *m_osu_scores_enabled;
	ConVar *m_name_ref;

	ConVar *m_osu_hud_scrubbing_timeline_strains_height_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_alpha_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_r_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_g_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_b_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_r_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_g_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_b_ref;

	Osu *m_osu;
	std::mt19937 m_rngalg;
	GROUP m_group;
	std::vector<GROUPING> m_groupings;
	SORT m_sortingMethod;
	std::vector<SORTING_METHOD> m_sortingMethods;

	// top bar
	float m_fSongSelectTopScale;

	// top bar left
	CBaseUIContainer *m_topbarLeft;
	OsuUISongBrowserInfoLabel *m_songInfo;
	std::vector<CBaseUIButton*> m_topbarLeftTabButtons;
	std::vector<CBaseUIButton*> m_topbarLeftButtons;
	CBaseUIButton *m_scoreSortButton;
	CBaseUIButton *m_webButton;

	// top bar right
	CBaseUIContainer *m_topbarRight;
	std::vector<CBaseUIButton*> m_topbarRightTabButtons;
	std::vector<CBaseUIButton*> m_topbarRightGroupButtons;
	std::vector<CBaseUIButton*> m_topbarRightSortButtons;
	CBaseUILabel *m_groupLabel;
	CBaseUIButton *m_groupButton;
	CBaseUIButton *m_noGroupingButton;
	CBaseUIButton *m_collectionsButton;
	CBaseUIButton *m_artistButton;
	CBaseUIButton *m_difficultiesButton;
	CBaseUILabel *m_sortLabel;
	CBaseUIButton *m_sortButton;
	OsuUIContextMenu *m_contextMenu;

	// bottom bar
	CBaseUIContainer *m_bottombar;
	std::vector<OsuUISelectionButton*> m_bottombarNavButtons;
	OsuUISongBrowserUserButton *m_userButton;

	// score browser
	std::vector<OsuUISongBrowserScoreButton*> m_scoreButtonCache;
	CBaseUIScrollView *m_scoreBrowser;
	CBaseUIElement *m_scoreBrowserNoRecordsYetElement;

	// song browser
	CBaseUIScrollView *m_songBrowser;
	bool m_bSongBrowserRightClickScrollCheck;
	bool m_bSongBrowserRightClickScrolling;

	// song browser selection state logic
	OsuUISongBrowserSongButton *m_selectionPreviousSongButton;
	OsuUISongBrowserSongDifficultyButton *m_selectionPreviousSongDiffButton;
	OsuUISongBrowserCollectionButton *m_selectionPreviousCollectionButton;

	// beatmap database
	OsuDatabase *m_db;
	std::vector<OsuDatabaseBeatmap*> m_beatmaps;
	std::vector<OsuUISongBrowserSongButton*> m_songButtons;
	std::vector<OsuUISongBrowserButton*> m_visibleSongButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_collectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_artistCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_difficultyCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_bpmCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_creatorCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_dateaddedCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_lengthCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_titleCollectionButtons;
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
	float m_fBackgroundFadeInTime;
	std::vector<OsuDatabaseBeatmap*> m_previousRandomBeatmaps;

	// search
	OsuUISearchOverlay *m_search;
	UString m_sSearchString;
	float m_fSearchWaitTime;
	bool m_bInSearch;
	GROUP m_searchPrevGroup;

	// background star calculation
	float m_fBackgroundStarCalculationWorkNotificationTime;
	int m_iBackgroundStarCalculationIndex;
};

#endif
