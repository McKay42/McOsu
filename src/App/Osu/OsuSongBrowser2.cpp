//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap browser and selector
//
// $NoKeywords: $osusb
//===============================================================================//

#include "OsuDatabase.h"
#include "OsuSongBrowser2.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timer.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"

#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuMultiplayer.h"
#include "OsuHUD.h"
#include "OsuIcons.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuNotificationOverlay.h"
#include "OsuRankingScreen.h"
#include "OsuModSelector.h"
#include "OsuOptionsMenu.h"
#include "OsuKeyBindings.h"
#include "OsuRichPresence.h"

#include "OsuDatabaseBeatmap.h"

#include "OsuBeatmapExample.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"

#include "OsuUIBackButton.h"
#include "OsuUIContextMenu.h"
#include "OsuUISearchOverlay.h"
#include "OsuUISelectionButton.h"
#include "OsuUISongBrowserUserButton.h"
#include "OsuUISongBrowserInfoLabel.h"
#include "OsuUISongBrowserSongButton.h"
#include "OsuUISongBrowserSongDifficultyButton.h"
#include "OsuUISongBrowserCollectionButton.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUIUserStatsScreenLabel.h"



ConVar osu_gamemode("osu_gamemode", "std", FCVAR_NONE);

ConVar osu_songbrowser_sortingtype("osu_songbrowser_sortingtype", "By Date Added", FCVAR_NONE);
ConVar osu_songbrowser_scores_sortingtype("osu_songbrowser_scores_sortingtype", "Sort By Score", FCVAR_NONE);

ConVar osu_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f, FCVAR_NONE);
ConVar osu_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f, FCVAR_NONE);
ConVar osu_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f, FCVAR_NONE);
ConVar osu_songbrowser_topbar_right_height_percent("osu_songbrowser_topbar_right_height_percent", 0.5f, FCVAR_NONE);
ConVar osu_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.378f, FCVAR_NONE);
ConVar osu_songbrowser_bottombar_percent("osu_songbrowser_bottombar_percent", 0.116f, FCVAR_NONE);

ConVar osu_draw_songbrowser_background_image("osu_draw_songbrowser_background_image", true, FCVAR_NONE);
ConVar osu_draw_songbrowser_menu_background_image("osu_draw_songbrowser_menu_background_image", true, FCVAR_NONE);
ConVar osu_draw_songbrowser_strain_graph("osu_draw_songbrowser_strain_graph", false, FCVAR_NONE);
ConVar osu_songbrowser_scorebrowser_enabled("osu_songbrowser_scorebrowser_enabled", true, FCVAR_NONE);
ConVar osu_songbrowser_background_fade_in_duration("osu_songbrowser_background_fade_in_duration", 0.1f, FCVAR_NONE);

ConVar osu_songbrowser_search_delay("osu_songbrowser_search_delay", 0.5f, FCVAR_NONE, "delay until search update when entering text");
void _osu_songbrowser_search_hardcoded_filter(UString oldValue, UString newValue);
ConVar osu_songbrowser_search_hardcoded_filter("osu_songbrowser_search_hardcoded_filter", "", FCVAR_NONE, "allows forcing the specified search filter to be active all the time", _osu_songbrowser_search_hardcoded_filter);
ConVar osu_songbrowser_background_star_calculation("osu_songbrowser_background_star_calculation", true, FCVAR_NONE, "precalculate stars for all loaded beatmaps while in songbrowser");
ConVar osu_songbrowser_dynamic_star_recalc("osu_songbrowser_dynamic_star_recalc", true, FCVAR_NONE, "dynamically recalculate displayed star value of currently selected beatmap in songbrowser");

ConVar osu_debug_background_star_calc("osu_debug_background_star_calc", false, FCVAR_NONE, "prints the name of the beatmap about to get its stars calculated (cmd/terminal window only, no in-game log!)");

void _osu_songbrowser_search_hardcoded_filter(UString oldValue, UString newValue)
{
	if (newValue.length() == 1 && newValue.isWhitespaceOnly())
		osu_songbrowser_search_hardcoded_filter.setValue("");
}



class OsuSongBrowserBackgroundSearchMatcher : public Resource
{
public:
	OsuSongBrowserBackgroundSearchMatcher() : Resource()
	{
		m_bDead = true; // NOTE: start dead! need to revive() before use
	}

	bool isDead() const {return m_bDead.load();}
	void kill() {m_bDead = true;}
	void revive() {m_bDead = false;}

	void setSongButtonsAndSearchString(const std::vector<OsuUISongBrowserSongButton*> &songButtons, const UString &searchString, const UString &hardcodedSearchString)
	{
		m_songButtons = songButtons;

		m_sSearchString.clear();
		if (hardcodedSearchString.length() > 0)
		{
			m_sSearchString.append(hardcodedSearchString);
			m_sSearchString.append(" ");
		}
		m_sSearchString.append(searchString);
		m_sHardcodedSearchString = hardcodedSearchString;
	}

protected:
	virtual void init()
	{
		m_bReady = true;
	}

	virtual void initAsync()
	{
		if (m_bDead.load())
		{
			m_bAsyncReady = true;
			return;
		}

		// flag matches across entire database
		const std::vector<UString> searchStringTokens = m_sSearchString.split(" ");
		for (size_t i=0; i<m_songButtons.size(); i++)
		{
			const std::vector<OsuUISongBrowserButton*> &children = m_songButtons[i]->getChildren();
			if (children.size() > 0)
			{
				for (size_t c=0; c<children.size(); c++)
				{
					children[c]->setIsSearchMatch(OsuSongBrowser2::searchMatcher(children[c]->getDatabaseBeatmap(), searchStringTokens));
				}
			}
			else
				m_songButtons[i]->setIsSearchMatch(OsuSongBrowser2::searchMatcher(m_songButtons[i]->getDatabaseBeatmap(), searchStringTokens));

			// cancellation point
			if (m_bDead.load())
				break;
		}

		m_bAsyncReady = true;
	}

	virtual void destroy() {;}

private:
	std::atomic<bool> m_bDead;

	UString m_sSearchString;
	UString m_sHardcodedSearchString;
	std::vector<OsuUISongBrowserSongButton*> m_songButtons;
};



class OsuUISongBrowserNoRecordsSetElement : public CBaseUILabel
{
public:
	OsuUISongBrowserNoRecordsSetElement(Osu *osu, UString text) : CBaseUILabel(0, 0, 0, 0, "", text)
	{
		m_osu = osu;
		m_sIconString.insert(0, OsuIcons::TROPHY);
	}

	virtual void drawText(Graphics *g)
	{
		// draw icon
		const float iconScale = 0.6f;
		McFont *iconFont = m_osu->getFontIcons();
		int iconWidth = 0;
		g->pushTransform();
		{
			const float scale = (m_vSize.y / iconFont->getHeight())*iconScale;
			const float paddingLeft = scale*15;

			iconWidth = paddingLeft + iconFont->getStringWidth(m_sIconString)*scale;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x + paddingLeft), (int)(m_vPos.y + m_vSize.y/2 + iconFont->getHeight()*scale/2));
			g->setColor(0xffffffff);
			g->drawString(iconFont, m_sIconString);
		}
		g->popTransform();

		// draw text
		const float textScale = 0.6f;
		McFont *textFont = m_osu->getSongBrowserFont();
		g->pushTransform();
		{
			const float stringWidth = textFont->getStringWidth(m_sText);

			const float scale = ((m_vSize.x - iconWidth) / stringWidth)*textScale;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x + iconWidth + (m_vSize.x - iconWidth)/2 - stringWidth*scale/2), (int)(m_vPos.y + m_vSize.y/2 + textFont->getHeight()*scale/2));
			g->setColor(0xff02c3e5);
			g->drawString(textFont, m_sText);
		}
		g->popTransform();
	}

private:
	Osu *m_osu;
	UString m_sIconString;
};



bool OsuSongBrowser2::SortByArtist::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	if (a->getDatabaseBeatmap()->getArtist().equalsIgnoreCase(b->getDatabaseBeatmap()->getArtist()))
	{
		// strict weak ordering!
		if (a->getDatabaseBeatmap()->getTitle().equalsIgnoreCase(b->getDatabaseBeatmap()->getTitle()))
			return a->getSortHack() < b->getSortHack();
		else
			return a->getDatabaseBeatmap()->getTitle().lessThanIgnoreCase(b->getDatabaseBeatmap()->getTitle());
	}

	return a->getDatabaseBeatmap()->getArtist().lessThanIgnoreCase(b->getDatabaseBeatmap()->getArtist());
}

bool OsuSongBrowser2::SortByBPM::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	int bpm1 = a->getDatabaseBeatmap()->getMostCommonBPM();
	const std::vector<OsuDatabaseBeatmap*> &aDiffs = a->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<aDiffs.size(); i++)
	{
		if (aDiffs[i]->getMostCommonBPM() > bpm1)
			bpm1 = aDiffs[i]->getMostCommonBPM();
	}

	int bpm2 = b->getDatabaseBeatmap()->getMostCommonBPM();
	const std::vector<OsuDatabaseBeatmap*> &bDiffs = b->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<bDiffs.size(); i++)
	{
		if (bDiffs[i]->getMostCommonBPM() > bpm2)
			bpm2 = bDiffs[i]->getMostCommonBPM();
	}

	// strict weak ordering!
	if (bpm1 == bpm2)
		return a->getSortHack() < b->getSortHack();

	return bpm1 < bpm2;
}

bool OsuSongBrowser2::SortByCreator::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	// strict weak ordering!
	if (a->getDatabaseBeatmap()->getCreator().equalsIgnoreCase(b->getDatabaseBeatmap()->getCreator()))
		return a->getSortHack() < b->getSortHack();

	return a->getDatabaseBeatmap()->getCreator().lessThanIgnoreCase(b->getDatabaseBeatmap()->getCreator());
}

bool OsuSongBrowser2::SortByDateAdded::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	long long time1 = a->getDatabaseBeatmap()->getLastModificationTime();
	const std::vector<OsuDatabaseBeatmap*> &aDiffs = a->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<aDiffs.size(); i++)
	{
		if (aDiffs[i]->getLastModificationTime() > time1)
			time1 = aDiffs[i]->getLastModificationTime();
	}

	long long time2 = b->getDatabaseBeatmap()->getLastModificationTime();
	const std::vector<OsuDatabaseBeatmap*> &bDiffs = b->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<bDiffs.size(); i++)
	{
		if (bDiffs[i]->getLastModificationTime() > time2)
			time2 = bDiffs[i]->getLastModificationTime();
	}

	// strict weak ordering!
	if (time1 == time2)
		return a->getSortHack() > b->getSortHack();

	return time1 > time2;
}

bool OsuSongBrowser2::SortByDifficulty::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	float diff1 = (a->getDatabaseBeatmap()->getAR()+1)*(a->getDatabaseBeatmap()->getCS()+1)*(a->getDatabaseBeatmap()->getHP()+1)*(a->getDatabaseBeatmap()->getOD()+1)*(std::max(a->getDatabaseBeatmap()->getMostCommonBPM(), 1));
	float stars1 = a->getDatabaseBeatmap()->getStarsNomod();
	const std::vector<OsuDatabaseBeatmap*> &aDiffs = a->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<aDiffs.size(); i++)
	{
		const OsuDatabaseBeatmap *d = aDiffs[i];
		if (d->getStarsNomod() > stars1)
			stars1 = d->getStarsNomod();

		const float tempDiff1 = (d->getAR()+1)*(d->getCS()+1)*(d->getHP()+1)*(d->getOD()+1)*(std::max(d->getMostCommonBPM(), 1));
		if (tempDiff1 > diff1)
			diff1 = tempDiff1;
	}

	float diff2 = (b->getDatabaseBeatmap()->getAR()+1)*(b->getDatabaseBeatmap()->getCS()+1)*(b->getDatabaseBeatmap()->getHP()+1)*(b->getDatabaseBeatmap()->getOD()+1)*(std::max(b->getDatabaseBeatmap()->getMostCommonBPM(), 1));
	float stars2 = b->getDatabaseBeatmap()->getStarsNomod();
	const std::vector<OsuDatabaseBeatmap*> &bDiffs = b->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<bDiffs.size(); i++)
	{
		const OsuDatabaseBeatmap *d = bDiffs[i];
		if (d->getStarsNomod() > stars2)
			stars2 = d->getStarsNomod();

		const float tempDiff2 = (d->getAR()+1)*(d->getCS()+1)*(d->getHP()+1)*(d->getOD()+1)*(std::max(d->getMostCommonBPM(), 1));
		if (tempDiff2 > diff1)
			diff2 = tempDiff2;
	}

	if (stars1 > 0 && stars2 > 0)
	{
		// strict weak ordering!
		if (stars1 == stars2)
			return a->getSortHack() < b->getSortHack();

		return stars1 < stars2;
	}
	else
	{
		// strict weak ordering!
		if (diff1 == diff2)
			return a->getSortHack() < b->getSortHack();

		return diff1 < diff2;
	}
}

bool OsuSongBrowser2::SortByLength::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	unsigned long length1 = a->getDatabaseBeatmap()->getLengthMS();
	const std::vector<OsuDatabaseBeatmap*> &aDiffs = a->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<aDiffs.size(); i++)
	{
		if (aDiffs[i]->getLengthMS() > length1)
			length1 = aDiffs[i]->getLengthMS();
	}

	unsigned long length2 = b->getDatabaseBeatmap()->getLengthMS();
	const std::vector<OsuDatabaseBeatmap*> &bDiffs = b->getDatabaseBeatmap()->getDifficulties();
	for (size_t i=0; i<bDiffs.size(); i++)
	{
		if (bDiffs[i]->getLengthMS() > length2)
			length2 = bDiffs[i]->getLengthMS();
	}

	// strict weak ordering!
	if (length1 == length2)
		return a->getSortHack() < b->getSortHack();

	return length1 < length2;
}

bool OsuSongBrowser2::SortByTitle::operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
{
	if (a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL)
		return a->getSortHack() < b->getSortHack();

	// strict weak ordering!
	if (a->getDatabaseBeatmap()->getTitle().equalsIgnoreCase(b->getDatabaseBeatmap()->getTitle()))
		return a->getSortHack() < b->getSortHack();

	return a->getDatabaseBeatmap()->getTitle().lessThanIgnoreCase(b->getDatabaseBeatmap()->getTitle());
}



OsuSongBrowser2::OsuSongBrowser2(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// random selection algorithm init
	m_rngalg = std::mt19937(time(0));

	// sorting/grouping + methods
	m_group = GROUP::GROUP_NO_GROUPING;
	{
		m_groupings.push_back({GROUP::GROUP_NO_GROUPING, "No Grouping", 0});
		m_groupings.push_back({GROUP::GROUP_ARTIST, "By Artist", 1});
		///m_groupings.push_back({GROUP::GROUP_BPM, "By BPM", 2}); // not yet possible
		m_groupings.push_back({GROUP::GROUP_CREATOR, "By Creator", 3});
		///m_groupings.push_back({GROUP::GROUP_DATEADDED, "By Date Added", 4}); // not yet possible
		m_groupings.push_back({GROUP::GROUP_DIFFICULTY, "By Difficulty", 5});
		m_groupings.push_back({GROUP::GROUP_LENGTH, "By Length", 6});
		m_groupings.push_back({GROUP::GROUP_TITLE, "By Title", 7});
		m_groupings.push_back({GROUP::GROUP_COLLECTIONS, "Collections", 8});
	}

	m_sortingMethod = SORT::SORT_ARTIST;
	{
		m_sortingMethods.push_back({SORT::SORT_ARTIST, "By Artist", new SortByArtist()});
		m_sortingMethods.push_back({SORT::SORT_BPM, "By BPM", new SortByBPM()});
		m_sortingMethods.push_back({SORT::SORT_CREATOR, "By Creator", new SortByCreator()});
		m_sortingMethods.push_back({SORT::SORT_DATEADDED, "By Date Added", new SortByDateAdded()});
		m_sortingMethods.push_back({SORT::SORT_DIFFICULTY, "By Difficulty", new SortByDifficulty()});
		m_sortingMethods.push_back({SORT::SORT_LENGTH, "By Length", new SortByLength()});
		///m_sortingMethods.push_back({SORT::SORT_RANKACHIEVED, "By Rank Achieved", new SortByRankAchieved()}); // not yet possible
		m_sortingMethods.push_back({SORT::SORT_TITLE, "By Title", new SortByTitle()});
	}

	// convar refs
	m_fps_max_ref = convar->getConVarByName("fps_max");
	m_osu_scores_enabled = convar->getConVarByName("osu_scores_enabled");
	m_name_ref = convar->getConVarByName("name");

	m_osu_draw_scrubbing_timeline_strain_graph_ref = convar->getConVarByName("osu_draw_scrubbing_timeline_strain_graph");
	m_osu_hud_scrubbing_timeline_strains_height_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_height");
	m_osu_hud_scrubbing_timeline_strains_alpha_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_alpha");
	m_osu_hud_scrubbing_timeline_strains_aim_color_r_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_aim_color_r");
	m_osu_hud_scrubbing_timeline_strains_aim_color_g_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_aim_color_g");
	m_osu_hud_scrubbing_timeline_strains_aim_color_b_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_aim_color_b");
	m_osu_hud_scrubbing_timeline_strains_speed_color_r_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_speed_color_r");
	m_osu_hud_scrubbing_timeline_strains_speed_color_g_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_speed_color_g");
	m_osu_hud_scrubbing_timeline_strains_speed_color_b_ref = convar->getConVarByName("osu_hud_scrubbing_timeline_strains_speed_color_b");

	m_osu_draw_statistics_perfectpp_ref = convar->getConVarByName("osu_draw_statistics_perfectpp");
	m_osu_draw_statistics_totalstars_ref = convar->getConVarByName("osu_draw_statistics_totalstars");

	m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");

	// convar callbacks
	osu_gamemode.setCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onModeChange) );

	// vars
	m_bSongBrowserRightClickScrollCheck = false;
	m_bSongBrowserRightClickScrolling = false;
	m_bNextScrollToSongButtonJumpFixScheduled = false;
	m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;
	m_fNextScrollToSongButtonJumpFixOldRelPosY = 0.0f;

	m_selectionPreviousSongButton = NULL;
	m_selectionPreviousSongDiffButton = NULL;
	m_selectionPreviousCollectionButton = NULL;

	m_bF1Pressed = false;
	m_bF2Pressed = false;
	m_bF3Pressed = false;
	m_bShiftPressed = false;
	m_bLeft = false;
	m_bRight = false;

	m_bRandomBeatmapScheduled = false;
	m_bPreviousRandomBeatmapScheduled = false;

	m_fSongSelectTopScale = 1.0f;

	// build topbar left
	m_topbarLeft = new CBaseUIContainer(0, 0, 0, 0, "");
	{
		m_songInfo = new OsuUISongBrowserInfoLabel(m_osu, 0, 0, 0, 0, "");

		m_topbarLeft->addBaseUIElement(m_songInfo);
	}

	m_scoreSortButton = addTopBarLeftTabButton("Sort By Score");
	m_scoreSortButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortScoresClicked) );
	m_webButton = addTopBarLeftButton("Web");
	m_webButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onWebClicked) );

	// build topbar right
	m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");

	addTopBarRightGroupButton("")->setVisible(false); // NOTE: align with second tab
	{
		m_groupLabel = new CBaseUILabel(0, 0, 0, 0, "", "Group:");
		m_groupLabel->setSizeToContent(3);
		m_groupLabel->setDrawFrame(false);
		m_groupLabel->setDrawBackground(false);

		m_topbarRight->addBaseUIElement(m_groupLabel);
	}
	m_groupButton = addTopBarRightGroupButton("No Grouping");
	m_groupButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupClicked) );

	{
		// "hardcoded" grouping tabs
		m_collectionsButton = addTopBarRightTabButton("Collections");
		m_collectionsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupTabButtonClicked) );
		m_artistButton = addTopBarRightTabButton("By Artist");
		m_artistButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupTabButtonClicked) );
		m_difficultiesButton = addTopBarRightTabButton("By Difficulty");
		m_difficultiesButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupTabButtonClicked) );
		m_noGroupingButton = addTopBarRightTabButton("No Grouping");
		m_noGroupingButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupTabButtonClicked) );
		m_noGroupingButton->setTextBrightColor(COLOR(255, 0, 255, 0));
	}

	addTopBarRightSortButton("")->setVisible(false); // NOTE: align with last tab (1)
	addTopBarRightSortButton("")->setVisible(false); // NOTE: align with last tab (2)
	addTopBarRightSortButton("")->setVisible(false); // NOTE: align with last tab (3)
	{
		m_sortLabel = new CBaseUILabel(0, 0, 0, 0, "", "Sort:");
		m_sortLabel->setSizeToContent(3);
		m_sortLabel->setDrawFrame(false);
		m_sortLabel->setDrawBackground(false);

		m_topbarRight->addBaseUIElement(m_sortLabel);
	}
	m_sortButton = addTopBarRightSortButton("By Date Added");
	m_sortButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortClicked) );

	// context menu
	m_contextMenu = new OsuUIContextMenu(m_osu, 50, 50, 150, 0, "");
	m_contextMenu->setVisible(true);

	// build bottombar
	m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

	addBottombarNavButton([this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionMode();}, [this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionModeOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMode) );
	addBottombarNavButton([this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionMods();}, [this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionModsOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMods) );
	addBottombarNavButton([this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionRandom();}, [this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionRandomOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionRandom) );
	addBottombarNavButton([this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionOptions();}, [this]() -> OsuSkinImage *{return m_osu->getSkin()->getSelectionOptionsOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionOptions) );

	m_userButton = new OsuUISongBrowserUserButton(m_osu);
	m_userButton->addTooltipLine("Click to change [User] or view/edit [Top Ranks]");
	m_userButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onUserButtonClicked) );
	m_userButton->setText(m_name_ref->getString());
	m_bottombar->addBaseUIElement(m_userButton);

	// looks like shit there, don't like it
	m_ppVersionInfoLabel = NULL;
	/*
	m_ppVersionInfoLabel = new OsuUIUserStatsScreenLabel(m_osu);
	m_ppVersionInfoLabel->setText(UString::format("pp Version: %i", OsuDifficultyCalculator::PP_ALGORITHM_VERSION));
	m_ppVersionInfoLabel->setTooltipText("WARNING: McOsu's star/pp algorithm is currently lagging behind the \"official\" version.\n \nReason being that keeping up-to-date requires a LOT of changes now.\nThe next goal is rewriting the algorithm architecture to be more similar to osu!lazer,\nas that will make porting star/pp changes infinitely easier for the foreseeable future.\n \nNo promises as to when all of that will be finished.");
	m_ppVersionInfoLabel->setTextColor(0xbbbb0000);
	m_ppVersionInfoLabel->setDrawBackground(false);
	m_ppVersionInfoLabel->setDrawFrame(false);
	m_bottombar->addBaseUIElement(m_ppVersionInfoLabel);
	*/

	// build scorebrowser
	m_scoreBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_scoreBrowser->setDrawBackground(false);
	m_scoreBrowser->setDrawFrame(false);
	m_scoreBrowser->setClipping(false);
	m_scoreBrowser->setHorizontalScrolling(false);
	m_scoreBrowser->setScrollbarSizeMultiplier(0.25f);
	m_scoreBrowser->setScrollResistance((m_osu->isInVRMode() || env->getOS() == Environment::OS::OS_HORIZON) ? convar->getConVarByName("ui_scrollview_resistance")->getInt() : 15); // a bit shitty this check + convar, but works well enough
	m_scoreBrowserNoRecordsYetElement = new OsuUISongBrowserNoRecordsSetElement(m_osu, "No records set!");
	m_scoreBrowser->getContainer()->addBaseUIElement(m_scoreBrowserNoRecordsYetElement);

	// build songbrowser
	m_songBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_songBrowser->setDrawBackground(false);
	m_songBrowser->setDrawFrame(false);
	m_songBrowser->setHorizontalScrolling(false);
	m_songBrowser->setScrollResistance((m_osu->isInVRMode() || env->getOS() == Environment::OS::OS_HORIZON) ? convar->getConVarByName("ui_scrollview_resistance")->getInt() : 15); // a bit shitty this check + convar, but works well enough

	// beatmap database
	m_db = new OsuDatabase(m_osu);
	m_bBeatmapRefreshScheduled = true;

	// behaviour
	m_bHasSelectedAndIsPlaying = false;
	m_selectedBeatmap = NULL;
	m_fPulseAnimation = 0.0f;
	m_fBackgroundFadeInTime = 0.0f;

	// search
	m_search = new OsuUISearchOverlay(m_osu, 0, 0, 0, 0, "");
	m_search->setOffsetRight(10);
	m_fSearchWaitTime = 0.0f;
	m_bInSearch = (osu_songbrowser_search_hardcoded_filter.getString().length() > 0);
	m_searchPrevGroup = GROUP::GROUP_NO_GROUPING;
	m_backgroundSearchMatcher = new OsuSongBrowserBackgroundSearchMatcher();
	m_bOnAfterSortingOrGroupChangeUpdateScheduled = false;
	m_bOnAfterSortingOrGroupChangeUpdateScheduledAutoScroll = false;

	// background star calculation (entire database)
	m_fBackgroundStarCalculationWorkNotificationTime = 0.0f;
	m_iBackgroundStarCalculationIndex = 0;
	m_backgroundStarCalculator = new OsuDatabaseBeatmapStarCalculator();
	m_backgroundStarCalcTempParent = NULL;

	// background star calculation (currently selected beatmap)
	m_bBackgroundStarCalcScheduled = false;
	m_bBackgroundStarCalcScheduledForce = false;
	m_dynamicStarCalculator = new OsuDatabaseBeatmapStarCalculator();

	updateLayout();
}

OsuSongBrowser2::~OsuSongBrowser2()
{
	checkHandleKillBackgroundStarCalculator();
	checkHandleKillDynamicStarCalculator(false);
	checkHandleKillBackgroundSearchMatcher();

	engine->getResourceManager()->destroyResource(m_backgroundStarCalculator);
	engine->getResourceManager()->destroyResource(m_dynamicStarCalculator);
	engine->getResourceManager()->destroyResource(m_backgroundSearchMatcher);

	m_songBrowser->getContainer()->empty();

	for (size_t i=0; i<m_songButtons.size(); i++)
	{
		delete m_songButtons[i];
	}
	for (size_t i=0; i<m_collectionButtons.size(); i++)
	{
		delete m_collectionButtons[i];
	}
	for (size_t i=0; i<m_artistCollectionButtons.size(); i++)
	{
		delete m_artistCollectionButtons[i];
	}
	for (size_t i=0; i<m_difficultyCollectionButtons.size(); i++)
	{
		delete m_difficultyCollectionButtons[i];
	}
	for (size_t i=0; i<m_bpmCollectionButtons.size(); i++)
	{
		delete m_bpmCollectionButtons[i];
	}
	for (size_t i=0; i<m_creatorCollectionButtons.size(); i++)
	{
		delete m_creatorCollectionButtons[i];
	}
	for (size_t i=0; i<m_dateaddedCollectionButtons.size(); i++)
	{
		delete m_dateaddedCollectionButtons[i];
	}
	for (size_t i=0; i<m_lengthCollectionButtons.size(); i++)
	{
		delete m_lengthCollectionButtons[i];
	}
	for (size_t i=0; i<m_titleCollectionButtons.size(); i++)
	{
		delete m_titleCollectionButtons[i];
	}

	m_scoreBrowser->getContainer()->empty();
	for (size_t i=0; i<m_scoreButtonCache.size(); i++)
	{
		delete m_scoreButtonCache[i];
	}
	SAFE_DELETE(m_scoreBrowserNoRecordsYetElement);

	for (size_t i=0; i<m_sortingMethods.size(); i++)
	{
		delete m_sortingMethods[i].comparator;
	}

	SAFE_DELETE(m_search);
	SAFE_DELETE(m_topbarLeft);
	SAFE_DELETE(m_topbarRight);
	SAFE_DELETE(m_bottombar);
	SAFE_DELETE(m_scoreBrowser);
	SAFE_DELETE(m_songBrowser);
	SAFE_DELETE(m_db);
}

void OsuSongBrowser2::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw background
	//g->setColor(0xffffffff);
	g->setColor(0xff000000);
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	/*
	g->setColor(0xffffffff);
	g->setAlpha(clamp<float>(engine->getMouse()->getPos().x / 400.0f, 0.0f, 1.0f));
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	*/

	// refreshing (blocks every other call in draw() below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(m_db->getProgress()*100.0f));

		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), m_osu->getScreenHeight() - 15);
			g->drawString(m_osu->getSubTitleFont(), loadingMessage);
		}
		g->popTransform();

		m_osu->getHUD()->drawBeatmapImportSpinner(g);
		return;
	}

	// draw background image
	if (osu_draw_songbrowser_background_image.getBool())
	{
		float alpha = 1.0f;
		if (osu_songbrowser_background_fade_in_duration.getFloat() > 0.0f)
		{
			// handle fadein trigger after handler is finished loading
			const bool ready = m_osu->getSelectedBeatmap() != NULL
				&& m_osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL
				&& m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_osu->getSelectedBeatmap()->getSelectedDifficulty2()) != NULL
				&& m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_osu->getSelectedBeatmap()->getSelectedDifficulty2())->isReady();

			if (!ready)
				m_fBackgroundFadeInTime = engine->getTime();
			else if (m_fBackgroundFadeInTime > 0.0f && engine->getTime() > m_fBackgroundFadeInTime)
			{
				alpha = clamp<float>((engine->getTime() - m_fBackgroundFadeInTime)/osu_songbrowser_background_fade_in_duration.getFloat(), 0.0f, 1.0f);
				alpha = 1.0f - (1.0f - alpha)*(1.0f - alpha);
			}
		}

		drawSelectedBeatmapBackgroundImage(g, m_osu, alpha);
	}
	else if (osu_draw_songbrowser_menu_background_image.getBool())
	{
		// menu-background
		Image *backgroundImage = m_osu->getSkin()->getMenuBackground();
		if (backgroundImage != NULL && backgroundImage != m_osu->getSkin()->getMissingTexture() && backgroundImage->isReady())
		{
			const float scale = Osu::getImageScaleToFillResolution(backgroundImage, m_osu->getScreenSize());

			g->setColor(0xffffffff);
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
				g->drawImage(backgroundImage);
			}
			g->popTransform();
		}
	}

	// draw score browser
	m_scoreBrowser->draw(g);

	// draw strain graph of currently selected beatmap
	if (osu_draw_songbrowser_strain_graph.getBool() && getSelectedBeatmap() != NULL && getSelectedBeatmap()->getSelectedDifficulty2() != NULL && m_dynamicStarCalculator->isAsyncReady())
	{
		// this is still WIP

		///const std::vector<double> &aimStrains = getSelectedBeatmap()->getAimStrains();
		///const std::vector<double> &speedStrains = getSelectedBeatmap()->getSpeedStrains();
		const std::vector<double> &aimStrains = m_dynamicStarCalculator->getAimStrains();
		const std::vector<double> &speedStrains = m_dynamicStarCalculator->getSpeedStrains();
		const float speedMultiplier = m_osu->getSpeedMultiplier();

		//const unsigned long lengthFullMS = beatmapLength;
		//const unsigned long lengthMS = getSelectedBeatmap()->getLengthPlayable();
		//const unsigned long startTimeMS = getSelectedBeatmap()->getStartTimePlayable();
		//const unsigned long endTimeMS = startTimeMS + lengthMS;
		//const unsigned long currentTimeMS = beatmapTime;

		if (aimStrains.size() > 0 && aimStrains.size() == speedStrains.size())
		{
			const float strainStepMS = 400.0f * speedMultiplier;

			const unsigned long lengthMS = strainStepMS * aimStrains.size();

			// get highest strain values for normalization
			double highestAimStrain = 0.0;
			double highestSpeedStrain = 0.0;
			double highestStrain = 0.0;
			int highestStrainIndex = -1;
			double averageStrain = 0.0;
			for (int i=0; i<aimStrains.size(); i++)
			{
				const double aimStrain = aimStrains[i];
				const double speedStrain = speedStrains[i];
				const double strain = aimStrain + speedStrain;

				if (strain > highestStrain)
				{
					highestStrain = strain;
					highestStrainIndex = i;
				}
				if (aimStrain > highestAimStrain)
					highestAimStrain = aimStrain;
				if (speedStrain > highestSpeedStrain)
					highestSpeedStrain = speedStrain;

				averageStrain += strain;
			}
			averageStrain /= (double)aimStrains.size();

			// draw strain bar graph
			if (highestAimStrain > 0.0 && highestSpeedStrain > 0.0 && highestStrain > 0.0)
			{
				const float dpiScale = Osu::getUIScale(m_osu);

				const float graphWidth = m_scoreBrowser->getSize().x;

				const float msPerPixel = (float)lengthMS / graphWidth;
				const float strainWidth = strainStepMS / msPerPixel;
				const float strainHeightMultiplier = m_osu_hud_scrubbing_timeline_strains_height_ref->getFloat() * dpiScale;

				McRect graphRect(0, m_bottombar->getPos().y - strainHeightMultiplier, graphWidth, strainHeightMultiplier);

				const float alpha = (graphRect.contains(engine->getMouse()->getPos()) ? 1.0f : m_osu_hud_scrubbing_timeline_strains_alpha_ref->getFloat());

				const Color aimStrainColor = COLORf(alpha, m_osu_hud_scrubbing_timeline_strains_aim_color_r_ref->getInt() / 255.0f, m_osu_hud_scrubbing_timeline_strains_aim_color_g_ref->getInt() / 255.0f, m_osu_hud_scrubbing_timeline_strains_aim_color_b_ref->getInt() / 255.0f);
				const Color speedStrainColor = COLORf(alpha, m_osu_hud_scrubbing_timeline_strains_speed_color_r_ref->getInt() / 255.0f, m_osu_hud_scrubbing_timeline_strains_speed_color_g_ref->getInt() / 255.0f, m_osu_hud_scrubbing_timeline_strains_speed_color_b_ref->getInt() / 255.0f);

				g->setDepthBuffer(true);
				for (int i=0; i<aimStrains.size(); i++)
				{
					const double aimStrain = (aimStrains[i]) / highestStrain;
					const double speedStrain = (speedStrains[i]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					if (!engine->getKeyboard()->isShiftDown())
					{
						g->setColor(aimStrainColor);
						g->fillRect(i*strainWidth, m_bottombar->getPos().y - aimStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), aimStrainHeight);
					}

					if (!engine->getKeyboard()->isControlDown())
					{
						g->setColor(speedStrainColor);
						g->fillRect(i*strainWidth, m_bottombar->getPos().y - (engine->getKeyboard()->isShiftDown() ? 0 : aimStrainHeight) - speedStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), speedStrainHeight + 1);
					}
				}
				g->setDepthBuffer(false);

				// highlight highest total strain value (+- section block)
				if (highestStrainIndex > -1)
				{
					const double aimStrain = (aimStrains[highestStrainIndex]) / highestStrain;
					const double speedStrain = (speedStrains[highestStrainIndex]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					Vector2 topLeftCenter = Vector2(highestStrainIndex*strainWidth + strainWidth/2.0f, m_bottombar->getPos().y - aimStrainHeight - speedStrainHeight);

					const float margin = 5.0f * dpiScale;

					g->setColor(0xffffffff);
					g->setAlpha(alpha);
					g->drawRect(topLeftCenter.x - margin*strainWidth, topLeftCenter.y - margin*strainWidth, strainWidth*2*margin, aimStrainHeight + speedStrainHeight + 2*margin*strainWidth);
					g->setAlpha(alpha * 0.5f);
					g->drawRect(topLeftCenter.x - margin*strainWidth - 2, topLeftCenter.y - margin*strainWidth - 2, strainWidth*2*margin + 4, aimStrainHeight + speedStrainHeight + 2*margin*strainWidth + 4);
					g->setAlpha(alpha * 0.25f);
					g->drawRect(topLeftCenter.x - margin*strainWidth - 4, topLeftCenter.y - margin*strainWidth - 4, strainWidth*2*margin + 8, aimStrainHeight + speedStrainHeight + 2*margin*strainWidth + 8);
				}

				// DEBUG:
				/*
				g->pushTransform();
				{
					g->translate(10, m_bottombar->getPos().y - 200);
					g->setColor(0xffffffff);
					g->drawString(m_osu->getSubTitleFont(), UString::format("avg strain = %i%% (%f)", (int)((averageStrain / highestStrain) * 100.0f), averageStrain));
					g->translate(0, m_osu->getSubTitleFont()->getHeight());
					g->drawString(m_osu->getSubTitleFont(), UString::format("top strain = %i%% (%f)", (int)((highestStrain / highestStrain) * 100.0f), highestStrain));
					g->translate(0, m_osu->getSubTitleFont()->getHeight());
					g->drawString(m_osu->getSubTitleFont(), UString::format("delta = %i%% (%f)", (int)(((highestStrain - averageStrain) / highestStrain) * 100.0f), (highestStrain - averageStrain)));
				}
				g->popTransform();
				*/
			}
		}
	}

	// draw song browser
	m_songBrowser->draw(g);

	// draw search
	m_search->setSearchString(m_sSearchString, osu_songbrowser_search_hardcoded_filter.getString());
	m_search->setDrawNumResults(m_bInSearch);
	m_search->setNumFoundResults(m_visibleSongButtons.size());
	m_search->setSearching(!m_backgroundSearchMatcher->isDead());
	m_search->draw(g);

	// draw top bar
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(m_fSongSelectTopScale, m_fSongSelectTopScale);
		g->translate((m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale)/2, (m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale)/2);
		g->drawImage(m_osu->getSkin()->getSongSelectTop());
	}
	g->popTransform();

	m_topbarLeft->draw(g);
	if (Osu::debug->getBool())
		m_topbarLeft->draw_debug(g);
	m_topbarRight->draw(g);
	if (Osu::debug->getBool())
		m_topbarRight->draw_debug(g);

	// draw bottom bar
	float songSelectBottomScale = m_bottombar->getSize().y / m_osu->getSkin()->getSongSelectBottom()->getHeight();
	songSelectBottomScale *= 0.8f;

	g->setColor(0xff000000);
	g->fillRect(0, m_bottombar->getPos().y + 10, m_osu->getScreenWidth(), m_bottombar->getSize().y);

	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(songSelectBottomScale, songSelectBottomScale);
		g->translate(0, (int)(m_bottombar->getPos().y) + (int)((m_osu->getSkin()->getSongSelectBottom()->getHeight()*songSelectBottomScale)/2) - 1);
		m_osu->getSkin()->getSongSelectBottom()->bind();
		{
			g->drawQuad(0, -(int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)/2), (int)(m_osu->getScreenWidth()*(1.0f/songSelectBottomScale)), (int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)));
		}
		m_osu->getSkin()->getSongSelectBottom()->unbind();
	}
	g->popTransform();

	OsuScreenBackable::draw(g);
	m_bottombar->draw(g);
	if (Osu::debug->getBool())
		m_bottombar->draw_debug(g);

	// background task busy notification
	if (m_fBackgroundStarCalculationWorkNotificationTime > engine->getTime())
	{
		UString busyMessage = "Calculating stars (";
		busyMessage.append(UString::format("%i/%i) ...", m_iBackgroundStarCalculationIndex, m_beatmaps.size()));
		McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");

		g->setColor(0xff333333);
		g->pushTransform();
		{
			g->translate((int)(m_bottombar->getPos().x + m_bottombar->getSize().x - font->getStringWidth(busyMessage) - 20), (int)(m_bottombar->getPos().y + m_bottombar->getSize().y/2 - font->getHeight()/2));
			g->drawString(font, busyMessage);
		}
		g->popTransform();
	}

	// top ranks available info
	/*
	if (true)
	{
		UString topRanksInfoMessage = "<<< Top Ranks";
		McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");
		g->setColor(0xff444444);
		g->pushTransform();
		{
			g->translate((int)(m_userButton->getPos().x + m_userButton->getSize().x + 10), (int)(m_userButton->getPos().y + m_userButton->getSize().y/2 + font->getHeight()/2));
			g->drawString(font, topRanksInfoMessage);
		}
		g->popTransform();
	}
	*/

	// no beatmaps found (osu folder is probably invalid)
	if (m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled)
	{
		UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
		errorMessage1.append(m_sLastOsuFolder);
		UString errorMessage2 = "Go to Options -> osu!folder";

		g->setColor(0xffff0000);
		g->pushTransform();
		{
			g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage1)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()));
			g->drawString(m_osu->getSubTitleFont(), errorMessage1);
		}
		g->popTransform();

		g->setColor(0xff00ff00);
		g->pushTransform();
		{
			g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage2)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()*2 + 15));
			g->drawString(m_osu->getSubTitleFont(), errorMessage2);
		}
		g->popTransform();
	}

	// context menu
	m_contextMenu->draw(g);

	// click pulse animation overlay
	if (m_fPulseAnimation > 0.0f)
	{
		Color topColor = 0x00ffffff;
		Color bottomColor = COLOR((int)(25*m_fPulseAnimation), 255, 255, 255);

		g->setColor(0xffffffff);
		g->fillGradient(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), topColor, topColor, bottomColor, bottomColor);
	}

	// debug previous random beatmap
	/*
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(m_osu->getScreenWidth()/5, m_osu->getScreenHeight()/3);
	for (int i=0; i<m_previousRandomBeatmaps.size(); i++)
	{
		UString message = UString::format("#%i = ", i);
		message.append(m_previousRandomBeatmaps[i]->getTitle());

		g->drawString(m_osu->getSongBrowserFont(), message);
		g->translate(0, m_osu->getSongBrowserFont()->getHeight()+10);
	}
	g->popTransform();
	*/

	// debug thumbnail resource loading
	/*
	g->setColor(0xffffffff);
	g->pushTransform();
		g->translate(m_osu->getScreenWidth()/6, m_osu->getScreenHeight()/3);
		g->drawString(m_osu->getSongBrowserFont(), UString::format("res %i", engine->getResourceManager()->getNumResources()));
		g->translate(0, m_osu->getSongBrowserFont()->getHeight()*2);
		int numVisibleSongButtons = 0;
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			if ((*elements)[i]->isVisible())
				numVisibleSongButtons++;
		}
		g->drawString(m_osu->getSongBrowserFont(), UString::format("vis %i", numVisibleSongButtons));
	g->popTransform();
	*/
}

void OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(Graphics *g, Osu *osu, float alpha)
{
	if (osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL)
	{
		Image *backgroundImage = osu->getBackgroundImageHandler()->getLoadBackgroundImage(osu->getSelectedBeatmap()->getSelectedDifficulty2());
		if (backgroundImage != NULL && backgroundImage->isReady())
		{
			const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

			g->setColor(0xff999999);
			g->setAlpha(alpha);
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(osu->getScreenWidth()/2, osu->getScreenHeight()/2);
				g->drawImage(backgroundImage);
			}
			g->popTransform();
		}
	}
}

void OsuSongBrowser2::update()
{
	// HACKHACK: temporarily putting this on top as to support recalc while inPlayMode() (and songbrowser is invisible) for drawing strain graph in scrubbing timeline (and total stars statistics, and max total pp statistics)
	// handle background star calculation (1)
	if (m_bBackgroundStarCalcScheduled)
	{
		m_bBackgroundStarCalcScheduled = false;
		const bool force = m_bBackgroundStarCalcScheduledForce;
		m_bBackgroundStarCalcScheduledForce = false;

		recalculateStarsForSelectedBeatmap(force);
	}

	// HACKHACK: workaround for very wide back button skin images overlapping bottombar button hitboxes
	const bool isMouseInsideValidBackButtonHitbox = (engine->getMouse()->getPos().x < m_osu->getSkin()->getMenuBack2()->getSizeBase().x);
	if (m_bottombar->isMouseInside() && !isMouseInsideValidBackButtonHitbox)
		OsuScreenBackable::stealFocus();

	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// refresh logic (blocks every other call in the update() function below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		m_db->update();

		// check if we are finished loading
		if (m_db->isFinished())
		{
			m_bBeatmapRefreshScheduled = false;
			onDatabaseLoadingFinished();
		}
		return;
	}

	// HACKHACK: mouse wheel handling order
	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
		engine->getMouse()->resetWheelDelta();

	// update and focus handling
	m_contextMenu->update();
	m_songBrowser->update();
	m_songBrowser->getContainer()->update_pos(); // necessary due to constant animations
	m_bottombar->update();
	m_scoreBrowser->update();
	m_topbarLeft->update();
	m_topbarRight->update();

	if (m_contextMenu->isMouseInside() || m_osu->getHUD()->isVolumeOverlayBusy() || (m_backButton->isMouseInside() && isMouseInsideValidBackButtonHitbox))
	{
		m_topbarLeft->stealFocus();
		m_topbarRight->stealFocus();
		m_songBrowser->stealFocus();
		m_bottombar->stealFocus();
		if (!m_scoreBrowser->isBusy())
			m_scoreBrowser->stealFocus();
	}

	if (m_contextMenu->isMouseInside())
		stealFocus();

	if (m_bottombar->isMouseInside())
	{
		if (!m_scoreBrowser->isBusy())
			m_scoreBrowser->stealFocus();
	}

	if (m_osu->getOptionsMenu()->isMouseInside())
	{
		stealFocus();
		m_scoreBrowser->stealFocus();
		m_bottombar->stealFocus();
		m_contextMenu->stealFocus();
		m_songInfo->stealFocus();
		m_topbarLeft->stealFocus();
	}

	if (m_osu->getOptionsMenu()->isBusy())
		m_songBrowser->stealFocus();

	// handle right click absolute scrolling
	{
		if (engine->getMouse()->isRightDown() && !m_contextMenu->isMouseInside())
		{
			if (!m_bSongBrowserRightClickScrollCheck)
			{
				m_bSongBrowserRightClickScrollCheck = true;

				bool isMouseInsideAnySongButton = false;
				{
					const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
					for (CBaseUIElement *songButton : elements)
					{
						if (songButton->isMouseInside())
						{
							isMouseInsideAnySongButton = true;
							break;
						}
					}
				}

				if (m_songBrowser->isMouseInside() && !m_osu->getOptionsMenu()->isMouseInside() && !isMouseInsideAnySongButton)
					m_bSongBrowserRightClickScrolling = true;
				else
					m_bSongBrowserRightClickScrolling = false;
			}
		}
		else
		{
			m_bSongBrowserRightClickScrollCheck = false;
			m_bSongBrowserRightClickScrolling = false;
		}

		if (m_bSongBrowserRightClickScrolling)
			m_songBrowser->scrollToY(-((engine->getMouse()->getPos().y - 2 - m_songBrowser->getPos().y)/m_songBrowser->getSize().y)*m_songBrowser->getScrollSize().y);
	}

	// handle async random beatmap selection
	if (m_bRandomBeatmapScheduled)
	{
		m_bRandomBeatmapScheduled = false;
		selectRandomBeatmap();
	}
	if (m_bPreviousRandomBeatmapScheduled)
	{
		m_bPreviousRandomBeatmapScheduled = false;
		selectPreviousRandomBeatmap();
	}

	// if cursor is to the left edge of the screen, force center currently selected beatmap/diff
	// but only if the context menu is currently not visible (since we don't want move things while e.g. managing collections etc.)
	if (engine->getMouse()->getPos().x < m_osu->getScreenWidth()*0.1f && !m_contextMenu->isVisible())
		scrollToSelectedSongButton();

	// handle searching
	if (m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime)
	{
		m_fSearchWaitTime = 0.0f;
		onSearchUpdate();
	}

	// handle background search matcher
	{
		if (!m_backgroundSearchMatcher->isDead() && m_backgroundSearchMatcher->isAsyncReady())
		{
			// we have the results, now update the UI
			rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);

			m_backgroundSearchMatcher->kill();
		}

		if (m_backgroundSearchMatcher->isDead())
		{
			if (m_bOnAfterSortingOrGroupChangeUpdateScheduled)
			{
				m_bOnAfterSortingOrGroupChangeUpdateScheduled = false;
				onAfterSortingOrGroupChangeUpdateInt(m_bOnAfterSortingOrGroupChangeUpdateScheduledAutoScroll);
			}
		}
	}

	// handle background star calculation (2)
	// this goes through all loaded beatmaps and checks if stars + length have to be calculated and set (e.g. if without osu!.db database)
	if (m_beatmaps.size() > 0 && osu_songbrowser_background_star_calculation.getBool())
	{
		for (int s=0; s<1; s++) // one beatmap per update (leave one update loop gap between beatmaps to avoid stalling background image loading)
		{
			bool canMoveToNextBeatmap = true;
			if (m_iBackgroundStarCalculationIndex >= 0 && m_iBackgroundStarCalculationIndex < m_beatmaps.size())
			{
				OsuDatabaseBeatmap *beatmap = m_beatmaps[m_iBackgroundStarCalculationIndex];
				const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();

				if (!m_backgroundStarCalculator->isDead() && m_backgroundStarCalculator->isAsyncReady())
				{
					// we have a result, store it in db
					// this is up here to avoid one extra update loop until the next calc is triggered, but still have a gap between consecutive beatmap objects
					// NOTE: stars are upclamped to 0.0001f to signify that they have been calculated once for this diff (even if something fails), we only try once

					OsuDatabaseBeatmap *calculatedDiff = m_backgroundStarCalculator->getBeatmapDifficulty();
					calculatedDiff->setStarsNoMod(std::max(0.0001f, (float)m_backgroundStarCalculator->getTotalStars()));
					calculatedDiff->setNumObjects(m_backgroundStarCalculator->getNumObjects());
					calculatedDiff->setNumCircles(m_backgroundStarCalculator->getNumCircles());
					calculatedDiff->setNumSliders(std::max(0, m_backgroundStarCalculator->getNumObjects() - m_backgroundStarCalculator->getNumCircles() - m_backgroundStarCalculator->getNumSpinners()));
					calculatedDiff->setNumSpinners(m_backgroundStarCalculator->getNumSpinners());
					calculatedDiff->setLengthMS(std::max(calculatedDiff->getLengthMS(), (unsigned long)m_backgroundStarCalculator->getLengthMS()));

					// re-add (potentially changed stars, potentially changed length)
					readdBeatmap(calculatedDiff);

					// and update wrapper representative values (parent)
					if (m_backgroundStarCalcTempParent != NULL)
						m_backgroundStarCalcTempParent->updateSetHeuristics();

					m_backgroundStarCalculator->kill();
				}

				OsuDatabaseBeatmap *diffToCalc = NULL;
				if (diffs.size() > 0)
				{
					for (size_t d=0; d<diffs.size(); d++)
					{
						if (diffs[d]->getStarsNomod() <= 0.0f)
						{
							diffToCalc = diffs[d];
							break;
						}
					}
				}
				else if (diffToCalc->getStarsNomod() <= 0.0f)
					diffToCalc = beatmap;

				if (diffToCalc != NULL)
				{
					// bump notification
					m_fBackgroundStarCalculationWorkNotificationTime = engine->getTime() + 0.1f;

					// only one diff per beatmap per update
					canMoveToNextBeatmap = false;

					if (m_backgroundStarCalculator->isDead())
					{
						m_backgroundStarCalculator->revive();

						if (osu_debug_background_star_calc.getBool())
							printf("diffToCalc = %s\n", diffToCalc->getFilePath().toUtf8());

						// start new calc (nomod stars)
						{
							m_backgroundStarCalculator->release();

							const float AR = diffToCalc->getAR();
							const float CS = diffToCalc->getCS();
							const float OD = diffToCalc->getOD();
							const float HP = diffToCalc->getHP();
							const float speedMultiplier = 1.0f;
							m_backgroundStarCalculator->setBeatmapDifficulty(diffToCalc, AR, CS, OD, HP, speedMultiplier, false, false, false, false);
							m_backgroundStarCalcTempParent = (diffs.size() > 0 ? beatmap : NULL);

							engine->getResourceManager()->requestNextLoadAsync();
							engine->getResourceManager()->loadResource(m_backgroundStarCalculator);
						}
					}
				}
			}

			if (canMoveToNextBeatmap)
			{
				m_iBackgroundStarCalculationIndex++;
				if (m_iBackgroundStarCalculationIndex >= m_beatmaps.size())
					m_iBackgroundStarCalculationIndex = 0;

				m_iBackgroundStarCalculationIndex = clamp<int>(m_iBackgroundStarCalculationIndex, 0, m_beatmaps.size());
			}
		}
	}
}

void OsuSongBrowser2::onKeyDown(KeyboardEvent &key)
{
	OsuScreen::onKeyDown(key); // only used for options menu
	if (!m_bVisible || key.isConsumed()) return;

	if (m_bVisible && m_bBeatmapRefreshScheduled && (key == KEY_ESCAPE || key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt()))
	{
		m_db->cancel();
		key.consume();
		return;
	}

	if (m_bBeatmapRefreshScheduled) return;

	// context menu
	m_contextMenu->onKeyDown(key);
	if (key.isConsumed()) return;

	// searching text delete & escape key handling
	if (m_sSearchString.length() > 0)
	{
		switch (key.getKeyCode())
		{
		case KEY_DELETE:
		case KEY_BACKSPACE:
			key.consume();
			if (m_sSearchString.length() > 0)
			{
				if (engine->getKeyboard()->isControlDown())
				{
					// delete everything from the current caret position to the left, until after the first non-space character (but including it)
					bool foundNonSpaceChar = false;
					while (m_sSearchString.length() > 0)
					{
						UString curChar = m_sSearchString.substr(m_sSearchString.length()-1, 1);

						if (foundNonSpaceChar && curChar.isWhitespaceOnly())
							break;

						if (!curChar.isWhitespaceOnly())
							foundNonSpaceChar = true;

						m_sSearchString.erase(m_sSearchString.length()-1, 1);
					}
				}
				else
					m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length()-1);

				scheduleSearchUpdate(m_sSearchString.length() == 0);
			}
			break;

		case KEY_ESCAPE:
			key.consume();
			m_sSearchString = "";
			scheduleSearchUpdate(true);
			break;
		}
	}
	else if (!m_contextMenu->isVisible())
	{
		if (key == KEY_ESCAPE) // can't support GAME_PAUSE hotkey here because of text searching
			m_osu->toggleSongBrowser();
	}

	// paste clipboard support
	if (key == KEY_V)
	{
		if (engine->getKeyboard()->isControlDown())
		{
			const UString clipstring = env->getClipBoardText();
			if (clipstring.length() > 0)
			{
				m_sSearchString.append(clipstring);
				scheduleSearchUpdate(false);
			}
		}
	}

	if (key == KEY_SHIFT)
		m_bShiftPressed = true;

	// function hotkeys
	if ((key == KEY_F1 || key == (KEYCODE)OsuKeyBindings::TOGGLE_MODSELECT.getInt()) && !m_bF1Pressed)
	{
		m_bF1Pressed = true;
		m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 1 : 0]->keyboardPulse();
		onSelectionMods();
	}
	if ((key == KEY_F2 || key == (KEYCODE)OsuKeyBindings::RANDOM_BEATMAP.getInt()) && !m_bF2Pressed)
	{
		m_bF2Pressed = true;
		m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 2 : 1]->keyboardPulse();
		onSelectionRandom();
	}
	if (key == KEY_F3 && !m_bF3Pressed)
	{
		m_bF3Pressed = true;
		m_bottombarNavButtons[m_bottombarNavButtons.size() > 3 ? 3 : 2]->keyboardPulse();
		onSelectionOptions();
	}

	if (key == KEY_F5)
		refreshBeatmaps();

	// selection move
	if (!engine->getKeyboard()->isAltDown() && key == KEY_DOWN)
	{
		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

		// get bottom selection
		int selectedIndex = -1;
		for (int i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		// select +1
		if (selectedIndex > -1 && selectedIndex+1 < elements.size())
		{
			int nextSelectionIndex = selectedIndex+1;
			OsuUISongBrowserButton *nextButton = dynamic_cast<OsuUISongBrowserButton*>(elements[nextSelectionIndex]);
			OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>(elements[nextSelectionIndex]);
			if (nextButton != NULL)
			{
				nextButton->select(true, false);

				// if this is a song button, select top child
				if (songButton != NULL)
				{
					const std::vector<OsuUISongBrowserButton*> &children = songButton->getChildren();
					if (children.size() > 0 && !children[0]->isSelected())
						children[0]->select(true, false, false);
				}
			}
		}
	}

	if (!engine->getKeyboard()->isAltDown() && key == KEY_UP)
	{
		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

		// get bottom selection
		int selectedIndex = -1;
		for (int i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		// select -1
		if (selectedIndex > -1 && selectedIndex-1 > -1)
		{
			int nextSelectionIndex = selectedIndex-1;
			OsuUISongBrowserButton *nextButton = dynamic_cast<OsuUISongBrowserButton*>(elements[nextSelectionIndex]);
			bool isCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[nextSelectionIndex]);

			if (nextButton != NULL)
			{
				nextButton->select();

				// automatically open collection on top of this one and go to bottom child
				if (isCollectionButton && nextSelectionIndex-1 > -1)
				{
					nextSelectionIndex = nextSelectionIndex-1;
					OsuUISongBrowserCollectionButton *nextCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[nextSelectionIndex]);
					if (nextCollectionButton != NULL)
					{
						nextCollectionButton->select();

						std::vector<OsuUISongBrowserButton*> children = nextCollectionButton->getChildren();
						if (children.size() > 0 && !children[children.size()-1]->isSelected())
							children[children.size()-1]->select();
					}
				}
			}
		}
	}

	if (key == KEY_LEFT && !m_bLeft)
	{
		m_bLeft = true;

		const bool jumpToNextGroup = engine->getKeyboard()->isShiftDown();

		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

		bool foundSelected = false;
		for (int i=elements.size()-1; i>=0; i--)
		{
			const OsuUISongBrowserSongDifficultyButton *diffButtonPointer = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(elements[i]);
			const OsuUISongBrowserCollectionButton *collectionButtonPointer = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[i]);

			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
			const bool isSongDifficultyButtonAndNotIndependent = (diffButtonPointer != NULL && !diffButtonPointer->isIndependentDiffButton());

			if (foundSelected && button != NULL && !button->isSelected() && !isSongDifficultyButtonAndNotIndependent && (!jumpToNextGroup || collectionButtonPointer != NULL))
			{
				m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = true;
				{
					button->select();

					if (!jumpToNextGroup || collectionButtonPointer == NULL)
					{
						// automatically open collection below and go to bottom child
						OsuUISongBrowserCollectionButton *collectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[i]);
						if (collectionButton != NULL)
						{
							std::vector<OsuUISongBrowserButton*> children = collectionButton->getChildren();
							if (children.size() > 0 && !children[children.size()-1]->isSelected())
								children[children.size()-1]->select();
						}
					}
				}
				m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;

				break;
			}

			if (button != NULL && button->isSelected())
				foundSelected = true;
		}
	}

	if (key == KEY_RIGHT && !m_bRight)
	{
		m_bRight = true;

		const bool jumpToNextGroup = engine->getKeyboard()->isShiftDown();

		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

		// get bottom selection
		int selectedIndex = -1;
		for (size_t i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		if (selectedIndex > -1)
		{
			for (size_t i=selectedIndex; i<elements.size(); i++)
			{
				const OsuUISongBrowserSongDifficultyButton *diffButtonPointer = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(elements[i]);
				const OsuUISongBrowserCollectionButton *collectionButtonPointer = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[i]);

				OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
				const bool isSongDifficultyButtonAndNotIndependent = (diffButtonPointer != NULL && !diffButtonPointer->isIndependentDiffButton());

				if (button != NULL && !button->isSelected() && !isSongDifficultyButtonAndNotIndependent && (!jumpToNextGroup || collectionButtonPointer != NULL))
				{
					button->select();
					break;
				}
			}
		}
	}

	if (key == KEY_PAGEUP)
		m_songBrowser->scrollY(m_songBrowser->getSize().y);
	if (key == KEY_PAGEDOWN)
		m_songBrowser->scrollY(-m_songBrowser->getSize().y);

	// group open/close
	// NOTE: only closing works atm (no "focus" state on buttons yet)
	if (key == KEY_ENTER && engine->getKeyboard()->isShiftDown())
	{
		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

		for (int i=0; i<elements.size(); i++)
		{
			const OsuUISongBrowserCollectionButton *collectionButtonPointer = dynamic_cast<OsuUISongBrowserCollectionButton*>(elements[i]);

			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);

			if (collectionButtonPointer != NULL && button != NULL && button->isSelected())
			{
				button->select(); // deselect

				scrollToSongButton(button);

				break;
			}
		}
	}

	// selection select
	if (key == KEY_ENTER && !engine->getKeyboard()->isShiftDown())
		playSelectedDifficulty();

	// toggle auto
	if (key == KEY_A && engine->getKeyboard()->isControlDown())
		m_osu->getModSelector()->toggleAuto();

	key.consume();
}

void OsuSongBrowser2::onKeyUp(KeyboardEvent &key)
{
	// context menu
	m_contextMenu->onKeyUp(key);
	if (key.isConsumed()) return;

	if (key == KEY_SHIFT)
		m_bShiftPressed = false;
	if (key == KEY_LEFT)
		m_bLeft = false;
	if (key == KEY_RIGHT)
		m_bRight = false;

	if (key == KEY_F1 || key == (KEYCODE)OsuKeyBindings::TOGGLE_MODSELECT.getInt())
		m_bF1Pressed = false;
	if (key == KEY_F2 || key == (KEYCODE)OsuKeyBindings::RANDOM_BEATMAP.getInt())
		m_bF2Pressed = false;
	if (key == KEY_F3)
		m_bF3Pressed = false;
}

void OsuSongBrowser2::onChar(KeyboardEvent &e)
{
	// context menu
	m_contextMenu->onChar(e);
	if (e.isConsumed()) return;

	if (e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown())) return;
	if (m_bF1Pressed || m_bF2Pressed || m_bF3Pressed) return;

	// handle searching
	KEYCODE charCode = e.getCharCode();
	UString stringChar = "";
	stringChar.insert(0, charCode);
	m_sSearchString.append(stringChar);

	scheduleSearchUpdate();
}

void OsuSongBrowser2::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);
}

void OsuSongBrowser2::setVisible(bool visible)
{
	m_bVisible = visible;
	m_bShiftPressed = false; // seems to get stuck sometimes otherwise

	if (m_bVisible)
	{
		OsuRichPresence::onSongBrowser(m_osu);

		updateLayout();

		// we have to re-select the current beatmap to start playing music again
		if (m_selectedBeatmap != NULL)
			m_selectedBeatmap->select();

		m_bHasSelectedAndIsPlaying = false; // sanity

		// try another refresh, maybe the osu!folder has changed
		if (m_beatmaps.size() == 0)
			refreshBeatmaps();

		// update user name/stats
		onUserButtonChange(m_name_ref->getString(), -1);

		// HACKHACK: workaround for BaseUI framework deficiency (missing mouse events. if a mouse button is being held, and then suddenly a BaseUIElement gets put under it and set visible, and then the mouse button is released, that "incorrectly" fires onMouseUpInside/onClicked/etc.)
		engine->getMouse()->onLeftChange(false);
		engine->getMouse()->onRightChange(false);
	}
	else
		m_contextMenu->setVisible2(false);
}

void OsuSongBrowser2::onPlayEnd(bool quit)
{
	m_bHasSelectedAndIsPlaying = false;

	// update score displays
	if (!quit)
	{
		rebuildScoreButtons();

		OsuUISongBrowserSongDifficultyButton *selectedSongDiffButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(findCurrentlySelectedSongButton());
		if (selectedSongDiffButton != NULL)
			selectedSongDiffButton->updateGrade();
	}

	// update song info
	if (m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty2() != NULL)
		m_songInfo->setFromBeatmap(m_selectedBeatmap, m_selectedBeatmap->getSelectedDifficulty2());
}

void OsuSongBrowser2::onSelectionChange(OsuUISongBrowserButton *button, bool rebuild)
{
	if (button == NULL) return;

	m_contextMenu->setVisible2(false);

	// keep track and update all selection states
	// I'm still not happy with this, but at least all state update logic is localized in this function instead of spread across all buttons

	OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(button);
	OsuUISongBrowserSongDifficultyButton *songDiffButtonPointer = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(button);
	OsuUISongBrowserCollectionButton *collectionButtonPointer = dynamic_cast<OsuUISongBrowserCollectionButton*>(button);

	///debugLog("onSelectionChange(%i, %i, %i)\n", (int)(songButtonPointer != NULL), (int)(songDiffButtonPointer != NULL), (int)(collectionButtonPointer != NULL));

	if (songDiffButtonPointer != NULL)
	{
		if (m_selectionPreviousSongDiffButton != NULL && m_selectionPreviousSongDiffButton != songDiffButtonPointer)
			m_selectionPreviousSongDiffButton->deselect();

		// support individual diffs independent from their parent song button container
		{
			// if the new diff has a parent song button, then update its selection state (select it to stay consistent)
			if (songDiffButtonPointer->getParentSongButton() != NULL && !songDiffButtonPointer->getParentSongButton()->isSelected())
			{
				songDiffButtonPointer->getParentSongButton()->sortChildren(); // NOTE: workaround for disabled callback firing in select()
				songDiffButtonPointer->getParentSongButton()->select(false);
				onSelectionChange(songDiffButtonPointer->getParentSongButton(), false); // NOTE: recursive call
			}

			// if the new diff does not have a parent song button, but the previous diff had, then update the previous diff parent song button selection state (to deselect it)
			if (songDiffButtonPointer->getParentSongButton() == NULL)
			{
				if (m_selectionPreviousSongDiffButton != NULL && m_selectionPreviousSongDiffButton->getParentSongButton() != NULL)
					m_selectionPreviousSongDiffButton->getParentSongButton()->deselect();
			}
		}

		m_selectionPreviousSongDiffButton = songDiffButtonPointer;
	}
	else if (songButtonPointer != NULL)
	{
		if (m_selectionPreviousSongButton != NULL && m_selectionPreviousSongButton != songButtonPointer)
			m_selectionPreviousSongButton->deselect();
		if (m_selectionPreviousSongDiffButton != NULL)
			m_selectionPreviousSongDiffButton->deselect();

		m_selectionPreviousSongButton = songButtonPointer;
	}
	else if (collectionButtonPointer != NULL)
	{
		// TODO: maybe expand this logic with per-group-type last-open-collection memory

		// logic for allowing collections to be deselected by clicking on the same button (contrary to how beatmaps work)
		const bool isTogglingCollection = (m_selectionPreviousCollectionButton != NULL && m_selectionPreviousCollectionButton == collectionButtonPointer);

		if (m_selectionPreviousCollectionButton != NULL)
			m_selectionPreviousCollectionButton->deselect();

		m_selectionPreviousCollectionButton = collectionButtonPointer;

		if (isTogglingCollection)
			m_selectionPreviousCollectionButton = NULL;
	}

	if (rebuild)
		rebuildSongButtons();
}

void OsuSongBrowser2::onDifficultySelected(OsuDatabaseBeatmap *diff2, bool play, bool mp)
{
	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::SELECT, 0, false, diff2);

	// legacy logic (deselect = unload)
	const bool wasSelectedBeatmapNULL = (m_selectedBeatmap == NULL);
	if (m_selectedBeatmap != NULL)
		m_selectedBeatmap->deselect();

	// create/recreate/cache runtime beatmap object depending on gamemode
	if (m_osu->getGamemode() == Osu::GAMEMODE::STD && dynamic_cast<OsuBeatmapStandard*>(m_selectedBeatmap) == NULL)
	{
		SAFE_DELETE(m_selectedBeatmap);
		m_selectedBeatmap = new OsuBeatmapStandard(m_osu);
	}
	else if (m_osu->getGamemode() == Osu::GAMEMODE::MANIA && dynamic_cast<OsuBeatmapMania*>(m_selectedBeatmap) == NULL)
	{
		SAFE_DELETE(m_selectedBeatmap);
		m_selectedBeatmap = new OsuBeatmapMania(m_osu);
	}

	// remember it
	if (diff2 != m_selectedBeatmap->getSelectedDifficulty2())
		m_previousRandomBeatmaps.push_back(diff2);

	// select diff on runtime beatmap object
	m_selectedBeatmap->selectDifficulty2(diff2);
	if (wasSelectedBeatmapNULL)
	{
		// force update music through songbrowser refreshes (db reloads)
		m_selectedBeatmap->deselect();
		m_selectedBeatmap->select();
	}

	// update song info
	m_songInfo->setFromBeatmap(m_selectedBeatmap, diff2);

	// start playing
	if (play)
	{
		bool clientPlayStateChangeRequestBeatmapSent = false;
		if (m_osu->isInMultiplayer() && !mp)
		{
			// clients may also select beatmaps (the server can then decide if it wants to broadcast or ignore it)
			clientPlayStateChangeRequestBeatmapSent = m_osu->getMultiplayer()->onClientPlayStateChangeRequestBeatmap(diff2);
		}

		if (!clientPlayStateChangeRequestBeatmapSent)
		{
			// CTRL + click = auto
			if (!m_osu->isInMultiplayer() && engine->getKeyboard()->isControlDown())
				m_osu->getModSelector()->enableAuto();

			m_osu->onBeforePlayStart();
			if (m_selectedBeatmap->play())
			{
				m_bHasSelectedAndIsPlaying = true;
				setVisible(false);

				m_osu->onPlayStart();
			}
		}
	}

	// animate
	m_fPulseAnimation = 1.0f;
	anim->moveLinear(&m_fPulseAnimation, 0.0f, 0.55f, true);

	// update score display
	rebuildScoreButtons();

	// update web button
	m_webButton->setVisible(m_songInfo->getBeatmapID() > 0);

	// trigger dynamic star calc (including current mods etc.)
	recalculateStarsForSelectedBeatmap();
}

void OsuSongBrowser2::onDifficultySelectedMP(OsuDatabaseBeatmap *diff2, bool play)
{
	onDifficultySelected(diff2, play, true);
}

void OsuSongBrowser2::selectBeatmapMP(OsuDatabaseBeatmap *diff2)
{
	// force exit search
	if (m_sSearchString.length() > 0)
	{
		m_sSearchString = "";
		osu_songbrowser_search_hardcoded_filter.setValue(m_sSearchString); // safety
		onSearchUpdate();
	}

	// force no grouping
	if (m_group != GROUP::GROUP_NO_GROUPING)
		onGroupChange("", 0);

	OsuUISongBrowserButton *matchingButton = NULL;
	for (size_t i=0; i<m_songButtons.size(); i++)
	{
		if (m_songButtons[i]->getDatabaseBeatmap() == diff2)
		{
			matchingButton = m_songButtons[i];
			break;
		}

		const std::vector<OsuUISongBrowserButton*> &children = m_songButtons[i]->getChildren();
		for (size_t c=0; c<children.size(); c++)
		{
			if (children[c]->getDatabaseBeatmap() == diff2)
			{
				matchingButton = children[c];
				break;
			}
		}

		if (matchingButton != NULL)
			break;
	}

	if (matchingButton != NULL)
	{
		if (!matchingButton->isSelected())
			matchingButton->select();
	}
}

void OsuSongBrowser2::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying) return;

	// reset
	checkHandleKillBackgroundStarCalculator();
	checkHandleKillDynamicStarCalculator(false);
	checkHandleKillBackgroundSearchMatcher();

	m_selectedBeatmap = NULL;

	m_selectionPreviousSongButton = NULL;
	m_selectionPreviousSongDiffButton = NULL;
	m_selectionPreviousCollectionButton = NULL;

	// delete local database and UI
	m_songBrowser->getContainer()->empty();

	for (size_t i=0; i<m_songButtons.size(); i++)
	{
		delete m_songButtons[i];
	}
	m_songButtons.clear();
	for (size_t i=0; i<m_collectionButtons.size(); i++)
	{
		delete m_collectionButtons[i];
	}
	m_collectionButtons.clear();
	for (size_t i=0; i<m_artistCollectionButtons.size(); i++)
	{
		delete m_artistCollectionButtons[i];
	}
	m_artistCollectionButtons.clear();
	for (size_t i=0; i<m_difficultyCollectionButtons.size(); i++)
	{
		delete m_difficultyCollectionButtons[i];
	}
	m_difficultyCollectionButtons.clear();
	for (size_t i=0; i<m_bpmCollectionButtons.size(); i++)
	{
		delete m_bpmCollectionButtons[i];
	}
	m_bpmCollectionButtons.clear();
	for (size_t i=0; i<m_creatorCollectionButtons.size(); i++)
	{
		delete m_creatorCollectionButtons[i];
	}
	m_creatorCollectionButtons.clear();
	for (size_t i=0; i<m_dateaddedCollectionButtons.size(); i++)
	{
		delete m_dateaddedCollectionButtons[i];
	}
	m_dateaddedCollectionButtons.clear();
	for (size_t i=0; i<m_lengthCollectionButtons.size(); i++)
	{
		delete m_lengthCollectionButtons[i];
	}
	m_lengthCollectionButtons.clear();
	for (size_t i=0; i<m_titleCollectionButtons.size(); i++)
	{
		delete m_titleCollectionButtons[i];
	}
	m_titleCollectionButtons.clear();

	m_visibleSongButtons.clear();
	m_beatmaps.clear();
	m_previousRandomBeatmaps.clear();

	m_contextMenu->setVisible2(false);

	// clear potentially active search
	m_bInSearch = false;
	m_sSearchString = "";
	m_sPrevSearchString = "";
	m_fSearchWaitTime = 0.0f;
	m_searchPrevGroup = GROUP::GROUP_NO_GROUPING;

	// force no grouping
	if (m_group != GROUP::GROUP_NO_GROUPING)
		onGroupChange("", 0);

	// start loading
	m_bBeatmapRefreshScheduled = true;
	m_db->load();
}

void OsuSongBrowser2::addBeatmap(OsuDatabaseBeatmap *beatmap)
{
	if (beatmap->getDifficulties().size() < 1) return;

	OsuUISongBrowserSongButton *songButton;
	if (beatmap->getDifficulties().size() > 1)
		songButton = new OsuUISongBrowserSongButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size()*50, 200, 50, "", beatmap);
	else
		songButton = new OsuUISongBrowserSongDifficultyButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size()*50, 200, 50, "", beatmap->getDifficulties()[0], NULL);

	m_songButtons.push_back(songButton);

	// prebuild temporary list of all relevant buttons, used by some groups
	std::vector<OsuUISongBrowserButton*> tempChildrenForGroups;
	{
		if (songButton->getChildren().size() > 0)
		{
			for (OsuUISongBrowserButton *child : songButton->getChildren())
			{
				tempChildrenForGroups.push_back(child);
			}
		}
		else
			tempChildrenForGroups.push_back(songButton);
	}

	// add beatmap to all necessary groups
	{
		// artist
		if (m_artistCollectionButtons.size() == 28)
		{
			const UString &artist = beatmap->getArtist();
			if (artist.length() > 0)
			{
				const char firstChar = artist.toUtf8()[0];

				const bool isNumber = (firstChar >= '0' && firstChar <= '9');
				const bool isLowerCase = (firstChar >= 'a' && firstChar <= 'z');
				const bool isUpperCase = (firstChar >= 'A' && firstChar <= 'Z');

				if (isNumber)
					m_artistCollectionButtons[0]->getChildren().push_back(songButton);
				else if (isLowerCase || isUpperCase)
				{
					const int index = 1 + (25 - (isLowerCase ? 'z' - firstChar : 'Z' - firstChar));
					if (index > 0 && index < 27)
						m_artistCollectionButtons[index]->getChildren().push_back(songButton);
				}
				else
					m_artistCollectionButtons[27]->getChildren().push_back(songButton);
			}
		}

		// difficulty
		if (m_difficultyCollectionButtons.size() == 12)
		{
			for (size_t i=0; i<tempChildrenForGroups.size(); i++)
			{
				const int index = clamp<int>((int)tempChildrenForGroups[i]->getDatabaseBeatmap()->getStarsNomod(), 0, 11);
				m_difficultyCollectionButtons[index]->getChildren().push_back(tempChildrenForGroups[i]);
			}
		}

		// bpm
		if (m_bpmCollectionButtons.size() == 6)
		{
			// TODO: 6 buttons (60, 120, 180, 240, 300, >300)
			// TODO: have to rip apart children and group separately depending on bpm, ffs
		}

		// creator
		if (m_creatorCollectionButtons.size() == 28)
		{
			const UString &creator = beatmap->getCreator();
			if (creator.length() > 0)
			{
				const char firstChar = creator.toUtf8()[0];

				const bool isNumber = (firstChar >= '0' && firstChar <= '9');
				const bool isLowerCase = (firstChar >= 'a' && firstChar <= 'z');
				const bool isUpperCase = (firstChar >= 'A' && firstChar <= 'Z');

				if (isNumber)
					m_creatorCollectionButtons[0]->getChildren().push_back(songButton);
				else if (isLowerCase || isUpperCase)
				{
					const int index = 1 + (25 - (isLowerCase ? 'z' - firstChar : 'Z' - firstChar));
					if (index > 0 && index < 27)
						m_creatorCollectionButtons[index]->getChildren().push_back(songButton);
				}
				else
					m_creatorCollectionButtons[27]->getChildren().push_back(songButton);
			}
		}

		// dateadded
		{
			// TODO: extremely annoying
		}

		// length
		if (m_lengthCollectionButtons.size() == 7)
		{
			for (size_t i=0; i<tempChildrenForGroups.size(); i++)
			{
				const unsigned long lengthMS = tempChildrenForGroups[i]->getDatabaseBeatmap()->getLengthMS();
				if (lengthMS <= 1000*60)
					m_lengthCollectionButtons[0]->getChildren().push_back(tempChildrenForGroups[i]);
				else if (lengthMS <= 1000*60*2)
					m_lengthCollectionButtons[1]->getChildren().push_back(tempChildrenForGroups[i]);
				else if (lengthMS <= 1000*60*3)
					m_lengthCollectionButtons[2]->getChildren().push_back(tempChildrenForGroups[i]);
				else if (lengthMS <= 1000*60*4)
					m_lengthCollectionButtons[3]->getChildren().push_back(tempChildrenForGroups[i]);
				else if (lengthMS <= 1000*60*5)
					m_lengthCollectionButtons[4]->getChildren().push_back(tempChildrenForGroups[i]);
				else if (lengthMS <= 1000*60*10)
					m_lengthCollectionButtons[5]->getChildren().push_back(tempChildrenForGroups[i]);
				else
					m_lengthCollectionButtons[6]->getChildren().push_back(tempChildrenForGroups[i]);
			}
		}

		// title
		if (m_titleCollectionButtons.size() == 28)
		{
			const UString &creator = beatmap->getTitle();
			if (creator.length() > 0)
			{
				const char firstChar = creator.toUtf8()[0];

				const bool isNumber = (firstChar >= '0' && firstChar <= '9');
				const bool isLowerCase = (firstChar >= 'a' && firstChar <= 'z');
				const bool isUpperCase = (firstChar >= 'A' && firstChar <= 'Z');

				if (isNumber)
					m_titleCollectionButtons[0]->getChildren().push_back(songButton);
				else if (isLowerCase || isUpperCase)
				{
					const int index = 1 + (25 - (isLowerCase ? 'z' - firstChar : 'Z' - firstChar));
					if (index > 0 && index < 27)
						m_titleCollectionButtons[index]->getChildren().push_back(songButton);
				}
				else
					m_titleCollectionButtons[27]->getChildren().push_back(songButton);
			}
		}
	}
}

void OsuSongBrowser2::readdBeatmap(OsuDatabaseBeatmap *diff2)
{
	// this function readds updated diffs to the correct groups
	// i.e. when stars and length are calculated in background

	// NOTE: the difficulty and length groups only contain diffs (and no parent wrapper objects), makes searching for the button way easier

	// difficulty group
	{
		// remove from difficulty group
		OsuUISongBrowserButton *difficultyGroupButton = NULL;
		for (size_t i=0; i<m_difficultyCollectionButtons.size(); i++)
		{
			OsuUISongBrowserCollectionButton *groupButton = m_difficultyCollectionButtons[i];
			std::vector<OsuUISongBrowserButton*> &children = groupButton->getChildren();
			for (size_t c=0; c<children.size(); c++)
			{
				if (children[c]->getDatabaseBeatmap() == diff2)
				{
					difficultyGroupButton = children[c];

					// remove
					children.erase(children.begin() + c);

					break;
				}
			}
		}

		// add to new difficulty group
		if (difficultyGroupButton != NULL)
		{
			// HACKHACK: partial code duplication, see addBeatmap()
			if (m_difficultyCollectionButtons.size() == 12)
			{
				const int index = clamp<int>((int)diff2->getStarsNomod(), 0, 11);
				m_difficultyCollectionButtons[index]->getChildren().push_back(difficultyGroupButton);
			}
		}
	}

	// length group
	{
		// remove from length group
		OsuUISongBrowserButton *lengthGroupButton = NULL;
		for (size_t i=0; i<m_lengthCollectionButtons.size(); i++)
		{
			OsuUISongBrowserCollectionButton *groupButton = m_lengthCollectionButtons[i];
			std::vector<OsuUISongBrowserButton*> &children = groupButton->getChildren();
			for (size_t c=0; c<children.size(); c++)
			{
				if (children[c]->getDatabaseBeatmap() == diff2)
				{
					lengthGroupButton = children[c];

					// remove
					children.erase(children.begin() + c);

					break;
				}
			}
		}

		// add to new length group
		if (lengthGroupButton != NULL)
		{
			// HACKHACK: partial code duplication, see addBeatmap()
			if (m_lengthCollectionButtons.size() == 7)
			{
				const unsigned long lengthMS = diff2->getLengthMS();
				if (lengthMS <= 1000*60)
					m_lengthCollectionButtons[0]->getChildren().push_back(lengthGroupButton);
				else if (lengthMS <= 1000*60*2)
					m_lengthCollectionButtons[1]->getChildren().push_back(lengthGroupButton);
				else if (lengthMS <= 1000*60*3)
					m_lengthCollectionButtons[2]->getChildren().push_back(lengthGroupButton);
				else if (lengthMS <= 1000*60*4)
					m_lengthCollectionButtons[3]->getChildren().push_back(lengthGroupButton);
				else if (lengthMS <= 1000*60*5)
					m_lengthCollectionButtons[4]->getChildren().push_back(lengthGroupButton);
				else if (lengthMS <= 1000*60*10)
					m_lengthCollectionButtons[5]->getChildren().push_back(lengthGroupButton);
				else
					m_lengthCollectionButtons[6]->getChildren().push_back(lengthGroupButton);
			}
		}
	}
}

void OsuSongBrowser2::requestNextScrollToSongButtonJumpFix(OsuUISongBrowserSongDifficultyButton *diffButton)
{
	if (diffButton == NULL) return;

	m_bNextScrollToSongButtonJumpFixScheduled = true;
	m_fNextScrollToSongButtonJumpFixOldRelPosY = (diffButton->getParentSongButton() != NULL ? diffButton->getParentSongButton()->getRelPos().y : diffButton->getRelPos().y);
	m_fNextScrollToSongButtonJumpFixOldScrollSizeY = m_songBrowser->getScrollSize().y;
}

void OsuSongBrowser2::scrollToSongButton(OsuUISongBrowserButton *songButton, bool alignOnTop)
{
	if (songButton == NULL) return;

	// NOTE: compensate potential scroll jump due to added/removed elements (feels a lot better this way, also easier on the eyes)
	if (m_bNextScrollToSongButtonJumpFixScheduled)
	{
		m_bNextScrollToSongButtonJumpFixScheduled = false;

		float delta = 0.0f;
		{
			if (!m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta)
				delta = (songButton->getRelPos().y - m_fNextScrollToSongButtonJumpFixOldRelPosY); // (default case)
			else
				delta = m_songBrowser->getScrollSize().y - m_fNextScrollToSongButtonJumpFixOldScrollSizeY; // technically not correct but feels a lot better for KEY_LEFT navigation
		}
		m_songBrowser->scrollToY(m_songBrowser->getScrollPosY() - delta, false);
	}

	m_songBrowser->scrollToY(-songButton->getRelPos().y + (alignOnTop ? (0) : (m_songBrowser->getSize().y/2 - songButton->getSize().y/2)));
}

OsuUISongBrowserButton *OsuSongBrowser2::findCurrentlySelectedSongButton() const
{
	OsuUISongBrowserButton *selectedButton = NULL;
	const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
		if (button != NULL && button->isSelected()) // NOTE: fall through multiple selected buttons (e.g. collections)
			selectedButton = button;
	}
	return selectedButton;
}

void OsuSongBrowser2::scrollToSelectedSongButton()
{
	auto selectedButton = findCurrentlySelectedSongButton();
	scrollToSongButton(selectedButton);
}

void OsuSongBrowser2::rebuildSongButtons()
{
	m_songBrowser->getContainer()->empty();

	// NOTE: currently supports 3 depth layers (collection > beatmap > diffs)
	for (size_t i=0; i<m_visibleSongButtons.size(); i++)
	{
		OsuUISongBrowserButton *button = m_visibleSongButtons[i];
		button->resetAnimations();

		if (!(button->isSelected() && button->isHiddenIfSelected()))
			m_songBrowser->getContainer()->addBaseUIElement(button);

		// children
		if (button->isSelected())
		{
			const std::vector<OsuUISongBrowserButton*> &children = m_visibleSongButtons[i]->getChildren();
			for (size_t c=0; c<children.size(); c++)
			{
				OsuUISongBrowserButton *button2 = children[c];

				bool isButton2SearchMatch = false;
				if (button2->getChildren().size() > 0)
				{
					const std::vector<OsuUISongBrowserButton*> &children2 = button2->getChildren();
					for (size_t c2=0; c2<children2.size(); c2++)
					{
						const OsuUISongBrowserButton *button3 = children2[c2];
						if (button3->isSearchMatch())
						{
							isButton2SearchMatch = true;
							break;
						}
					}
				}
				else
					isButton2SearchMatch = button2->isSearchMatch();

				if (m_bInSearch && !isButton2SearchMatch)
					continue;

				button2->resetAnimations();

				if (!(button2->isSelected() && button2->isHiddenIfSelected()))
					m_songBrowser->getContainer()->addBaseUIElement(button2);

				// child children
				if (button2->isSelected())
				{
					const std::vector<OsuUISongBrowserButton*> &children2 = button2->getChildren();
					for (size_t c2=0; c2<children2.size(); c2++)
					{
						OsuUISongBrowserButton *button3 = children2[c2];

						if (m_bInSearch && !button3->isSearchMatch())
							continue;

						button3->resetAnimations();

						if (!(button3->isSelected() && button3->isHiddenIfSelected()))
							m_songBrowser->getContainer()->addBaseUIElement(button3);
					}
				}
			}
		}
	}

	// TODO: regroup diffs which are next to each other into one song button (parent button)
	// TODO: regrouping is non-deterministic, depending on the searching method used.
	// TODO: meaning that any number of "clusters" of diffs belonging to the same beatmap could build, requiring multiple song "parent" buttons for the same beatmap (if touching group size >= 2)
	// TODO: when regrouping, these "fake" parent buttons have to be deleted on every reload. this means that the selection state logic has to be kept cleared of any invalid pointers!
	// TODO: (including everything else which would rely on having a permanent pointer to an OsuUISongBrowserSongButton)

	updateSongButtonLayout();
}

void OsuSongBrowser2::updateSongButtonLayout()
{
	// this rebuilds the entire songButton layout (songButtons in relation to others)
	// only the y axis is set, because the x axis is constantly animated and handled within the button classes themselves
	const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();

	int yCounter = m_songBrowser->getSize().y/4;
	if (elements.size() <= 1)
		yCounter = m_songBrowser->getSize().y/2;

	bool isSelected = false;
	bool inOpenCollection = false;
	for (size_t i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);

		if (songButton != NULL)
		{
			const OsuUISongBrowserSongDifficultyButton *diffButtonPointer = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(songButton);

			// depending on the object type, layout differently
			const bool isCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(songButton) != NULL;
			const bool isDiffButton = diffButtonPointer != NULL;
			const bool isIndependentDiffButton = isDiffButton && diffButtonPointer->isIndependentDiffButton();

			// give selected items & diffs a bit more spacing, to make them stand out
			if (((songButton->isSelected() && !isCollectionButton) || isSelected || (isDiffButton && !isIndependentDiffButton)))
				yCounter += songButton->getSize().y*0.1f;

			isSelected = songButton->isSelected() || (isDiffButton && !isIndependentDiffButton);

			// give collections a bit more spacing at start & end
			if ((songButton->isSelected() && isCollectionButton))
				yCounter += songButton->getSize().y*0.2f;
			if (inOpenCollection && isCollectionButton && !songButton->isSelected())
				yCounter += songButton->getSize().y*0.2f;
			if (isCollectionButton)
			{
				if (songButton->isSelected())
					inOpenCollection = true;
				else
					inOpenCollection = false;
			}

			songButton->setTargetRelPosY(yCounter);
			songButton->updateLayoutEx();

			yCounter += songButton->getActualSize().y;
		}
	}
	m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y/2);
}

void OsuSongBrowser2::updateSongButtonSorting()
{
	onSortChange(osu_songbrowser_scores_sortingtype.getString());
}

bool OsuSongBrowser2::searchMatcher(const OsuDatabaseBeatmap *databaseBeatmap, const std::vector<UString> &searchStringTokens)
{
	if (databaseBeatmap == NULL) return false;

	const std::vector<OsuDatabaseBeatmap*> &diffs = databaseBeatmap->getDifficulties();
	const bool isContainer = (diffs.size() > 0);
	const int numDiffs = (isContainer ? diffs.size() : 1);

	// TODO: optimize this dumpster fire. can at least cache the parsed tokens and literal strings array instead of parsing every single damn time

	// intelligent search parser
	// all strings which are not expressions get appended with spaces between, then checked with one call to findSubstringInDifficulty()
	// the rest is interpreted
	// NOTE: this code is quite shitty. the order of the operators array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
	enum operatorId
	{
		EQ,
		LT,
		GT,
		LE,
		GE,
		NE
	};
	static const std::vector<std::pair<UString, operatorId>> operators =
	{
		std::pair<UString, operatorId>("<=",LE),
		std::pair<UString, operatorId>(">=",GE),
		std::pair<UString, operatorId>("<", LT),
		std::pair<UString, operatorId>(">", GT),
		std::pair<UString, operatorId>("!=",NE),
		std::pair<UString, operatorId>("==",EQ),
		std::pair<UString, operatorId>("=", EQ),
	};

	enum keywordId
	{
		AR,
		CS,
		OD,
		HP,
		BPM,
		OPM,
		CPM,
		SPM,
		OBJECTS,
		CIRCLES,
		SLIDERS,
		SPINNERS,
		LENGTH,
		STARS,
	};
	static const std::vector<std::pair<UString, keywordId>> keywords =
	{
		std::pair<UString, keywordId>("ar", AR),
		std::pair<UString, keywordId>("cs", CS),
		std::pair<UString, keywordId>("od", OD),
		std::pair<UString, keywordId>("hp", HP),
		std::pair<UString, keywordId>("bpm",BPM),
		std::pair<UString, keywordId>("opm",OPM),
		std::pair<UString, keywordId>("cpm",CPM),
		std::pair<UString, keywordId>("spm",SPM),
		std::pair<UString, keywordId>("object",OBJECTS),
		std::pair<UString, keywordId>("objects",OBJECTS),
		std::pair<UString, keywordId>("circle",CIRCLES),
		std::pair<UString, keywordId>("circles",CIRCLES),
		std::pair<UString, keywordId>("slider",SLIDERS),
		std::pair<UString, keywordId>("sliders",SLIDERS),
		std::pair<UString, keywordId>("spinner",SPINNERS),
		std::pair<UString, keywordId>("spinners",SPINNERS),
		std::pair<UString, keywordId>("length", LENGTH),
		std::pair<UString, keywordId>("len", LENGTH),
		std::pair<UString, keywordId>("stars", STARS),
		std::pair<UString, keywordId>("star", STARS)
	};

	// split search string into tokens
	// parse over all difficulties
	bool expressionMatches = false; // if any diff matched all expressions
	std::vector<UString> literalSearchStrings;
	for (size_t d=0; d<numDiffs; d++)
	{
		const OsuDatabaseBeatmap *diff = (isContainer ? diffs[d] : databaseBeatmap);

		bool expressionsMatch = true; // if the current search string (meaning only the expressions in this case) matches the current difficulty

		for (size_t i=0; i<searchStringTokens.size(); i++)
		{
			//debugLog("token[%i] = %s\n", i, tokens[i].toUtf8());
			// determine token type, interpret expression
			bool expression = false;
			for (size_t o=0; o<operators.size(); o++)
			{
				if (searchStringTokens[i].find(operators[o].first) != -1)
				{
					// split expression into left and right parts (only accept singular expressions, things like "0<bpm<1" will not work with this)
					//debugLog("splitting by string %s\n", operators[o].first.toUtf8());
					std::vector<UString> values = searchStringTokens[i].split(operators[o].first);
					if (values.size() == 2 && values[0].length() > 0 && values[1].length() > 0)
					{
						const UString lvalue = values[0];
						const int rvaluePercentIndex = values[1].find("%");
						const bool rvalueIsPercent = (rvaluePercentIndex != -1);
						const float rvalue = (rvaluePercentIndex == -1 ? values[1].toFloat() : values[1].substr(0, rvaluePercentIndex).toFloat()); // this must always be a number (at least, assume it is)

						// find lvalue keyword in array (only continue if keyword exists)
						for (size_t k=0; k<keywords.size(); k++)
						{
							if (keywords[k].first == lvalue)
							{
								expression = true;

								// we now have a valid expression: the keyword, the operator and the value

								// solve keyword
								float compareValue = 5.0f;
								switch (keywords[k].second)
								{
								case AR:
									compareValue = diff->getAR();
									break;
								case CS:
									compareValue = diff->getCS();
									break;
								case OD:
									compareValue = diff->getOD();
									break;
								case HP:
									compareValue = diff->getHP();
									break;
								case BPM:
									compareValue = diff->getMostCommonBPM();
									break;
								case OPM:
									compareValue = (diff->getLengthMS() > 0 ? ((float)diff->getNumObjects() / (float)(diff->getLengthMS() / 1000.0f / 60.0f)) : 0.0f) * databaseBeatmap->getOsu()->getSpeedMultiplier();
									break;
								case CPM:
									compareValue = (diff->getLengthMS() > 0 ? ((float)diff->getNumCircles() / (float)(diff->getLengthMS() / 1000.0f / 60.0f)) : 0.0f) * databaseBeatmap->getOsu()->getSpeedMultiplier();
									break;
								case SPM:
									compareValue = (diff->getLengthMS() > 0 ? ((float)diff->getNumSliders() / (float)(diff->getLengthMS() / 1000.0f / 60.0f)) : 0.0f) * databaseBeatmap->getOsu()->getSpeedMultiplier();
									break;
								case OBJECTS:
									compareValue = diff->getNumObjects();
									break;
								case CIRCLES:
									compareValue = (rvalueIsPercent ? ((float)diff->getNumCircles() / (float)diff->getNumObjects())*100.0f : diff->getNumCircles());
									break;
								case SLIDERS:
									compareValue = (rvalueIsPercent ? ((float)diff->getNumSliders() / (float)diff->getNumObjects())*100.0f : diff->getNumSliders());
									break;
								case SPINNERS:
									compareValue = (rvalueIsPercent ? ((float)diff->getNumSpinners() / (float)diff->getNumObjects())*100.0f : diff->getNumSpinners());
									break;
								case LENGTH:
									compareValue = diff->getLengthMS() / 1000;
									break;
								case STARS:
									compareValue = std::round(diff->getStarsNomod() * 100.0f) / 100.0f; // round to 2 decimal places
									break;
								}

								// solve operator
								bool matches = false;
								switch (operators[o].second)
								{
								case LE:
									if (compareValue <= rvalue)
										matches = true;
									break;
								case GE:
									if (compareValue >= rvalue)
										matches = true;
									break;
								case LT:
									if (compareValue < rvalue)
										matches = true;
									break;
								case GT:
									if (compareValue > rvalue)
										matches = true;
									break;
								case NE:
									if (compareValue != rvalue)
										matches = true;
									break;
								case EQ:
									if (compareValue == rvalue)
										matches = true;
									break;
								}

								//debugLog("comparing %f %s %f (operatorId = %i) = %i\n", compareValue, operators[o].first.toUtf8(), rvalue, (int)operators[o].second, (int)matches);

								if (!matches) // if a single expression doesn't match, then the whole diff doesn't match
									expressionsMatch = false;

								break;
							}
						}
					}

					break;
				}
			}

			// if this is not an expression, add the token to the literalSearchStrings array
			if (!expression)
			{
				// only add it if it doesn't exist yet
				// this check is only necessary due to multiple redundant parser executions (one per diff!)
				bool exists = false;
				for (size_t l=0; l<literalSearchStrings.size(); l++)
				{
					if (literalSearchStrings[l] == searchStringTokens[i])
					{
						exists = true;
						break;
					}
				}

				if (!exists)
				{
					const UString litAdd = searchStringTokens[i].trim();
					if (litAdd.length() > 0 && !litAdd.isWhitespaceOnly())
						literalSearchStrings.push_back(litAdd);
				}
			}
		}

		if (expressionsMatch) // as soon as one difficulty matches all expressions, we are done here
		{
			expressionMatches = true;
			break;
		}
	}

	// if no diff matched any expression, then we can already stop here
	if (!expressionMatches)
		return false;

	bool hasAnyValidLiteralSearchString = false;
	for (size_t i=0; i<literalSearchStrings.size(); i++)
	{
		if (literalSearchStrings[i].length() > 0)
		{
			hasAnyValidLiteralSearchString = true;
			break;
		}
	}

	// early return here for literal match/contains
	if (hasAnyValidLiteralSearchString)
	{
		for (size_t i=0; i<numDiffs; i++)
		{
			const OsuDatabaseBeatmap *diff = (isContainer ? diffs[i] : databaseBeatmap);

			bool atLeastOneFullMatch = true;

			for (size_t s=0; s<literalSearchStrings.size(); s++)
			{
				if (!findSubstringInDifficulty(diff, literalSearchStrings[s]))
					atLeastOneFullMatch = false;
			}

			// as soon as one diff matches all strings, we are done
			if (atLeastOneFullMatch)
				return true;
		}

		// expression may have matched, but literal didn't match, so the entire beatmap doesn't match
		return false;
	}

	return expressionMatches;
}

bool OsuSongBrowser2::findSubstringInDifficulty(const OsuDatabaseBeatmap *diff, const UString &searchString)
{
	if (diff->getTitle().length() > 0)
	{
		if (diff->getTitle().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getArtist().length() > 0)
	{
		if (diff->getArtist().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getCreator().length() > 0)
	{
		if (diff->getCreator().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getDifficultyName().length() > 0)
	{
		if (diff->getDifficultyName().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getSource().length() > 0)
	{
		if (diff->getSource().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getTags().length() > 0)
	{
		if (diff->getTags().findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getID() > 0)
	{
		if (UString::format("%i", diff->getID()).findIgnoreCase(searchString) != -1)
			return true;
	}

	if (diff->getSetID() > 0)
	{
		if (UString::format("%i", diff->getSetID()).findIgnoreCase(searchString) != -1)
			return true;
	}

	return false;
}

void OsuSongBrowser2::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float uiScale = Osu::ui_scale->getFloat();
	const float dpiScale = Osu::getUIScale(m_osu);

	const int margin = 5 * dpiScale;

	// top bar
	m_fSongSelectTopScale = Osu::getImageScaleToFitResolution(m_osu->getSkin()->getSongSelectTop(), m_osu->getScreenSize());
	const float songSelectTopHeightScaled = std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale, m_songInfo->getMinimumHeight()*1.5f + margin); // NOTE: the height is a heuristic here more or less
	m_fSongSelectTopScale = std::max(m_fSongSelectTopScale, songSelectTopHeightScaled / m_osu->getSkin()->getSongSelectTop()->getHeight());
	m_fSongSelectTopScale *= uiScale; // NOTE: any user osu_ui_scale below 1.0 will break things (because songSelectTop image)

	// topbar left (NOTE: the right side of the std::max() width is commented to keep the scorebrowser width consistent, and because it's not really needed anyway)
	m_topbarLeft->setSize(std::max(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_width_percent.getFloat() + margin, /*m_songInfo->getMinimumWidth() + margin*/0.0f), std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_percent.getFloat(), m_songInfo->getMinimumHeight() + margin));
	m_songInfo->setRelPos(margin, margin);
	m_songInfo->setSize(m_topbarLeft->getSize().x - margin, std::max(m_topbarLeft->getSize().y*0.75f, m_songInfo->getMinimumHeight() + margin));

	const int topbarLeftButtonMargin = 5 * dpiScale;
	const int topbarLeftButtonHeight = 30 * dpiScale;
	const int topbarLeftButtonWidth = 55 * dpiScale;
	for (int i=0; i<m_topbarLeftButtons.size(); i++)
	{
		m_topbarLeftButtons[i]->onResized(); // HACKHACK: framework bug (should update string metrics on setSize())
		m_topbarLeftButtons[i]->setSize(topbarLeftButtonWidth, topbarLeftButtonHeight);
		m_topbarLeftButtons[i]->setRelPos(m_topbarLeft->getSize().x - (i + 1)*(topbarLeftButtonMargin + topbarLeftButtonWidth), m_topbarLeft->getSize().y-m_topbarLeftButtons[i]->getSize().y);
	}

	const int topbarLeftTabButtonMargin = topbarLeftButtonMargin;
	const int topbarLeftTabButtonHeight = topbarLeftButtonHeight;
	const int topbarLeftTabButtonWidth = m_topbarLeft->getSize().x - 3*topbarLeftTabButtonMargin - m_topbarLeftButtons.size()*(topbarLeftButtonWidth + topbarLeftButtonMargin);
	for (int i=0; i<m_topbarLeftTabButtons.size(); i++)
	{
		m_topbarLeftTabButtons[i]->onResized(); // HACKHACK: framework bug (should update string metrics on setSize())
		m_topbarLeftTabButtons[i]->setSize(topbarLeftTabButtonWidth, topbarLeftTabButtonHeight);
		m_topbarLeftTabButtons[i]->setRelPos((topbarLeftTabButtonMargin + i*topbarLeftTabButtonWidth), m_topbarLeft->getSize().y-m_topbarLeftTabButtons[i]->getSize().y);
	}

	m_topbarLeft->update_pos();

	// topbar right
	m_topbarRight->setPosX(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_percent.getFloat());
	m_topbarRight->setSize(m_osu->getScreenWidth() - m_topbarRight->getPos().x, m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_height_percent.getFloat());

	const int topbarRightTabButtonMargin = 10 * dpiScale;
	const int topbarRightTabButtonHeight = 30 * dpiScale;
	const int topbarRightTabButtonWidth = clamp<float>((float)(m_topbarRight->getSize().x - 2*topbarRightTabButtonMargin) / (float)m_topbarRightTabButtons.size(), 0.0f, 200.0f * dpiScale);
	for (int i=0; i<m_topbarRightTabButtons.size(); i++)
	{
		m_topbarRightTabButtons[i]->onResized(); // HACKHACK: framework bug (should update string metrics on setSize())
		m_topbarRightTabButtons[i]->setSize(topbarRightTabButtonWidth, topbarRightTabButtonHeight);
		m_topbarRightTabButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightTabButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightTabButtonWidth), m_topbarRight->getSize().y-m_topbarRightTabButtons[i]->getSize().y);
	}

	if (m_topbarRightTabButtons.size() > 0)
	{
		m_groupLabel->onResized(); // HACKHACK: framework bug (should update string metrics on setSizeToContent())
		m_groupLabel->setSizeToContent(3 * dpiScale);
		m_groupLabel->setRelPos(m_topbarRightTabButtons[0]->getRelPos() + Vector2(-m_groupLabel->getSize().x, m_topbarRightTabButtons[0]->getSize().y/2.0f - m_groupLabel->getSize().y/2.0f));
	}

	const int topbarRightSortButtonMargin = 10 * dpiScale;
	const int topbarRightSortButtonHeight = 30 * dpiScale;
	const int topbarRightSortButtonWidth = clamp<float>((float)(m_topbarRight->getSize().x - 2*topbarRightSortButtonMargin) / (float)m_topbarRightSortButtons.size(), 0.0f, 200.0f * dpiScale);
	for (int i=0; i<m_topbarRightSortButtons.size(); i++)
	{
		m_topbarRightSortButtons[i]->onResized(); // HACKHACK: framework bug (should update string metrics on setSize())
		m_topbarRightSortButtons[i]->setSize(topbarRightSortButtonWidth, topbarRightSortButtonHeight);
		m_topbarRightSortButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightSortButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightSortButtonWidth), topbarRightSortButtonMargin);
	}
	for (int i=0; i<m_topbarRightGroupButtons.size(); i++)
	{
		m_topbarRightGroupButtons[i]->onResized(); // HACKHACK: framework bug (should update string metrics on setSize())
		m_topbarRightGroupButtons[i]->setSize(topbarRightSortButtonWidth, topbarRightSortButtonHeight);
		m_topbarRightGroupButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightSortButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightSortButtonWidth), topbarRightSortButtonMargin);
	}

	if (m_topbarRightGroupButtons.size() > 0)
	{
		m_groupLabel->onResized(); // HACKHACK: framework bug (should update string metrics on setSizeToContent())
		m_groupLabel->setSizeToContent(3 * dpiScale);
		m_groupLabel->setRelPos(m_topbarRightGroupButtons[m_topbarRightGroupButtons.size()-1]->getRelPos() + Vector2(-m_groupLabel->getSize().x, m_topbarRightGroupButtons[m_topbarRightGroupButtons.size()-1]->getSize().y/2.0f - m_groupLabel->getSize().y/2.0f));
	}
	if (m_topbarRightSortButtons.size() > 0)
	{
		m_sortLabel->onResized(); // HACKHACK: framework bug (should update string metrics on setSizeToContent())
		m_sortLabel->setSizeToContent(3 * dpiScale);
		m_sortLabel->setRelPos(m_topbarRightSortButtons[m_topbarRightSortButtons.size()-1]->getRelPos() + Vector2(-m_sortLabel->getSize().x, m_topbarRightSortButtons[m_topbarRightSortButtons.size()-1]->getSize().y/2.0f - m_sortLabel->getSize().y/2.0f));
	}

	m_topbarRight->update_pos();

	// bottombar
	const int bottomBarHeight = m_osu->getScreenHeight()*osu_songbrowser_bottombar_percent.getFloat() * uiScale;

	m_bottombar->setPosY(m_osu->getScreenHeight() - bottomBarHeight);
	m_bottombar->setSize(m_osu->getScreenWidth(), bottomBarHeight);

	// nav bar
	const bool isWidescreen = ((int)(std::max(0, (int)((m_osu->getScreenWidth() - (m_osu->getScreenHeight() * 4.0f / 3.0f)) / 2.0f))) > 0);
	const float navBarXCounter = Osu::getUIScale(m_osu, (isWidescreen ? 140.0f : 120.0f));

	// bottombar cont
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(m_osu->getScreenWidth(), bottomBarHeight);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		const int gap = (i == 1 ? Osu::getUIScale(m_osu, 3.0f) : 0) + (i == 2 ? Osu::getUIScale(m_osu, 2.0f) : 0);

		// new, hardcoded offsets instead of dynamically using the button skin image widths (except starting at 3rd button)
		m_bottombarNavButtons[i]->setRelPosX(navBarXCounter + gap + (i > 0 ? Osu::getUIScale(m_osu, 57.6f) : 0) + (i > 1 ? std::max((i-1)*Osu::getUIScale(m_osu, 48.0f), m_bottombarNavButtons[i-1]->getSize().x) : 0));

		// old, overflows with some skins (e.g. kyu)
		//m_bottombarNavButtons[i]->setRelPosX((i == 0 ? navBarXCounter : 0) + gap + (i > 0 ? m_bottombarNavButtons[i-1]->getRelPos().x + m_bottombarNavButtons[i-1]->getSize().x : 0));
	}

	const int userButtonHeight = m_bottombar->getSize().y*0.9f;
	m_userButton->setSize(userButtonHeight*3.5f, userButtonHeight);
	m_userButton->setRelPos(std::max(m_bottombar->getSize().x/2 - m_userButton->getSize().x/2, m_bottombarNavButtons[m_bottombarNavButtons.size()-1]->getRelPos().x + m_bottombarNavButtons[m_bottombarNavButtons.size()-1]->getSize().x + 10), m_bottombar->getSize().y - m_userButton->getSize().y - 1);

	if (m_ppVersionInfoLabel != NULL)
	{
		m_ppVersionInfoLabel->onResized(); // HACKHACK: framework bug (should update string metrics on setSizeToContent())
		m_ppVersionInfoLabel->setSizeToContent(1, 10);
		m_ppVersionInfoLabel->setRelPos(m_userButton->getRelPos() + Vector2(m_userButton->getSize().x + 20*dpiScale, m_userButton->getSize().y/2 - m_ppVersionInfoLabel->getSize().y/2));
	}

	m_bottombar->update_pos();

	// score browser
	const int scoreBrowserExtraPaddingRight = 5 * dpiScale; // duplication, see below
	updateScoreBrowserLayout();

	// song browser
	m_songBrowser->setPos(m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + 1 + scoreBrowserExtraPaddingRight, m_topbarRight->getPos().y + m_topbarRight->getSize().y + 2);
	m_songBrowser->setSize(m_osu->getScreenWidth() - (m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + scoreBrowserExtraPaddingRight), m_osu->getScreenHeight() - m_songBrowser->getPos().y - m_bottombar->getSize().y + 2);
	updateSongButtonLayout();

	m_search->setPos(m_songBrowser->getPos());
	m_search->setSize(m_songBrowser->getSize());
}

void OsuSongBrowser2::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
}

void OsuSongBrowser2::updateScoreBrowserLayout()
{
	const float dpiScale = Osu::getUIScale(m_osu);

	const bool shouldScoreBrowserBeVisible = (m_osu_scores_enabled->getBool() && osu_songbrowser_scorebrowser_enabled.getBool());
	if (shouldScoreBrowserBeVisible != m_scoreBrowser->isVisible())
		m_scoreBrowser->setVisible(shouldScoreBrowserBeVisible);

	const int scoreBrowserExtraPaddingRight = 5 * dpiScale; // duplication, see above
	const int scoreButtonWidthMax = m_topbarLeft->getSize().x + 2 * dpiScale;
	m_scoreBrowser->setPos(m_topbarLeft->getPos().x - 2 * dpiScale, m_topbarLeft->getPos().y + m_topbarLeft->getSize().y);
	m_scoreBrowser->setSize(scoreButtonWidthMax + scoreBrowserExtraPaddingRight, m_bottombar->getPos().y - (m_topbarLeft->getPos().y + m_topbarLeft->getSize().y) + 2 * dpiScale);
	int scoreHeight = 100;
	{
		Image *menuButtonBackground = m_osu->getSkin()->getMenuButtonBackground();
		Vector2 minimumSize = Vector2(699.0f, 103.0f)*(m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
		float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
		float scoreScale = Osu::getImageScale(m_osu, menuButtonBackground->getSize()*minimumScale, 64.0f);
		scoreScale *= 0.5f;
		scoreHeight = (int)(menuButtonBackground->getHeight()*scoreScale);

		float scale = Osu::getImageScaleToFillResolution(menuButtonBackground, Vector2(scoreButtonWidthMax, scoreHeight));
		scoreHeight = std::max(scoreHeight, (int)(menuButtonBackground->getHeight()*scale));

		// limit to scrollview width (while keeping the aspect ratio)
		const float ratio = minimumSize.x / minimumSize.y;
		if (scoreHeight*ratio > scoreButtonWidthMax)
			scoreHeight = m_scoreBrowser->getSize().x / ratio;
	}
	const std::vector<CBaseUIElement*> &elements = m_scoreBrowser->getContainer()->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		CBaseUIElement *scoreButton = elements[i];
		scoreButton->setSize(m_scoreBrowser->getSize().x, scoreHeight);
		scoreButton->setRelPos(scoreBrowserExtraPaddingRight, i*scoreButton->getSize().y + 5 * dpiScale);
	}
	m_scoreBrowserNoRecordsYetElement->setSize(m_scoreBrowser->getSize().x*0.9f, scoreHeight*0.75f);
	m_scoreBrowserNoRecordsYetElement->setRelPos(m_scoreBrowser->getSize().x/2 - m_scoreBrowserNoRecordsYetElement->getSize().x/2, (m_scoreBrowser->getSize().y/2)*0.65f - m_scoreBrowserNoRecordsYetElement->getSize().y/2);
	m_scoreBrowser->getContainer()->update_pos();
	m_scoreBrowser->setScrollSizeToContent();
}

void OsuSongBrowser2::rebuildScoreButtons()
{
	// reset
	m_scoreBrowser->getContainer()->empty();

	const bool validBeatmap = (m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty2() != NULL);
	const int numScores = (validBeatmap ? ((*m_db->getScores())[m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash()]).size() : 0);

	// top up cache as necessary
	if (numScores > m_scoreButtonCache.size())
	{
		const int numNewButtons = numScores - m_scoreButtonCache.size();
		for (size_t i=0; i<numNewButtons; i++)
		{
			OsuUISongBrowserScoreButton *scoreButton = new OsuUISongBrowserScoreButton(m_osu, m_contextMenu, 0, 0, 0, 0, "");
			scoreButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onScoreClicked) );
			m_scoreButtonCache.push_back(scoreButton);
		}
	}

	// and build the ui
	if (numScores < 1)
		m_scoreBrowser->getContainer()->addBaseUIElement(m_scoreBrowserNoRecordsYetElement, m_scoreBrowserNoRecordsYetElement->getRelPos().x, m_scoreBrowserNoRecordsYetElement->getRelPos().y);
	else
	{
		// sort
		m_db->sortScores(m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash());

		// build
		std::vector<OsuUISongBrowserScoreButton*> scoreButtons;
		for (size_t i=0; i<numScores; i++)
		{
			OsuUISongBrowserScoreButton *button = m_scoreButtonCache[i];
			button->setName(UString(m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash().c_str()));
			button->setScore((*m_db->getScores())[m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash()][i], m_selectedBeatmap->getSelectedDifficulty2(), i+1);
			scoreButtons.push_back(button);
		}

		// add
		for (size_t i=0; i<numScores; i++)
		{
			scoreButtons[i]->setIndex(i+1);
			m_scoreBrowser->getContainer()->addBaseUIElement(scoreButtons[i]);
		}

		// reset
		for (size_t i=0; i<scoreButtons.size(); i++)
		{
			scoreButtons[i]->resetHighlight();
		}
	}

	// layout
	updateScoreBrowserLayout();
}

void OsuSongBrowser2::scheduleSearchUpdate(bool immediately)
{
	m_fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : osu_songbrowser_search_delay.getFloat());
}

void OsuSongBrowser2::checkHandleKillBackgroundStarCalculator()
{
	if (!m_backgroundStarCalculator->isDead())
	{
		m_backgroundStarCalculator->kill();

		const double startTime = engine->getTimeReal();
		while (!m_backgroundStarCalculator->isAsyncReady())
		{
			if (engine->getTimeReal() - startTime > 2)
			{
				debugLog("WARNING: Ignoring stuck BackgroundStarCalculator thread!\n");
				break;
			}
		}
	}
}

bool OsuSongBrowser2::checkHandleKillDynamicStarCalculator(bool timeout)
{
	if (!m_dynamicStarCalculator->isDead())
	{
		m_dynamicStarCalculator->kill();

		if (!timeout)
		{
			const double startTime = engine->getTimeReal();
			while (!m_dynamicStarCalculator->isAsyncReady())
			{
				env->sleep(1);

				if (engine->getTimeReal() - startTime > 20.0)
				{
					debugLog("WARNING: Ignoring stuck DynamicStarCalculator thread!\n");
					break;
				}
			}
		}

		return m_dynamicStarCalculator->isAsyncReady();
	}
	else
		return (!engine->getResourceManager()->isLoadingResource(m_dynamicStarCalculator) || m_dynamicStarCalculator->isAsyncReady());
}

void OsuSongBrowser2::checkHandleKillBackgroundSearchMatcher()
{
	if (!m_backgroundSearchMatcher->isDead())
	{
		m_backgroundSearchMatcher->kill();

		const double startTime = engine->getTimeReal();
		while (!m_backgroundSearchMatcher->isAsyncReady())
		{
			if (engine->getTimeReal() - startTime > 2)
			{
				debugLog("WARNING: Ignoring stuck SearchMatcher thread!\n");
				break;
			}
		}
	}
}

OsuUISelectionButton *OsuSongBrowser2::addBottombarNavButton(std::function<OsuSkinImage*()> getImageFunc, std::function<OsuSkinImage*()> getImageOverFunc)
{
	OsuUISelectionButton *btn = new OsuUISelectionButton(getImageFunc, getImageOverFunc, 0, 0, 0, 0, "");
	m_bottombar->addBaseUIElement(btn);
	m_bottombarNavButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightTabButton(UString text)
{
	// sanity check
	{
		bool isValid = false;
		for (size_t i=0; i<m_groupings.size(); i++)
		{
			if (m_groupings[i].name == text)
			{
				isValid = true;
				break;
			}
		}

		if (!isValid)
			engine->showMessageError("Error", UString::format("Invalid grouping name for tab button \"%s\"!", text.toUtf8()));
	}

	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightTabButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightGroupButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightGroupButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightSortButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightSortButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarLeftTabButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarLeft->addBaseUIElement(btn);
	m_topbarLeftTabButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarLeftButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarLeft->addBaseUIElement(btn);
	m_topbarLeftButtons.push_back(btn);
	return btn;
}

void OsuSongBrowser2::onDatabaseLoadingFinished()
{
	m_beatmaps = std::vector<OsuDatabaseBeatmap*>(m_db->getDatabaseBeatmaps()); // having a copy of the vector in here is actually completely unnecessary

	debugLog("OsuSongBrowser2::onDatabaseLoadingFinished() : %i beatmaps.\n", m_beatmaps.size());

	// initialize all collection (grouped) buttons
	{
		// artist
		{
			// 0-9
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "0-9", std::vector<OsuUISongBrowserButton*>());
				m_artistCollectionButtons.push_back(b);
			}

			// A-Z
			for (size_t i=0; i<26; i++)
			{
				UString artistCollectionName = UString::format("%c", 'A' + i);

				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", artistCollectionName, std::vector<OsuUISongBrowserButton*>());
				m_artistCollectionButtons.push_back(b);
			}

			// Other
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Other", std::vector<OsuUISongBrowserButton*>());
				m_artistCollectionButtons.push_back(b);
			}
		}

		// difficulty
		for (size_t i=0; i<12; i++)
		{
			UString difficultyCollectionName = UString::format(i == 1 ? "%i star" : "%i stars", i);
			if (i < 1)
				difficultyCollectionName = "Below 1 star";
			if (i > 10)
				difficultyCollectionName = "Above 10 stars";

			std::vector<OsuUISongBrowserButton*> children;

			OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", difficultyCollectionName, children);
			m_difficultyCollectionButtons.push_back(b);
		}

		// bpm
		{
			OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 60 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 120 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 180 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 240 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 300 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Over 300 BPM", std::vector<OsuUISongBrowserButton*>());
			m_bpmCollectionButtons.push_back(b);
		}

		// creator
		{
			// 0-9
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "0-9", std::vector<OsuUISongBrowserButton*>());
				m_creatorCollectionButtons.push_back(b);
			}

			// A-Z
			for (size_t i=0; i<26; i++)
			{
				UString artistCollectionName = UString::format("%c", 'A' + i);

				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", artistCollectionName, std::vector<OsuUISongBrowserButton*>());
				m_creatorCollectionButtons.push_back(b);
			}

			// Other
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Other", std::vector<OsuUISongBrowserButton*>());
				m_creatorCollectionButtons.push_back(b);
			}
		}

		// dateadded
		{
			// TODO: annoying
		}

		// length
		{
			OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "1 minute or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "2 minutes or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "3 minutes or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "4 minutes or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "5 minutes or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "10 minutes or less", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
			b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Over 10 minutes", std::vector<OsuUISongBrowserButton*>());
			m_lengthCollectionButtons.push_back(b);
		}

		// title
		{
			// 0-9
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "0-9", std::vector<OsuUISongBrowserButton*>());
				m_titleCollectionButtons.push_back(b);
			}

			// A-Z
			for (size_t i=0; i<26; i++)
			{
				UString artistCollectionName = UString::format("%c", 'A' + i);

				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", artistCollectionName, std::vector<OsuUISongBrowserButton*>());
				m_titleCollectionButtons.push_back(b);
			}

			// Other
			{
				OsuUISongBrowserCollectionButton *b = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Other", std::vector<OsuUISongBrowserButton*>());
				m_titleCollectionButtons.push_back(b);
			}
		}
	}

	// add all beatmaps (build buttons)
	for (size_t i=0; i<m_beatmaps.size(); i++)
	{
		addBeatmap(m_beatmaps[i]);
	}

	// build collections
	recreateCollectionsButtons();

	onSortChange(osu_songbrowser_sortingtype.getString());
	onSortScoresChange(osu_songbrowser_scores_sortingtype.getString());

	// update rich presence (discord total pp)
	OsuRichPresence::onSongBrowser(m_osu);

	// update user name/stats
	onUserButtonChange(m_name_ref->getString(), -1);

	if (osu_songbrowser_search_hardcoded_filter.getString().length() > 0)
		onSearchUpdate();
}

void OsuSongBrowser2::onSearchUpdate()
{
	const bool hasHardcodedSearchStringChanged = (m_sPrevHardcodedSearchString != osu_songbrowser_search_hardcoded_filter.getString());
	const bool hasSearchStringChanged = (m_sPrevSearchString != m_sSearchString);

	const bool prevInSearch = m_bInSearch;
	m_bInSearch = (m_sSearchString.length() > 0 || osu_songbrowser_search_hardcoded_filter.getString().length() > 0);
	const bool hasInSearchChanged = (prevInSearch != m_bInSearch);

	if (m_bInSearch)
	{
		m_searchPrevGroup = m_group;

		// flag all search matches across entire database
		if (hasSearchStringChanged || hasHardcodedSearchStringChanged || hasInSearchChanged)
		{
			// stop potentially running async search
			checkHandleKillBackgroundSearchMatcher();

			m_backgroundSearchMatcher->revive();
			m_backgroundSearchMatcher->release();
			m_backgroundSearchMatcher->setSongButtonsAndSearchString(m_songButtons, m_sSearchString, osu_songbrowser_search_hardcoded_filter.getString());

			engine->getResourceManager()->requestNextLoadAsync();
			engine->getResourceManager()->loadResource(m_backgroundSearchMatcher);
		}
		else
			rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);

		// (results are handled in update() once available)
	}
	else // exit search
	{
		// exiting the search does not need any async work, so we can just directly do it in here

		// stop potentially running async search
		checkHandleKillBackgroundSearchMatcher();

		// reset container and visible buttons list
		m_songBrowser->getContainer()->empty();
		m_visibleSongButtons.clear();

		// reset all search flags
		for (size_t i=0; i<m_songButtons.size(); i++)
		{
			const std::vector<OsuUISongBrowserButton*> &children = m_songButtons[i]->getChildren();
			if (children.size() > 0)
			{
				for (size_t c=0; c<children.size(); c++)
				{
					children[c]->setIsSearchMatch(true);
				}
			}
			else
				m_songButtons[i]->setIsSearchMatch(true);
		}

		// remember which tab was selected, instead of defaulting back to no grouping
		// this also rebuilds the visible buttons list
		for (size_t i=0; i<m_groupings.size(); i++)
		{
			if (m_groupings[i].type == m_searchPrevGroup)
				onGroupChange("", m_groupings[i].id);
		}
	}

	m_sPrevSearchString = m_sSearchString;
	m_sPrevHardcodedSearchString = osu_songbrowser_search_hardcoded_filter.getString();
}

void OsuSongBrowser2::rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop, bool doRebuildSongButtons)
{
	// reset container and visible buttons list
	m_songBrowser->getContainer()->empty();
	m_visibleSongButtons.clear();

	// use flagged search matches to rebuild visible song buttons
	{
		if (m_group == GROUP::GROUP_NO_GROUPING)
		{
			for (size_t i=0; i<m_songButtons.size(); i++)
			{
				const std::vector<OsuUISongBrowserButton*> &children = m_songButtons[i]->getChildren();
				if (children.size() > 0)
				{
					// if all children match, then we still want to display the parent wrapper button (without expanding all diffs)
					bool allChildrenMatch = true;
					for (size_t c=0; c<children.size(); c++)
					{
						if (!children[c]->isSearchMatch())
							allChildrenMatch = false;
					}

					if (allChildrenMatch)
						m_visibleSongButtons.push_back(m_songButtons[i]);
					else
					{
						// rip matching children from parent
						for (size_t c=0; c<children.size(); c++)
						{
							if (children[c]->isSearchMatch())
								m_visibleSongButtons.push_back(children[c]);
						}
					}
				}
				else if (m_songButtons[i]->isSearchMatch())
					m_visibleSongButtons.push_back(m_songButtons[i]);
			}
		}
		else
		{
			std::vector<OsuUISongBrowserCollectionButton*> *groupButtons = NULL;
			{
				switch (m_group)
				{
				case GROUP::GROUP_NO_GROUPING:
					break;
				case GROUP::GROUP_ARTIST:
					groupButtons = &m_artistCollectionButtons;
					break;
				case GROUP::GROUP_CREATOR:
					groupButtons = &m_creatorCollectionButtons;
					break;
				case GROUP::GROUP_DIFFICULTY:
					groupButtons = &m_difficultyCollectionButtons;
					break;
				case GROUP::GROUP_LENGTH:
					groupButtons = &m_lengthCollectionButtons;
					break;
				case GROUP::GROUP_TITLE:
					groupButtons = &m_titleCollectionButtons;
					break;
				case GROUP::GROUP_COLLECTIONS:
					groupButtons = &m_collectionButtons;
					break;
				}
			}

			if (groupButtons != NULL)
			{
				for (size_t i=0; i<groupButtons->size(); i++)
				{
					bool isAnyMatchInGroup = false;

					const std::vector<OsuUISongBrowserButton*> &children = (*groupButtons)[i]->getChildren();
					for (size_t c=0; c<children.size(); c++)
					{
						const std::vector<OsuUISongBrowserButton*> &childrenChildren = children[c]->getChildren();
						if (childrenChildren.size() > 0)
						{
							for (size_t cc=0; cc<childrenChildren.size(); cc++)
							{
								if (childrenChildren[cc]->isSearchMatch())
								{
									isAnyMatchInGroup = true;
									break;
								}
							}

							if (isAnyMatchInGroup)
								break;
						}
						else if (children[c]->isSearchMatch())
						{
							isAnyMatchInGroup = true;
							break;
						}
					}

					if (isAnyMatchInGroup || !m_bInSearch)
						m_visibleSongButtons.push_back((*groupButtons)[i]);
				}
			}
		}

		if (doRebuildSongButtons)
			rebuildSongButtons();

		// scroll to top search result, or auto select the only result
		if (scrollToTop)
		{
			if (m_visibleSongButtons.size() > 1)
				scrollToSongButton(m_visibleSongButtons[0]);
			else if (m_visibleSongButtons.size() > 0)
			{
				selectSongButton(m_visibleSongButtons[0]);
				m_songBrowser->scrollY(1);
			}
		}
	}
}

void OsuSongBrowser2::onSortScoresClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(button->getPos());
	m_contextMenu->setRelPos(button->getRelPos());
	m_contextMenu->begin(button->getSize().x);
	{
		const std::vector<OsuDatabase::SCORE_SORTING_METHOD> &scoreSortingMethods = m_db->getScoreSortingMethods();
		for (size_t i=0; i<scoreSortingMethods.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(scoreSortingMethods[i].name);
			if (scoreSortingMethods[i].name == osu_songbrowser_scores_sortingtype.getString())
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortScoresChange) );
}

void OsuSongBrowser2::onSortScoresChange(UString text, int id)
{
	osu_songbrowser_scores_sortingtype.setValue(text); // NOTE: remember
	m_scoreSortButton->setText(text);
	rebuildScoreButtons();
	m_scoreBrowser->scrollToTop();

	// update grades of all visible songdiffbuttons
	if (m_selectedBeatmap != NULL)
	{
		for (size_t i=0; i<m_visibleSongButtons.size(); i++)
		{
			if (m_visibleSongButtons[i]->getDatabaseBeatmap() == m_selectedBeatmap->getSelectedDifficulty2())
			{
				OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(m_visibleSongButtons[i]);
				if (songButtonPointer != NULL)
				{
					for (OsuUISongBrowserButton *diffButton : songButtonPointer->getChildren())
					{
						OsuUISongBrowserSongButton *diffButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(diffButton);
						if (diffButtonPointer != NULL)
							diffButtonPointer->updateGrade();
					}
				}
			}
		}
	}
}

void OsuSongBrowser2::onWebClicked(CBaseUIButton *button)
{
	if (m_songInfo->getBeatmapID() > 0)
	{
		env->openURLInDefaultBrowser(UString::format("https://osu.ppy.sh/b/%ld", m_songInfo->getBeatmapID()));
		m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	}
}

void OsuSongBrowser2::onGroupClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(button->getPos());
	m_contextMenu->setRelPos(button->getRelPos());
	m_contextMenu->begin(button->getSize().x);
	{
		for (size_t i=0; i<m_groupings.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(m_groupings[i].name, m_groupings[i].id);
			if (m_groupings[i].type == m_group)
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupChange) );
}

void OsuSongBrowser2::onGroupChange(UString text, int id)
{
	GROUPING *grouping = (m_groupings.size() > 0 ? &m_groupings[0] : NULL);
	for (size_t i=0; i<m_groupings.size(); i++)
	{
		if (m_groupings[i].id == id || (text.length() > 1 && m_groupings[i].name == text))
		{
			grouping = &m_groupings[i];
			break;
		}
	}
	if (grouping == NULL) return;

	const Color highlightColor = COLOR(255, 0, 255, 0);
	const Color defaultColor = COLOR(255, 255, 255, 255);

	// update group combobox button text
	m_groupButton->setText(grouping->name);

	// highlight current tab (if any tab button exists for group)
	bool hasTabButton = false;
	for (size_t i=0; i<m_topbarRightTabButtons.size(); i++)
	{
		if (m_topbarRightTabButtons[i]->getText() == grouping->name)
		{
			hasTabButton = true;
			m_topbarRightTabButtons[i]->setTextBrightColor(highlightColor);
		}
		else
			m_topbarRightTabButtons[i]->setTextBrightColor(defaultColor);
	}

	// if there is no tab button for this group, then highlight combobox button instead
	m_groupButton->setTextBrightColor(hasTabButton ? defaultColor : highlightColor);

	// and update the actual songbrowser contents
	switch (grouping->type)
	{
	case GROUP::GROUP_NO_GROUPING:
		onGroupNoGrouping();
		break;
	case GROUP::GROUP_ARTIST:
		onGroupArtist();
		break;
	case GROUP::GROUP_BPM:
		onGroupBPM();
		break;
	case GROUP::GROUP_CREATOR:
		onGroupCreator();
		break;
	case GROUP::GROUP_DATEADDED:
		onGroupDateadded();
		break;
	case GROUP::GROUP_DIFFICULTY:
		onGroupDifficulty();
		break;
	case GROUP::GROUP_LENGTH:
		onGroupLength();
		break;
	case GROUP::GROUP_TITLE:
		onGroupTitle();
		break;
	case GROUP::GROUP_COLLECTIONS:
		onGroupCollections();
		break;
	}
}

void OsuSongBrowser2::onSortClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(button->getPos());
	m_contextMenu->setRelPos(button->getRelPos());
	m_contextMenu->begin(button->getSize().x);
	{
		for (size_t i=0; i<m_sortingMethods.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(m_sortingMethods[i].name);
			if (m_sortingMethods[i].type == m_sortingMethod)
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortChange) );

	// NOTE: don't remember group setting on shutdown

	// manual hack for small resolutions
	if (m_contextMenu->getRelPos().x + m_contextMenu->getSize().x > m_topbarRight->getSize().x)
	{
		int newRelPosX = m_topbarRight->getSize().x - m_contextMenu->getSize().x - 1;
		m_contextMenu->setRelPosX(newRelPosX);
		m_contextMenu->setPosX(m_topbarRight->getPos().x + m_topbarRight->getSize().x - m_contextMenu->getSize().x - 1);
	}
}

void OsuSongBrowser2::onSortChange(UString text, int id)
{
	onSortChangeInt(text, true);
}

void OsuSongBrowser2::onSortChangeInt(UString text, bool autoScroll)
{
	SORTING_METHOD *sortingMethod = (m_sortingMethods.size() > 3 ? &m_sortingMethods[3] : NULL);
	for (size_t i=0; i<m_sortingMethods.size(); i++)
	{
		if (m_sortingMethods[i].name == text)
		{
			sortingMethod = &m_sortingMethods[i];
			break;
		}
	}
	if (sortingMethod == NULL) return;

	m_sortingMethod = sortingMethod->type;
	m_sortButton->setText(sortingMethod->name);

	osu_songbrowser_sortingtype.setValue(sortingMethod->name); // NOTE: remember persistently

	struct COMPARATOR_WRAPPER
	{
		SORTING_COMPARATOR *comp;
		bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
		{
			return comp->operator()(a, b);
		}
	};
	COMPARATOR_WRAPPER comparatorWrapper;
	comparatorWrapper.comp = sortingMethod->comparator;

	// resort primitive master button array (all songbuttons, No Grouping)
	std::sort(m_songButtons.begin(), m_songButtons.end(), comparatorWrapper);

	// resort Collection buttons (one button for each collection)
	// these are always sorted alphabetically by name
	{
		struct COLLECTION_NAME_SORTING_COMPARATOR
		{
			bool operator () (OsuUISongBrowserCollectionButton const *a, OsuUISongBrowserCollectionButton const *b)
			{
				// strict weak ordering!
				if (a->getCollectionName() == b->getCollectionName())
					return a->getSortHack() < b->getSortHack();

				return a->getCollectionName().lessThanIgnoreCase(b->getCollectionName());
			}
		};

		std::sort(m_collectionButtons.begin(), m_collectionButtons.end(), COLLECTION_NAME_SORTING_COMPARATOR());
	}

	// resort Collection button array (each group of songbuttons inside each Collection)
	for (size_t i=0; i<m_collectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_collectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_collectionButtons[i]->setChildren(children);
	}

	// etc.
	for (size_t i=0; i<m_artistCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_artistCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_artistCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_difficultyCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_difficultyCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_difficultyCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_bpmCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_bpmCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_bpmCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_creatorCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_creatorCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_creatorCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_dateaddedCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_dateaddedCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_dateaddedCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_lengthCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_lengthCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_lengthCollectionButtons[i]->setChildren(children);
	}
	for (size_t i=0; i<m_titleCollectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> &children = m_titleCollectionButtons[i]->getChildren();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_titleCollectionButtons[i]->setChildren(children);
	}

	// we only need to update the visible buttons array if we are in No Grouping (because Collections always get sorted by the collection name on the first level)
	if (m_group == GROUP::GROUP_NO_GROUPING)
	{
		// HACKHACK: TODO: this needs to happen to support correct sorting, but using this requires proper regrouping logic (with memory allocations every time)
		/*
		std::vector<OsuUISongBrowserSongButton*> allDiffButtons;
		for (size_t i=0; i<m_songButtons.size(); i++)
		{
			OsuUISongBrowserSongButton *button = m_songButtons[i];
			const std::vector<OsuUISongBrowserButton*> &children = button->getChildren();
			if (children.size() < 1)
				allDiffButtons.push_back(button);
			else
			{
				for (size_t c=0; c<children.size(); c++)
				{
					allDiffButtons.push_back((OsuUISongBrowserSongButton*)children[c]);
				}
			}
		}
		std::sort(allDiffButtons.begin(), allDiffButtons.end(), comparatorWrapper);

		m_visibleSongButtons.clear();
		m_visibleSongButtons.insert(m_visibleSongButtons.end(), allDiffButtons.begin(), allDiffButtons.end());
		*/



		m_visibleSongButtons.clear();
		m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_songButtons.begin(), m_songButtons.end());
	}

	rebuildSongButtons();
	onAfterSortingOrGroupChange(autoScroll);
}

void OsuSongBrowser2::onGroupTabButtonClicked(CBaseUIButton *groupTabButton)
{
	onGroupChange(groupTabButton->getText());
}

void OsuSongBrowser2::onGroupNoGrouping()
{
	m_group = GROUP::GROUP_NO_GROUPING;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_songButtons.begin(), m_songButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupCollections(bool autoScroll)
{
	m_group = GROUP::GROUP_COLLECTIONS;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_collectionButtons.begin(), m_collectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange(autoScroll);
}

void OsuSongBrowser2::onGroupArtist()
{
	m_group = GROUP::GROUP_ARTIST;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_artistCollectionButtons.begin(), m_artistCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupDifficulty()
{
	m_group = GROUP::GROUP_DIFFICULTY;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_difficultyCollectionButtons.begin(), m_difficultyCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupBPM()
{
	m_group = GROUP::GROUP_BPM;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_bpmCollectionButtons.begin(), m_bpmCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupCreator()
{
	m_group = GROUP::GROUP_CREATOR;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_creatorCollectionButtons.begin(), m_creatorCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupDateadded()
{
	m_group = GROUP::GROUP_DATEADDED;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_dateaddedCollectionButtons.begin(), m_dateaddedCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupLength()
{
	m_group = GROUP::GROUP_LENGTH;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_lengthCollectionButtons.begin(), m_lengthCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onGroupTitle()
{
	m_group = GROUP::GROUP_TITLE;

	m_visibleSongButtons.clear();
	m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_titleCollectionButtons.begin(), m_titleCollectionButtons.end());

	rebuildSongButtons();
	onAfterSortingOrGroupChange();
}

void OsuSongBrowser2::onAfterSortingOrGroupChange(bool autoScroll)
{
	// keep search state consistent between tab changes
	if (m_bInSearch)
		onSearchUpdate();

	// (can't call it right here because we maybe have async)
	m_bOnAfterSortingOrGroupChangeUpdateScheduledAutoScroll = autoScroll;
	m_bOnAfterSortingOrGroupChangeUpdateScheduled = true;
}

void OsuSongBrowser2::onAfterSortingOrGroupChangeUpdateInt(bool autoScroll)
{
	// if anything was selected, scroll to that. otherwise scroll to top
	const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
	bool isAnythingSelected = false;
	for (size_t i=0; i<elements.size(); i++)
	{
		const OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
		if (button != NULL && button->isSelected())
		{
			isAnythingSelected = true;
			break;
		}
	}

	if (autoScroll)
	{
		if (isAnythingSelected)
			scrollToSelectedSongButton();
		else
			m_songBrowser->scrollToTop();
	}
}

void OsuSongBrowser2::onSelectionMode()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_contextMenu->setPos(m_bottombarNavButtons[0]->getPos());
	m_contextMenu->setRelPos(m_bottombarNavButtons[0]->getRelPos());
	m_contextMenu->begin(0, true);
	{
		OsuUIContextMenuButton *standardButton = m_contextMenu->addButton("Standard", 0);
		standardButton->setTextLeft(false);
		standardButton->setTooltipText("Standard 2D circle clicking.");
		//CBaseUIButton *maniaButton = m_contextMenu->addButton("Mania", 1);
		//maniaButton->setTextLeft(false);
		OsuUIContextMenuButton *fposuButton = m_contextMenu->addButton("FPoSu", 2);
		fposuButton->setTextLeft(false);
		fposuButton->setTooltipText("The real 3D FPS mod.\nPlay from a first person shooter perspective in a 3D environment.\nThis is only intended for mouse! (Enable \"Tablet/Absolute Mode\" for tablets.)");

		CBaseUIButton *activeButton = NULL;
		{
			if (m_osu_mod_fposu_ref->getBool())
				activeButton = fposuButton;
			else if (m_osu->getGamemode() == Osu::GAMEMODE::STD)
				activeButton = standardButton;
			//else if (m_osu->getGamemode() == Osu::GAMEMODE::MANIA)
			//	activeButton = maniaButton;
		}
		if (activeButton != NULL)
			activeButton->setTextBrightColor(0xff00ff00);

	}
	m_contextMenu->setPos(m_contextMenu->getPos() - Vector2((m_contextMenu->getSize().x - m_bottombarNavButtons[0]->getSize().x)/2.0f, m_contextMenu->getSize().y));
	m_contextMenu->setRelPos(m_contextMenu->getRelPos() - Vector2((m_contextMenu->getSize().x - m_bottombarNavButtons[0]->getSize().x)/2.0f, m_contextMenu->getSize().y));
	m_contextMenu->end(true, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onModeChange2) );
}

void OsuSongBrowser2::onSelectionMods()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleModSelection(m_bF1Pressed);
}

void OsuSongBrowser2::onSelectionRandom()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	if (m_bShiftPressed)
		m_bPreviousRandomBeatmapScheduled = true;
	else
		m_bRandomBeatmapScheduled = true;
}

void OsuSongBrowser2::onSelectionOptions()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	OsuUISongBrowserButton *currentlySelectedSongButton = findCurrentlySelectedSongButton();
	if (currentlySelectedSongButton != NULL)
	{
		scrollToSongButton(currentlySelectedSongButton);

		const Vector2 heuristicSongButtonPositionAfterSmoothScrollFinishes = (m_songBrowser->getPos() + m_songBrowser->getSize()/2);

		OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(currentlySelectedSongButton);
		OsuUISongBrowserCollectionButton *collectionButtonPointer = dynamic_cast<OsuUISongBrowserCollectionButton*>(currentlySelectedSongButton);
		if (songButtonPointer != NULL)
			songButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
		else if (collectionButtonPointer != NULL)
			collectionButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
	}
}

void OsuSongBrowser2::onModeChange(UString text)
{
	onModeChange2(text);
}

void OsuSongBrowser2::onModeChange2(UString text, int id)
{
	if (id != 2 && text != "fposu")
		m_osu_mod_fposu_ref->setValue(0.0f);

	if (id == 0 || text == "std")
	{
		if (m_osu->getGamemode() != Osu::GAMEMODE::STD)
		{
			m_osu->setGamemode(Osu::GAMEMODE::STD);
			refreshBeatmaps();
		}
	}
	else if (id == 1 || text == "mania")
	{
		if (m_osu->getGamemode() != Osu::GAMEMODE::MANIA)
		{
			m_osu->setGamemode(Osu::GAMEMODE::MANIA);
			refreshBeatmaps();
		}
	}
	else if (id == 2 || text == "fposu")
	{
		m_osu_mod_fposu_ref->setValue(1.0f);

		if (m_osu->getGamemode() != Osu::GAMEMODE::STD)
		{
			m_osu->setGamemode(Osu::GAMEMODE::STD);
			refreshBeatmaps();
		}
	}
}

void OsuSongBrowser2::onUserButtonClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	std::vector<UString> names = m_db->getPlayerNamesWithScoresForUserSwitcher();
	if (names.size() > 0)
	{
		m_contextMenu->setPos(m_userButton->getPos());
		m_contextMenu->setRelPos(m_userButton->getPos());
		m_contextMenu->begin(m_userButton->getSize().x);
		m_contextMenu->addButton("Switch User:", 0)->setTextColor(0xff888888)->setTextDarkColor(0xff000000)->setTextLeft(false)->setEnabled(false);
		//m_contextMenu->addButton("", 0)->setEnabled(false);
		for (size_t i=0; i<names.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(names[i]);
			if (names[i] == m_name_ref->getString())
				button->setTextBrightColor(0xff00ff00);
		}
		m_contextMenu->addButton("", 0)->setEnabled(false);
		m_contextMenu->addButton(">>> Top Ranks <<<", 1)->setTextLeft(false);
		m_contextMenu->addButton("", 0)->setEnabled(false);
		m_contextMenu->setPos(m_contextMenu->getPos() - Vector2(0, m_contextMenu->getSize().y));
		m_contextMenu->setRelPos(m_contextMenu->getRelPos() - Vector2(0, m_contextMenu->getSize().y));
		m_contextMenu->end(true, true);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onUserButtonChange) );
		OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
	}
}

void OsuSongBrowser2::onUserButtonChange(UString text, int id)
{
	if (id == 0) return;

	if (id == 1)
	{
		m_osu->toggleUserStatsScreen();
		return;
	}

	m_name_ref->setValue(text);
	m_osu->getOptionsMenu()->setUsername(text); // NOTE: force update options textbox to avoid shutdown inconsistency
	m_userButton->setText(text);

	m_userButton->updateUserStats();
}

void OsuSongBrowser2::onScoreClicked(CBaseUIButton *button)
{
	OsuUISongBrowserScoreButton *scoreButton = (OsuUISongBrowserScoreButton*)button;

	// NOTE: the order of these two calls matters (score data overwrites relevant fields, but base values are coming from the beatmap)
	m_osu->getRankingScreen()->setBeatmapInfo(m_selectedBeatmap, m_selectedBeatmap->getSelectedDifficulty2());
	m_osu->getRankingScreen()->setScore(scoreButton->getScore(), scoreButton->getDateTime());

	m_osu->getSongBrowser()->setVisible(false);
	m_osu->getRankingScreen()->setVisible(true);
}

void OsuSongBrowser2::onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, int id)
{
	// NOTE: see OsuUISongBrowserScoreButton::onContextMenu()

	if (id == 2)
	{
		m_db->deleteScore(std::string(scoreButton->getName().toUtf8()), scoreButton->getScoreUnixTimestamp());

		rebuildScoreButtons();
		m_userButton->updateUserStats();
	}
}

void OsuSongBrowser2::onSongButtonContextMenu(OsuUISongBrowserSongButton *songButton, UString text, int id)
{
	//debugLog("OsuSongBrowser2::onSongButtonContextMenu(%p, %s, %i)\n", songButton, text.toUtf8(), id);

	struct CollectionManagementHelper
	{
		static std::vector<std::string> getBeatmapSetHashesForSongButton(OsuUISongBrowserSongButton *songButton, OsuDatabase *db)
		{
			std::vector<std::string> beatmapSetHashes;
			{
				const std::vector<OsuUISongBrowserButton*> &songButtonChildren = songButton->getChildren();
				if (songButtonChildren.size() > 0)
				{
					for (size_t i=0; i<songButtonChildren.size(); i++)
					{
						beatmapSetHashes.push_back(songButtonChildren[i]->getDatabaseBeatmap()->getMD5Hash());
					}
				}
				else
				{
					const OsuDatabaseBeatmap *beatmap = db->getBeatmap(songButton->getDatabaseBeatmap()->getMD5Hash());
					if (beatmap != NULL)
					{
						const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
						for (size_t i=0; i<diffs.size(); i++)
						{
							beatmapSetHashes.push_back(diffs[i]->getMD5Hash());
						}
					}
				}
			}
			return beatmapSetHashes;
		}
	};

	bool updateUIScheduled = false;
	{
		if (id == 1)
		{
			// add diff to collection

			m_db->addBeatmapToCollection(text, songButton->getDatabaseBeatmap()->getMD5Hash());

			updateUIScheduled = true;
		}
		else if (id == 2)
		{
			// add set to collection

			const std::vector<std::string> beatmapSetHashes = CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
			for (size_t i=0; i<beatmapSetHashes.size(); i++)
			{
				m_db->addBeatmapToCollection(text, beatmapSetHashes[i], false); // (don't save here every time) (this will ignore already added parts of the set, so nothing to worry about)
			}
			m_db->triggerSaveCollections(); // (but do save here once at the end)

			updateUIScheduled = true;
		}
		else if (id == 3)
		{
			// remove diff from collection

			// get collection name by selection
			UString collectionName;
			{
				for (size_t i=0; i<m_collectionButtons.size(); i++)
				{
					if (m_collectionButtons[i]->isSelected())
					{
						collectionName = m_collectionButtons[i]->getCollectionName();
						break;
					}
				}
			}

			m_db->removeBeatmapFromCollection(collectionName, songButton->getDatabaseBeatmap()->getMD5Hash());

			updateUIScheduled = true;
		}
		else if (id == 4)
		{
			// remove entire set from collection

			// get collection name by selection
			UString collectionName;
			{
				for (size_t i=0; i<m_collectionButtons.size(); i++)
				{
					if (m_collectionButtons[i]->isSelected())
					{
						collectionName = m_collectionButtons[i]->getCollectionName();
						break;
					}
				}
			}

			const std::vector<std::string> beatmapSetHashes = CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
			for (size_t i=0; i<beatmapSetHashes.size(); i++)
			{
				m_db->removeBeatmapFromCollection(collectionName, beatmapSetHashes[i], false); // (don't save here every time)
			}
			m_db->triggerSaveCollections(); // (but do save here once at the end)

			updateUIScheduled = true;
		}
		else if (id == -2 || id == -4)
		{
			// add new collection with name text

			if (!m_db->addCollection(text))
				m_osu->getNotificationOverlay()->addNotification("Error: Collection name already exists or is invalid.", 0xffffff00);
			else
			{
				if (id == -2)
				{
					// id == -2 means add diff to the just-created new collection

					m_db->addBeatmapToCollection(text, songButton->getDatabaseBeatmap()->getMD5Hash());

					updateUIScheduled = true;
				}
				else if (id == -4)
				{
					// id == -4 means add set to the just-created new collection

					const std::vector<std::string> beatmapSetHashes = CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
					for (size_t i=0; i<beatmapSetHashes.size(); i++)
					{
						m_db->addBeatmapToCollection(text, beatmapSetHashes[i], false); // (don't save here every time)
					}
					m_db->triggerSaveCollections(); // (but do save here once at the end)

					updateUIScheduled = true;
				}
			}
		}
	}

	if (updateUIScheduled)
	{
		const float prevScrollPosY = m_songBrowser->getScrollPosY(); // usability
		const UString previouslySelectedCollectionName = (m_selectionPreviousCollectionButton != NULL ? m_selectionPreviousCollectionButton->getCollectionName() : ""); // usability
		{
			recreateCollectionsButtons();
			rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(false, false);	// (last false = skipping rebuildSongButtons() here)
			onSortChangeInt(osu_songbrowser_sortingtype.getString(), false);				// (because this does the rebuildSongButtons())
		}
		if (previouslySelectedCollectionName.length() > 0)
		{
			for (size_t i=0; i<m_collectionButtons.size(); i++)
			{
				if (m_collectionButtons[i]->getCollectionName() == previouslySelectedCollectionName)
				{
					m_collectionButtons[i]->select();
					m_songBrowser->scrollToY(prevScrollPosY, false);
					break;
				}
			}
		}
	}
}

void OsuSongBrowser2::onCollectionButtonContextMenu(OsuUISongBrowserCollectionButton *collectionButton, UString text, int id)
{
	if (id == 2) // delete collection
	{
		for (size_t i=0; i<m_collectionButtons.size(); i++)
		{
			if (m_collectionButtons[i]->getCollectionName() == text)
			{
				// delete UI
				delete m_collectionButtons[i];
				m_collectionButtons.erase(m_collectionButtons.begin() + i);

				// reset UI state
				m_selectionPreviousCollectionButton = NULL;

				m_db->deleteCollection(text);

				// update UI
				{
					onGroupCollections(false);
				}

				break;
			}
		}
	}
	else if (id == 3) // collection has been renamed
	{
		// update UI
		{
			onSortChangeInt(osu_songbrowser_sortingtype.getString(), false);
		}
	}
}

void OsuSongBrowser2::highlightScore(uint64_t unixTimestamp)
{
	for (size_t i=0; i<m_scoreButtonCache.size(); i++)
	{
		if (m_scoreButtonCache[i]->getScore().unixTimestamp == unixTimestamp)
		{
			m_scoreBrowser->scrollToElement(m_scoreButtonCache[i], 0, 10);
			m_scoreButtonCache[i]->highlight();
			break;
		}
	}
}

void OsuSongBrowser2::recalculateStarsForSelectedBeatmap(bool force)
{
	if (!osu_songbrowser_dynamic_star_recalc.getBool()) return;
	if (m_selectedBeatmap == NULL || m_selectedBeatmap->getSelectedDifficulty2() == NULL) return;

	// HACKHACK: temporarily deactivated, see OsuSongBrowser2::update(), but only if drawing scrubbing timeline strain graph is enabled (or "Draw Stats: Stars* (Total)", or "Draw Stats: pp (SS)")
	if (!m_osu_draw_scrubbing_timeline_strain_graph_ref->getBool() && !m_osu_draw_statistics_perfectpp_ref->getBool() && !m_osu_draw_statistics_totalstars_ref->getBool())
	{
		if (m_osu->isInPlayMode())
		{
			m_bBackgroundStarCalcScheduled = true;
			m_bBackgroundStarCalcScheduledForce = (m_bBackgroundStarCalcScheduledForce || force);

			return;
		}
	}

	if (checkHandleKillDynamicStarCalculator(true))
	{
		// NOTE: this only works safely because OsuDatabaseBeatmapStarCalculator does no work in load(), because it might still be in the ResourceManager's sync load() queue, so future loadAsync() could crash with the old pending load()

		m_dynamicStarCalculator->release();
		m_dynamicStarCalculator->revive();

		const float AR = m_selectedBeatmap->getAR(false);
		const float CS = m_selectedBeatmap->getCS();
		const float OD = m_selectedBeatmap->getOD(false);
		const float HP = m_selectedBeatmap->getHP();
		const float speedMultiplier = m_osu->getSpeedMultiplier(); // NOTE: not m_selectedBeatmap->getSpeedMultiplier()!
		const bool hidden = m_osu->getModHD();
		const bool relax = m_osu->getModRelax();
		const bool autopilot = m_osu->getModAutopilot();
		const bool touchdevice = m_osu->getModTD();

		m_dynamicStarCalculator->setBeatmapDifficulty(m_selectedBeatmap->getSelectedDifficulty2(), AR, CS, OD, HP, speedMultiplier, hidden, relax, autopilot, touchdevice);

		engine->getResourceManager()->requestNextLoadAsync();
		engine->getResourceManager()->loadResource(m_dynamicStarCalculator);
	}
	else
	{
		m_bBackgroundStarCalcScheduled = true;
		m_bBackgroundStarCalcScheduledForce = (m_bBackgroundStarCalcScheduledForce || force);
	}
}

void OsuSongBrowser2::selectSongButton(OsuUISongBrowserButton *songButton)
{
	if (songButton != NULL && !songButton->isSelected())
	{
		m_contextMenu->setVisible2(false);
		songButton->select();
	}
}

void OsuSongBrowser2::selectRandomBeatmap(bool playMusicFromPreviewPoint)
{
	// filter songbuttons or independent diffs
	const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
	std::vector<OsuUISongBrowserSongButton*> songButtons;
	for (size_t i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(elements[i]);
		OsuUISongBrowserSongDifficultyButton *songDifficultyButtonPointer = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(elements[i]);

		if (songButtonPointer != NULL && (songDifficultyButtonPointer == NULL || songDifficultyButtonPointer->isIndependentDiffButton())) // only allow songbuttons or independent diffs
			songButtons.push_back(songButtonPointer);
	}

	if (songButtons.size() < 1) return;

	// remember previous
	if (m_previousRandomBeatmaps.size() == 0 && m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty2() != NULL)
		m_previousRandomBeatmaps.push_back(m_selectedBeatmap->getSelectedDifficulty2());

	std::uniform_int_distribution<size_t> rng(0, songButtons.size() - 1);
	size_t randomIndex = rng(m_rngalg);
	OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>(songButtons[randomIndex]);
	selectSongButton(songButton);
}

void OsuSongBrowser2::selectPreviousRandomBeatmap()
{
	if (m_previousRandomBeatmaps.size() > 0)
	{
		OsuDatabaseBeatmap *currentRandomBeatmap = m_previousRandomBeatmaps.back();
		if (m_previousRandomBeatmaps.size() > 1 && m_selectedBeatmap != NULL && m_previousRandomBeatmaps[m_previousRandomBeatmaps.size()-1] == m_selectedBeatmap->getSelectedDifficulty2())
			m_previousRandomBeatmaps.pop_back(); // deletes the current beatmap which may also be at the top (so we don't switch to ourself)

		// filter songbuttons
		const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
		std::vector<OsuUISongBrowserSongButton*> songButtons;
		for (size_t i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(elements[i]);

			if (songButtonPointer != NULL) // allow ALL songbuttons
				songButtons.push_back(songButtonPointer);
		}

		// select it, if we can find it (and remove it from memory)
		bool foundIt = false;
		const OsuDatabaseBeatmap *previousRandomBeatmap = m_previousRandomBeatmaps.back();
		for (size_t i=0; i<songButtons.size(); i++)
		{
			if (songButtons[i]->getDatabaseBeatmap() != NULL && songButtons[i]->getDatabaseBeatmap() == previousRandomBeatmap)
			{
				m_previousRandomBeatmaps.pop_back();
				selectSongButton(songButtons[i]);
				foundIt = true;
				break;
			}

			const std::vector<OsuUISongBrowserButton*> &children = songButtons[i]->getChildren();
			for (size_t c=0; c<children.size(); c++)
			{
				if (children[c]->getDatabaseBeatmap() == previousRandomBeatmap)
				{
					m_previousRandomBeatmaps.pop_back();
					selectSongButton(children[c]);
					foundIt = true;
					break;
				}
			}

			if (foundIt)
				break;
		}

		// if we didn't find it then restore the current random beatmap, which got pop_back()'d above (shit logic)
		if (!foundIt)
			m_previousRandomBeatmaps.push_back(currentRandomBeatmap);
	}
}

void OsuSongBrowser2::playSelectedDifficulty()
{
	const std::vector<CBaseUIElement*> &elements = m_songBrowser->getContainer()->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserSongDifficultyButton *songDifficultyButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(elements[i]);
		if (songDifficultyButton != NULL && songDifficultyButton->isSelected())
		{
			songDifficultyButton->select();
			break;
		}
	}
}

void OsuSongBrowser2::recreateCollectionsButtons()
{
	// reset
	{
		m_selectionPreviousCollectionButton = NULL;
		for (size_t i=0; i<m_collectionButtons.size(); i++)
		{
			delete m_collectionButtons[i];
		}
		m_collectionButtons.clear();

		// sanity
		if (m_group == GROUP::GROUP_COLLECTIONS)
		{
			m_songBrowser->getContainer()->empty();
			m_visibleSongButtons.clear();
		}
	}

	const std::vector<OsuDatabase::Collection> &collections = m_db->getCollections();
	for (size_t i=0; i<collections.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> children;
		{
			for (size_t b=0; b<collections[i].beatmaps.size(); b++)
			{
				const OsuDatabaseBeatmap *collectionBeatmap = collections[i].beatmaps[b].first;
				const std::vector<OsuDatabaseBeatmap*> &colDiffs = collections[i].beatmaps[b].second;
				for (size_t sb=0; sb<m_songButtons.size(); sb++)
				{
					// first: search direct buttons on collection beatmap
					bool isMatchingSongButton = (m_songButtons[sb]->getDatabaseBeatmap() == collectionBeatmap);
					if (!isMatchingSongButton)
					{
						// second: search direct buttons on collection diffs (e.g. standalone diffs which still have a wrapper beatmap)
						for (size_t c=0; c<colDiffs.size(); c++)
						{
							isMatchingSongButton = (m_songButtons[sb]->getDatabaseBeatmap() == colDiffs[c]);
							if (isMatchingSongButton)
								break;
						}

						if (!isMatchingSongButton)
						{
							// third: search child buttons
							const std::vector<OsuUISongBrowserButton*> &songButtonChildren = m_songButtons[sb]->getChildren();
							for (size_t sbc=0; sbc<songButtonChildren.size(); sbc++)
							{
								// check set match
								if (songButtonChildren[sbc]->getDatabaseBeatmap() == collectionBeatmap)
								{
									isMatchingSongButton = true;
									break;
								}
							}
						}
					}

					if (isMatchingSongButton)
					{
						const std::vector<OsuUISongBrowserButton*> &diffChildren = m_songButtons[sb]->getChildren();

						std::vector<OsuUISongBrowserButton*> matchingDiffs;
						for (size_t d=0; d<diffChildren.size(); d++)
						{
							OsuUISongBrowserButton *songButtonPointer = diffChildren[d];
							for (size_t cd=0; cd<colDiffs.size(); cd++)
							{
								if (songButtonPointer->getDatabaseBeatmap() == colDiffs[cd])
									matchingDiffs.push_back(songButtonPointer);
							}
						}

						// new: only add matched diff buttons, instead of the set button
						// however, if all diffs match, then add the set button instead (user added all diffs of beatmap into collection)
						if (diffChildren.size() == matchingDiffs.size())
							children.push_back(m_songButtons[sb]); // TODO: this is more of a convenience workaround until dynamic regrouping is implemented
						else
							children.insert(children.end(), matchingDiffs.begin(), matchingDiffs.end());

						break;
					}
				}
			}
		}
		OsuUISongBrowserCollectionButton *collectionButton = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size()*50, 200, 50, "", collections[i].name, children);
		m_collectionButtons.push_back(collectionButton);
	}
}
