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

class OsuDatabaseBeatmapStarCalculator;

class OsuSkinImage;

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
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const = 0;
	};

	struct SortByArtist : public SORTING_COMPARATOR
	{
		virtual ~SortByArtist() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByBPM : public SORTING_COMPARATOR
	{
		virtual ~SortByBPM() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByCreator : public SORTING_COMPARATOR
	{
		virtual ~SortByCreator() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByDateAdded : public SORTING_COMPARATOR
	{
		virtual ~SortByDateAdded() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByDifficulty : public SORTING_COMPARATOR
	{
		virtual ~SortByDifficulty() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByLength : public SORTING_COMPARATOR
	{
		virtual ~SortByLength() {;}
		bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByTitle : public SORTING_COMPARATOR
	{
		virtual ~SortByTitle() {;}
		bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

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

	void onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, int id);
	void onSongButtonContextMenu(OsuUISongBrowserSongButton *songButton, UString text, int id);
	void onCollectionButtonContextMenu(OsuUISongBrowserCollectionButton *collectionButton, UString text, int id);

	void highlightScore(uint64_t unixTimestamp);
	void playNextRandomBeatmap() {selectRandomBeatmap();playSelectedDifficulty();}
	void recalculateStarsForSelectedBeatmap(bool force = false);

	void refreshBeatmaps();
	void addBeatmap(OsuDatabaseBeatmap *beatmap);
	void readdBeatmap(OsuDatabaseBeatmap *diff2);

	void scrollToSongButton(OsuUISongBrowserButton *songButton, bool alignOnTop = false);
	void scrollToSelectedSongButton();
	void rebuildSongButtons();
	void rebuildScoreButtons();
	void updateSongButtonLayout();
	void updateSongButtonSorting();

	OsuUISongBrowserButton *findCurrentlySelectedSongButton() const;

	inline bool hasSelectedAndIsPlaying() const {return m_bHasSelectedAndIsPlaying;}
	inline bool isInSearch() const {return m_bInSearch;}

	inline OsuDatabase *getDatabase() const {return m_db;}
	inline OsuBeatmap *getSelectedBeatmap() const {return m_selectedBeatmap;}
	inline const OsuDatabaseBeatmapStarCalculator *getDynamicStarCalculator() const {return m_dynamicStarCalculator;}

	inline OsuUISongBrowserInfoLabel *getInfoLabel() {return m_songInfo;}

	inline GROUP getGroupingMode() const {return m_group;}

private:
	static bool searchMatcher(const OsuDatabaseBeatmap *databaseBeatmap, const UString &searchString);
	static bool findSubstringInDifficulty(const OsuDatabaseBeatmap *diff, const UString &searchString);

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
		int id;
	};

	virtual void updateLayout();
	virtual void onBack();

	void updateScoreBrowserLayout();

	void scheduleSearchUpdate(bool immediately = false);

	bool checkHandleKillDynamicStarCalculator(bool timeout);

	OsuUISelectionButton *addBottombarNavButton(std::function<OsuSkinImage*()> getImageFunc, std::function<OsuSkinImage*()> getImageOverFunc);
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

	void onGroupTabButtonClicked(CBaseUIButton *groupTabButton);
	void onGroupNoGrouping();
	void onGroupCollections(bool autoScroll = true);
	void onGroupArtist();
	void onGroupDifficulty();
	void onGroupBPM();
	void onGroupCreator();
	void onGroupDateadded();
	void onGroupLength();
	void onGroupTitle();

	void onAfterSortingOrGroupChange(bool autoScroll = true);

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
	ConVar *m_osu_scores_enabled;
	ConVar *m_name_ref;

	ConVar *m_osu_draw_scrubbing_timeline_strain_graph_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_height_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_alpha_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_r_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_g_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_b_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_r_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_g_ref;
	ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_b_ref;

	ConVar *m_osu_draw_statistics_perfectpp_ref;
	ConVar *m_osu_draw_statistics_totalstars_ref;

	ConVar *m_osu_mod_fposu_ref;

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
	bool m_bF3Pressed;
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
	UString m_sPrevSearchString;
	float m_fSearchWaitTime;
	bool m_bInSearch;
	GROUP m_searchPrevGroup;

	// background star calculation (entire database)
	float m_fBackgroundStarCalculationWorkNotificationTime;
	int m_iBackgroundStarCalculationIndex;
	OsuDatabaseBeatmapStarCalculator *m_backgroundStarCalculator;
	OsuDatabaseBeatmap *m_backgroundStarCalcTempParent;

	// background star calculation (currently selected beatmap)
	bool m_bBackgroundStarCalcScheduled;
	bool m_bBackgroundStarCalcScheduledForce;
	OsuDatabaseBeatmapStarCalculator *m_dynamicStarCalculator;
};

#endif
