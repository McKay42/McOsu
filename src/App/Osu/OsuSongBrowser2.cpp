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
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuNotificationOverlay.h"
#include "OsuRankingScreen.h"
#include "OsuModSelector.h"
#include "OsuOptionsMenu.h"
#include "OsuKeyBindings.h"
#include "OsuRichPresence.h"

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

ConVar osu_gamemode("osu_gamemode", "std");

ConVar osu_songbrowser_sortingtype("osu_songbrowser_sortingtype", "By Date Added");
ConVar osu_songbrowser_scores_sortingtype("osu_songbrowser_scores_sortingtype", "Sort By Score");
ConVar osu_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f);
ConVar osu_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f);
ConVar osu_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f);
ConVar osu_songbrowser_topbar_right_height_percent("osu_songbrowser_topbar_right_height_percent", 0.5f);
ConVar osu_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.378f);
ConVar osu_songbrowser_bottombar_percent("osu_songbrowser_bottombar_percent", 0.116f);

ConVar osu_draw_songbrowser_background_image("osu_draw_songbrowser_background_image", true);
ConVar osu_draw_songbrowser_menu_background_image("osu_draw_songbrowser_menu_background_image", true);
ConVar osu_songbrowser_background_fade_in_duration("osu_songbrowser_background_fade_in_duration", 0.1f);

ConVar osu_songbrowser_draw_top_ranks_available_info_message("osu_songbrowser_draw_top_ranks_available_info_message", true);



class OsuUISongBrowserDifficultyCollectionButton : public OsuUISongBrowserCollectionButton
{
public:
	OsuUISongBrowserDifficultyCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children) : OsuUISongBrowserCollectionButton(osu, songBrowser, view, xPos, yPos, xSize, ySize, name, collectionName, children)
	{
		s_previousButton = NULL;
	}

	virtual void setPreviousButton(OsuUISongBrowserCollectionButton *previousButton)
	{
		s_previousButton = previousButton;
	}

	virtual OsuUISongBrowserCollectionButton *getPreviousButton()
	{
		return s_previousButton;
	}

private:
	static OsuUISongBrowserCollectionButton *s_previousButton;
};

OsuUISongBrowserCollectionButton *OsuUISongBrowserDifficultyCollectionButton::s_previousButton = NULL;



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



struct SortByArtist : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByArtist() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		std::wstring artistLowercase1 = std::wstring((a->getBeatmap()->getArtist().wc_str() == NULL || a->getBeatmap()->getArtist().length() < 1) ? L"" : a->getBeatmap()->getArtist().wc_str());
		std::wstring artistLowercase2 = std::wstring((b->getBeatmap()->getArtist().wc_str() == NULL || b->getBeatmap()->getArtist().length() < 1) ? L"" : b->getBeatmap()->getArtist().wc_str());

		std::transform(artistLowercase1.begin(), artistLowercase1.end(), artistLowercase1.begin(), std::towlower);
		std::transform(artistLowercase2.begin(), artistLowercase2.end(), artistLowercase2.begin(), std::towlower);

		// strict weak ordering!
		if (artistLowercase1 == artistLowercase2)
			return a->getSortHack() < b->getSortHack();
		return artistLowercase1 < artistLowercase2;
	}
};

struct SortByBPM : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByBPM() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		int bpm1 = 0;
		int bpm2 = 0;
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<aDiffs->size(); i++)
		{
			if ((*aDiffs)[i]->maxBPM > bpm1)
				bpm1 = (*aDiffs)[i]->maxBPM;
		}

		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<bDiffs->size(); i++)
		{
			if ((*bDiffs)[i]->maxBPM > bpm2)
				bpm2 = (*bDiffs)[i]->maxBPM;
		}

		// strict weak ordering!
		if (bpm1 == bpm2)
			return a->getSortHack() < b->getSortHack();
		return bpm1 < bpm2;
	}
};

struct SortByCreator : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByCreator() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		std::wstring creatorLowercase1;
		std::wstring creatorLowercase2;
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		if (aDiffs->size() > 0)
			creatorLowercase1 = std::wstring(((*aDiffs)[aDiffs->size()-1]->creator.wc_str() == NULL || (*aDiffs)[aDiffs->size()-1]->creator.length() < 1) ? L"" : (*aDiffs)[aDiffs->size()-1]->creator.wc_str());

		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		if (bDiffs->size() > 0)
			creatorLowercase2 = std::wstring(((*bDiffs)[bDiffs->size()-1]->creator.wc_str() == NULL || (*bDiffs)[bDiffs->size()-1]->creator.length() < 1) ? L"" : (*bDiffs)[bDiffs->size()-1]->creator.wc_str());

		std::transform(creatorLowercase1.begin(), creatorLowercase1.end(), creatorLowercase1.begin(), std::towlower);
		std::transform(creatorLowercase2.begin(), creatorLowercase2.end(), creatorLowercase2.begin(), std::towlower);

		// strict weak ordering!
		if (creatorLowercase1 == creatorLowercase2)
			return a->getSortHack() < b->getSortHack();
		return creatorLowercase1 < creatorLowercase2;
	}
};

struct SortByDateAdded : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByDateAdded() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		long long time1 = std::numeric_limits<long long>::min();
		long long time2 = std::numeric_limits<long long>::min();
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<aDiffs->size(); i++)
		{
			if ((*aDiffs)[i]->lastModificationTime > time1)
				time1 = (*aDiffs)[i]->lastModificationTime;
		}

		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<bDiffs->size(); i++)
		{
			if ((*bDiffs)[i]->lastModificationTime > time2)
				time2 = (*bDiffs)[i]->lastModificationTime;
		}

		// strict weak ordering!
		if (time1 == time2)
			return a->getSortHack() > b->getSortHack();
		return time1 > time2;
	}
};

struct SortByDifficulty : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByDifficulty() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		float diff1 = 0.0f;
		float stars1 = 0.0f;
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<aDiffs->size(); i++)
		{
			OsuBeatmapDifficulty *d = (*aDiffs)[i];
			if (d->starsNoMod > stars1)
				stars1 = d->starsNoMod;

			float tempDiff1 = (d->AR+1)*(d->CS+1)*(d->HP+1)*(d->OD+1)*(d->maxBPM > 0 ? d->maxBPM : 1);
			if (tempDiff1 > diff1)
				diff1 = tempDiff1;
		}

		float diff2 = 0.0f;
		float stars2 = 0.0f;
		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<bDiffs->size(); i++)
		{
			OsuBeatmapDifficulty *d = (*bDiffs)[i];
			if (d->starsNoMod > stars2)
				stars2 = d->starsNoMod;

			float tempDiff2 = (d->AR+1)*(d->CS+1)*(d->HP+1)*(d->OD+1)*(d->maxBPM > 0 ? d->maxBPM : 1);
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
};

struct SortByLength : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByLength() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		unsigned long length1 = 0;
		unsigned long length2 = 0;
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<aDiffs->size(); i++)
		{
			if ((*aDiffs)[i]->lengthMS > length1)
				length1 = (*aDiffs)[i]->lengthMS;
		}

		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		for (int i=0; i<bDiffs->size(); i++)
		{
			if ((*bDiffs)[i]->lengthMS > length2)
				length2 = (*bDiffs)[i]->lengthMS;
		}

		// strict weak ordering!
		if (length1 == length2)
			return a->getSortHack() < b->getSortHack();
		return length1 < length2;
	}
};

struct SortByTitle : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByTitle() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		if (a->getBeatmap() == NULL || b->getBeatmap() == NULL)
			return a->getSortHack() < b->getSortHack();

		std::wstring titleLowercase1 = std::wstring((a->getBeatmap()->getTitle().wc_str() == NULL || a->getBeatmap()->getTitle().length() < 1) ? L"" : a->getBeatmap()->getTitle().wc_str());
		std::wstring titleLowercase2 = std::wstring((b->getBeatmap()->getTitle().wc_str() == NULL || b->getBeatmap()->getTitle().length() < 1) ? L"" : b->getBeatmap()->getTitle().wc_str());

		std::transform(titleLowercase1.begin(), titleLowercase1.end(), titleLowercase1.begin(), std::towlower);
		std::transform(titleLowercase2.begin(), titleLowercase2.end(), titleLowercase2.begin(), std::towlower);

		// strict weak ordering!
		if (titleLowercase1 == titleLowercase2)
			return a->getSortHack() < b->getSortHack();
		return titleLowercase1 < titleLowercase2;
	}
};



OsuSongBrowser2::OsuSongBrowser2(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// random selection algorithm init
	m_rngalg = std::mt19937(time(0));

	// sorting/grouping + methods
	m_group = GROUP::GROUP_NO_GROUPING;
	m_sortingMethod = SORT::SORT_ARTIST;

	m_sortingMethods.push_back({SORT::SORT_ARTIST, "By Artist", new SortByArtist()});
	m_sortingMethods.push_back({SORT::SORT_BPM, "By BPM", new SortByBPM()});
	m_sortingMethods.push_back({SORT::SORT_CREATOR, "By Creator", new SortByCreator()});
	m_sortingMethods.push_back({SORT::SORT_DATEADDED, "By Date Added", new SortByDateAdded()});
	m_sortingMethods.push_back({SORT::SORT_DIFFICULTY, "By Difficulty", new SortByDifficulty()});
	m_sortingMethods.push_back({SORT::SORT_LENGTH, "By Length", new SortByLength()});
	///m_sortingMethods.push_back({SORT::SORT_RANKACHIEVED, "By Rank Achieved", new SortByRankAchieved()}); // not yet possible
	m_sortingMethods.push_back({SORT::SORT_TITLE, "By Title", new SortByTitle()});

	// convar refs
	m_fps_max_ref = convar->getConVarByName("fps_max");
	m_osu_database_dynamic_star_calculation_ref = convar->getConVarByName("osu_database_dynamic_star_calculation");
	m_osu_scores_enabled = convar->getConVarByName("osu_scores_enabled");
	m_name_ref = convar->getConVarByName("name");

	// convar callbacks
	osu_gamemode.setCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onModeChange) );

	// vars
	m_bF1Pressed = false;
	m_bF2Pressed = false;
	m_bShiftPressed = false;
	m_bLeft = false;
	m_bRight = false;

	m_bRandomBeatmapScheduled = false;
	m_bPreviousRandomBeatmapScheduled = false;

	m_fSongSelectTopScale = 1.0f;

	// build topbar left
	m_topbarLeft = new CBaseUIContainer(0, 0, 0, 0, "");

	m_songInfo = new OsuUISongBrowserInfoLabel(m_osu, 0, 0, 0, 0, "");
	m_topbarLeft->addBaseUIElement(m_songInfo);

	m_scoreSortButton = addTopBarLeftTabButton("Sort By Score");
	m_scoreSortButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortScoresClicked) );
	m_webButton = addTopBarLeftButton("Web");
	m_webButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onWebClicked) );

	// build topbar right
	m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");
	m_groupLabel = new CBaseUILabel(0, 0, 0, 0, "", "Group:");
	m_groupLabel->setSizeToContent(3);
	m_groupLabel->setDrawFrame(false);
	m_groupLabel->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(m_groupLabel);

	m_collectionsButton = addTopBarRightTabButton("Collections");
	m_collectionsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupCollections) );
	///addTopBarRightTabButton("By Artist");
	///addTopBarRightTabButton("By Date Added")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortDateAdded) );
	///addTopBarRightTabButton("By Difficulty")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupDifficulty) );
	m_noGroupingButton = addTopBarRightTabButton("No Grouping");
	m_noGroupingButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupNoGrouping) );
	m_noGroupingButton->setTextBrightColor(COLOR(255, 0, 255, 0));


	addTopBarRightSortButton("")->setVisible(false);
	m_sortLabel = new CBaseUILabel(0, 0, 0, 0, "", "Sort:");
	m_sortLabel->setSizeToContent(3);
	m_sortLabel->setDrawFrame(false);
	m_sortLabel->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(m_sortLabel);
	m_sortButton = addTopBarRightSortButton("By Date Added");
	m_sortButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortClicked) );
	m_contextMenu = new OsuUIContextMenu(m_osu, 50, 50, 150, 0, "");
	m_contextMenu->setVisible(true);
	///m_topbarRight->addBaseUIElement(m_contextMenu);


	// build bottombar
	m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

	///addBottombarNavButton();
	/*
	CBaseUIButton *modeButton = addBottombarNavButton([this]() -> Image *{return m_osu->getSkin()->getSelectionMode();}, [this]() -> Image *{return m_osu->getSkin()->getSelectionModeOver();});
	modeButton->setText("std");
	modeButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMode) );
	*/
	addBottombarNavButton([this]() -> Image *{return m_osu->getSkin()->getSelectionMods();}, [this]() -> Image *{return m_osu->getSkin()->getSelectionModsOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMods) );
	addBottombarNavButton([this]() -> Image *{return m_osu->getSkin()->getSelectionRandom();}, [this]() -> Image *{return m_osu->getSkin()->getSelectionRandomOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionRandom) );
	///addBottombarNavButton([this]() -> Image *{return m_osu->getSkin()->getSelectionOptions();}, [this]() -> Image *{return m_osu->getSkin()->getSelectionOptionsOver();})->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser::onSelectionOptions) );

	m_userButton = new OsuUISongBrowserUserButton(m_osu);
	m_userButton->addTooltipLine("Click to change [User] or view [Top Ranks]");
	m_userButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onUserButtonClicked) );
	m_userButton->setText(m_name_ref->getString());
	m_bottombar->addBaseUIElement(m_userButton);

	// build scorebrowser
	m_scoreBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_scoreBrowser->setDrawBackground(false);
	m_scoreBrowser->setDrawFrame(false);
	m_scoreBrowser->setClipping(false);
	m_scoreBrowser->setHorizontalScrolling(false);
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
	m_bInSearch = false;
	m_searchPrevGroup = GROUP::GROUP_NO_GROUPING;

	// background star calculation
	m_fBackgroundStarCalculationWorkNotificationTime = 0.0f;
	m_iBackgroundStarCalculationIndex = 0;

	updateLayout();
}

OsuSongBrowser2::~OsuSongBrowser2()
{
	m_songBrowser->getContainer()->empty();
	for (int i=0; i<m_songButtons.size(); i++)
	{
		delete m_songButtons[i];
	}
	for (int i=0; i<m_collectionButtons.size(); i++)
	{
		delete m_collectionButtons[i];
	}
	for (int i=0; i<m_difficultyCollectionButtons.size(); i++)
	{
		delete m_difficultyCollectionButtons[i];
	}

	m_scoreBrowser->getContainer()->empty();
	for (int i=0; i<m_scoreButtonCache.size(); i++)
	{
		delete m_scoreButtonCache[i];
	}
	SAFE_DELETE(m_scoreBrowserNoRecordsYetElement);

	for (int i=0; i<m_sortingMethods.size(); i++)
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
			const bool ready = m_osu->getSelectedBeatmap() != NULL
					&& m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL
					&& m_osu->getSelectedBeatmap()->getSelectedDifficulty()->backgroundImage != NULL
					&& m_osu->getSelectedBeatmap()->getSelectedDifficulty()->backgroundImage->isReady();

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

	// draw song browser
	m_songBrowser->draw(g);

	// draw search
	m_search->setSearchString(m_sSearchString);
	m_search->setDrawNumResults(m_bInSearch);
	m_search->setNumFoundResults(m_visibleSongButtons.size());
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

	m_bottombar->draw(g);
	OsuScreenBackable::draw(g);
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
	if (osu_songbrowser_draw_top_ranks_available_info_message.getBool())
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
	if (osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		Image *backgroundImage = osu->getSelectedBeatmap()->getSelectedDifficulty()->backgroundImage;
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

	// HACKHACK:
	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
		engine->getMouse()->resetWheelDelta();

	// update and focus handling
	m_songBrowser->update();
	m_songBrowser->getContainer()->update_pos(); // necessary due to constant animations
	m_bottombar->update();
	m_scoreBrowser->update();
	m_topbarLeft->update();
	m_topbarRight->update();
	m_contextMenu->update();

	if (m_contextMenu->isMouseInside() || m_osu->getHUD()->isVolumeOverlayBusy() || m_backButton->isMouseInside())
	{
		m_topbarLeft->stealFocus();
		m_topbarRight->stealFocus();
		m_songBrowser->stealFocus();
		m_bottombar->stealFocus();
		if (!m_scoreBrowser->isBusy())
			m_scoreBrowser->stealFocus();
	}

	if (m_contextMenu->isMouseInside())
		m_backButton->stealFocus();

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
	}

	if (m_osu->getOptionsMenu()->isBusy())
		m_songBrowser->stealFocus();

	// handle right click absolute drag scrolling
	if (m_songBrowser->isMouseInside() && engine->getMouse()->isRightDown())
		m_songBrowser->scrollToY(-((engine->getMouse()->getPos().y - 2 - m_songBrowser->getPos().y)/m_songBrowser->getSize().y)*m_songBrowser->getScrollSize().y);

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
	if (engine->getMouse()->getPos().x < m_osu->getScreenWidth()*0.1f)
		scrollToSelectedSongButton();

	// handle searching
	if (m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime)
	{
		m_fSearchWaitTime = 0.0f;
		onSearchUpdate();
	}

	// handle background star calculation
	if (m_beatmaps.size() > 0 && m_osu_database_dynamic_star_calculation_ref->getBool())
	{
		for (int s=0; s<1; s++) // one beatmap per update
		{
			bool canMoveToNextBeatmap = true;
			if (m_iBackgroundStarCalculationIndex < m_beatmaps.size())
			{
				for (int i=0; i<m_beatmaps[m_iBackgroundStarCalculationIndex]->getDifficultiesPointer()->size(); i++)
				{
					OsuBeatmapDifficulty *diff = (*m_beatmaps[m_iBackgroundStarCalculationIndex]->getDifficultiesPointer())[i];
					if (!diff->isBackgroundLoaderActive() && diff->starsNoMod == 0.0f)
					{
						diff->semaphore = true; // NOTE: this is used by the BackgroundImagePathLoader to wait until the main thread is done, and then recalculate accurately
						{
							diff->loadMetadataRaw(true, true); // NOTE: calculateStarsInaccurately = true
						}
						diff->semaphore = false;

						m_fBackgroundStarCalculationWorkNotificationTime = engine->getTime() + 0.1f;

						// only one diff per beatmap per update
						canMoveToNextBeatmap = false;
						break;
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

	// HACKHACK: handle delayed star calculation UI update for song info label
	if (getSelectedBeatmap() != NULL && getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		if (!getSelectedBeatmap()->getSelectedDifficulty()->isBackgroundLoaderActive() || getSelectedBeatmap()->getSelectedDifficulty()->starsWereCalculatedAccurately)
			m_songInfo->setStars(getSelectedBeatmap()->getSelectedDifficulty()->starsNoMod);

		m_songInfo->setStarsRecalculating(!getSelectedBeatmap()->getSelectedDifficulty()->starsWereCalculatedAccurately);
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
	if (m_bBeatmapRefreshScheduled)
		return;

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
					// TODO: use a CBaseUITextbox instead for the search box
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
	else
	{
		if (key == KEY_ESCAPE) // can't support GAME_PAUSE hotkey here because of text searching
			m_osu->toggleSongBrowser();
	}

	if (key == KEY_SHIFT)
		m_bShiftPressed = true;

	// function hotkeys
	if (key == KEY_F1 && !m_bF1Pressed)
	{
		m_bF1Pressed = true;
		m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 1 : 0]->keyboardPulse();
		onSelectionMods();
	}
	if (key == KEY_F2 && !m_bF2Pressed)
	{
		m_bF2Pressed = true;
		m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 2 : 1]->keyboardPulse();
		onSelectionRandom();
	}
	if (key == KEY_F3)
		onSelectionOptions();

	if (key == KEY_F5)
		refreshBeatmaps();

	// selection move
	if (!engine->getKeyboard()->isAltDown() && key == KEY_DOWN)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();

		// get bottom selection
		int selectedIndex = -1;
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		// select +1
		if (selectedIndex > -1 && selectedIndex+1 < elements->size())
		{
			int nextSelectionIndex = selectedIndex+1;
			OsuUISongBrowserButton *nextButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[nextSelectionIndex]);
			OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>((*elements)[nextSelectionIndex]);
			if (nextButton != NULL)
			{
				nextButton->select();

				// if this is a song button, select top child
				if (songButton != NULL)
				{
					std::vector<OsuUISongBrowserButton*> children = songButton->getChildren();
					if (children.size() > 0 && !children[0]->isSelected())
						children[0]->select();
				}
			}
		}
	}

	if (!engine->getKeyboard()->isAltDown() && key == KEY_UP)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();

		// get bottom selection
		int selectedIndex = -1;
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		// select -1
		if (selectedIndex > -1 && selectedIndex-1 > -1)
		{
			int nextSelectionIndex = selectedIndex-1;
			OsuUISongBrowserButton *nextButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[nextSelectionIndex]);
			bool isCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>((*elements)[nextSelectionIndex]);

			if (nextButton != NULL)
			{
				nextButton->select();

				// automatically open collection on top of this one and go to bottom child
				if (isCollectionButton && nextSelectionIndex-1 > -1)
				{
					nextSelectionIndex = nextSelectionIndex-1;
					OsuUISongBrowserCollectionButton *nextCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>((*elements)[nextSelectionIndex]);
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
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		bool foundSelected = false;
		for (int i=elements->size()-1; i>=0; i--)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			bool isSongDifficultyButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>((*elements)[i]) != NULL;

			if (foundSelected && button != NULL && !button->isSelected() && !isSongDifficultyButton)
			{
				button->select();

				// automatically open collection below and go to bottom child
				OsuUISongBrowserCollectionButton *collectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>((*elements)[i]);
				if (collectionButton != NULL)
				{
					std::vector<OsuUISongBrowserButton*> children = collectionButton->getChildren();
					if (children.size() > 0 && !children[children.size()-1]->isSelected())
						children[children.size()-1]->select();
				}
				break;
			}

			if (button != NULL && button->isSelected())
				foundSelected = true;
		}
	}

	if (key == KEY_RIGHT && !m_bRight)
	{
		m_bRight = true;
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();

		// get bottom selection
		int selectedIndex = -1;
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (button != NULL && button->isSelected())
				selectedIndex = i;
		}

		if (selectedIndex > -1)
		{
			for (int i=selectedIndex; i<elements->size(); i++)
			{
				OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
				bool isSongDifficultyButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>((*elements)[i]) != NULL;

				if (button != NULL && !button->isSelected() && !isSongDifficultyButton)
				{
					button->select();
					break;
				}
			}
		}
	}

	// selection select
	if (key == KEY_ENTER)
		playSelectedDifficulty();

	// toggle auto
	if (key == KEY_A && engine->getKeyboard()->isControlDown())
		m_osu->getModSelector()->toggleAuto();

	key.consume();
}

void OsuSongBrowser2::onKeyUp(KeyboardEvent &key)
{
	if (key == KEY_SHIFT)
		m_bShiftPressed = false;
	if (key == KEY_LEFT)
		m_bLeft = false;
	if (key == KEY_RIGHT)
		m_bRight = false;

	if (key == KEY_F1)
		m_bF1Pressed = false;
	if (key == KEY_F2)
		m_bF2Pressed = false;
}

void OsuSongBrowser2::onChar(KeyboardEvent &e)
{
	if (e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown())) return;

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
}

void OsuSongBrowser2::onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool play, bool mp)
{
	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::SELECT, 0, false, beatmap);

	// remember it
	if (beatmap != m_selectedBeatmap)
		m_previousRandomBeatmaps.push_back(beatmap);

	m_selectedBeatmap = beatmap;

	// update song info
	m_songInfo->setFromBeatmap(beatmap, diff);

	// start playing
	if (play)
	{
		bool clientPlayStateChangeRequestBeatmapSent = false;
		if (m_osu->isInMultiplayer() && !mp)
		{
			// clients may also select beatmaps (the server can then decide if it wants to broadcast or ignore it)
			clientPlayStateChangeRequestBeatmapSent = m_osu->getMultiplayer()->onClientPlayStateChangeRequestBeatmap(beatmap);
		}

		if (!clientPlayStateChangeRequestBeatmapSent)
		{
			// CTRL + click = auto
			if (!m_osu->isInMultiplayer() && engine->getKeyboard()->isControlDown())
				m_osu->getModSelector()->enableAuto();

			m_osu->onBeforePlayStart();
			if (beatmap->play())
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

	// notify mod selector (for BPM override slider, else we get inconsistent values)
	m_osu->getModSelector()->checkUpdateBPMSliderSlaves();

	// update score display
	rebuildScoreButtons();

	// update web button
	m_webButton->setVisible(m_songInfo->getBeatmapID() > 0);
}

void OsuSongBrowser2::onDifficultySelectedMP(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool play)
{
	onDifficultySelected(beatmap, diff, play, true);
}

void OsuSongBrowser2::selectBeatmapMP(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff)
{
	// this is a bit hacky, but the easiest solution (since we are using the visible songbuttons)
	{
		// force exit search
		if (m_sSearchString.length() > 0)
		{
			m_sSearchString = "";
			onSearchUpdate();
		}

		// force no grouping
		if (m_group != GROUP::GROUP_NO_GROUPING)
			onGroupNoGrouping(m_noGroupingButton);
	}

	for (int i=0; i<m_visibleSongButtons.size(); i++)
	{
		if (m_visibleSongButtons[i]->getBeatmap() == beatmap)
		{
			OsuUISongBrowserButton *songButton = m_visibleSongButtons[i];
			for (int c=0; c<songButton->getChildrenAbs().size(); c++)
			{
				OsuUISongBrowserSongDifficultyButton *diffButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(songButton->getChildrenAbs()[c]);
				if (diffButton != NULL && diffButton->getDiff() == diff)
				{
					if (!songButton->isSelected())
						songButton->select();

					if (!diffButton->isSelected())
						diffButton->select();

					break;
				}
			}
			break;
		}
	}
}

void OsuSongBrowser2::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying) return;

	// reset
	m_selectedBeatmap = NULL;

	// delete local database and UI
	m_songBrowser->getContainer()->empty();
	for (int i=0; i<m_songButtons.size(); i++)
	{
		delete m_songButtons[i];
	}
	m_songButtons.clear();
	for (int i=0; i<m_collectionButtons.size(); i++)
	{
		delete m_collectionButtons[i];
	}
	m_collectionButtons.clear();
	for (int i=0; i<m_difficultyCollectionButtons.size(); i++)
	{
		delete m_difficultyCollectionButtons[i];
	}
	m_difficultyCollectionButtons.clear();
	m_visibleSongButtons.clear();
	m_beatmaps.clear();
	m_previousRandomBeatmaps.clear();

	// start loading
	m_bBeatmapRefreshScheduled = true;
	m_db->load();
}

void OsuSongBrowser2::scrollToSongButton(OsuUISongBrowserButton *songButton, bool alignOnTop)
{
	if (songButton != NULL)
		m_songBrowser->scrollToY(-songButton->getRelPos().y + (alignOnTop ? (0) : (m_songBrowser->getSize().y/2 - songButton->getSize().y/2)));
}

OsuUISongBrowserButton* OsuSongBrowser2::findCurrentlySelectedSongButton() const
{
	OsuUISongBrowserButton *selectedButton = NULL;
	std::vector<CBaseUIElement*> elements = m_songBrowser->getContainer()->getAllBaseUIElements();
	for (int i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
		if (button != NULL && button->isSelected())
			selectedButton = button;
	}
	return selectedButton;
}

void OsuSongBrowser2::scrollToSelectedSongButton()
{
	auto selectedButton = findCurrentlySelectedSongButton();
	scrollToSongButton(selectedButton);
}

void OsuSongBrowser2::rebuildSongButtons(bool unloadAllThumbnails)
{
	m_songBrowser->getContainer()->empty();

	if (unloadAllThumbnails)
	{
		for (int i=0; i<m_songButtons.size(); i++)
		{
			m_songButtons[i]->setVisible(false);
		}
	}

	for (int i=0; i<m_visibleSongButtons.size(); i++)
	{
		OsuUISongBrowserButton *button = m_visibleSongButtons[i];
		button->resetAnimations();

		// "parent"
		if (!(button->isSelected() && button->isHiddenIfSelected()))
			m_songBrowser->getContainer()->addBaseUIElement(m_visibleSongButtons[i]);

		// children
		std::vector<OsuUISongBrowserButton*> recursiveChildren = m_visibleSongButtons[i]->getChildren();
		if (recursiveChildren.size() > 0)
		{
			for (int c=0; c<recursiveChildren.size(); c++)
			{
				OsuUISongBrowserButton *button = recursiveChildren[c];
				button->resetAnimations();

				if (!(button->isSelected() && button->isHiddenIfSelected()))
					m_songBrowser->getContainer()->addBaseUIElement(recursiveChildren[c]);
			}
		}
	}

	updateSongButtonLayout();
}

void OsuSongBrowser2::updateSongButtonLayout()
{
	// this rebuilds the entire songButton layout (songButtons in relation to others)
	// only the y axis is set, because the x axis is constantly animated and handled within the button classes themselves
	std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();

	int yCounter = m_songBrowser->getSize().y/4;
	if (elements->size() <= 1)
		yCounter = m_songBrowser->getSize().y/2;

	bool isSelected = false;
	bool inOpenCollection = false;
	bool wasCollectionButton = false;
	for (int i=0; i<elements->size(); i++)
	{
		OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);

		if (songButton != NULL)
		{
			// HACKHACK: since individual diff buttons are not supported with the current UI structure, highlight added collection diffs in collections
			if (songButton->getCollectionDiffHack())
			{
				if (m_group == GROUP::GROUP_COLLECTIONS)
					songButton->setInactiveBackgroundColor(COLOR(255, 233, 104, 0));
				else
					songButton->setInactiveBackgroundColor(COLOR(255, 0, 150, 236));
			}

			// depending on the object type, layout differently
			const bool isCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(songButton) != NULL;
			const bool isDiffButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(songButton) != NULL;

			// give selected items & diffs a bit more spacing, to make them stand out
			if (((songButton->isSelected() && !isCollectionButton) || isSelected || isDiffButton) && !wasCollectionButton)
				yCounter += songButton->getSize().y*0.1f;

			isSelected = songButton->isSelected() || isDiffButton;

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
			wasCollectionButton = isCollectionButton;

			songButton->setTargetRelPosY(yCounter);
			songButton->updateLayoutEx();

			yCounter += songButton->getActualSize().y;
		}
	}
	m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y/2);
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
	}
}

bool OsuSongBrowser2::searchMatcher(OsuBeatmap *beatmap, UString searchString)
{
	if (beatmap == NULL) return false;

	std::vector<OsuBeatmapDifficulty*> diffs = beatmap->getDifficulties();

	// intelligent search parser
	// all strings which are not expressions get appended with spaces between, then checked with one call to findSubstringInDifficulty()
	// the rest is interpreted
	// WARNING: this code is quite shitty. the order of the operators array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
	// TODO: write proper parser
	enum operatorId
	{
		EQ,
		LT,
		GT,
		LE,
		GE,
		NE
	};
	const std::vector<std::pair<UString, operatorId>> operators =
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
		LENGTH,
		STARS
	};
	const std::vector<std::pair<UString, keywordId>> keywords =
	{
		std::pair<UString, keywordId>("ar", AR),
		std::pair<UString, keywordId>("cs", CS),
		std::pair<UString, keywordId>("od", OD),
		std::pair<UString, keywordId>("hp", HP),
		std::pair<UString, keywordId>("bpm",BPM),
		std::pair<UString, keywordId>("length", LENGTH),
		std::pair<UString, keywordId>("len", LENGTH),
		std::pair<UString, keywordId>("stars", STARS),
		std::pair<UString, keywordId>("star", STARS)
	};

	// split search string into tokens
	// parse over all difficulties
	bool expressionMatches = false; // if any diff matched all expressions
	std::vector<UString> tokens = searchString.split(" ");
	std::vector<UString> literalSearchStrings;
	for (int d=0; d<diffs.size(); d++)
	{
		bool expressionsMatch = true; // if the current search string (meaning only the expressions in this case) matches the current difficulty

		for (int i=0; i<tokens.size(); i++)
		{
			//debugLog("token[%i] = %s\n", i, tokens[i].toUtf8());
			// determine token type, interpret expression
			bool expression = false;
			for (int o=0; o<operators.size(); o++)
			{
				if (tokens[i].find(operators[o].first) != -1)
				{
					// split expression into left and right parts (only accept singular expressions, things like "0<bpm<1" will not work with this)
					//debugLog("splitting by string %s\n", operators[o].first.toUtf8());
					std::vector<UString> values = tokens[i].split(operators[o].first);
					if (values.size() == 2 && values[0].length() > 0 && values[1].length() > 0)
					{
						//debugLog("lvalue = %s, rvalue = %s\n", values[0].toUtf8(), values[1].toUtf8());
						UString lvalue = values[0];
						float rvalue = values[1].toFloat(); // this must always be a number (at least, assume it is)

						// find lvalue keyword in array (only continue if keyword exists)
						for (int k=0; k<keywords.size(); k++)
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
									compareValue = diffs[d]->AR;
									break;
								case CS:
									compareValue = diffs[d]->CS;
									break;
								case OD:
									compareValue = diffs[d]->OD;
									break;
								case HP:
									compareValue = diffs[d]->HP;
									break;
								case BPM:
									compareValue = diffs[d]->maxBPM;
									break;
								case LENGTH:
									compareValue = diffs[d]->lengthMS / 1000;
									break;
								case STARS:
									compareValue = std::round(diffs[d]->starsNoMod * 10.0f) / 10.0f; // round to 1 decimal place
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
				for (int l=0; l<literalSearchStrings.size(); l++)
				{
					if (literalSearchStrings[l] == tokens[i])
					{
						exists = true;
						break;
					}
				}
				if (!exists)
				{
					UString litAdd = tokens[i].trim();
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
	if (!expressionMatches) // if no diff matched any expression, we can already stop here
		return false;

	// build literal search string from all parts
	UString literalSearchString;
	for (int i=0; i<literalSearchStrings.size(); i++)
	{
		literalSearchString.append(literalSearchStrings[i]);
		if (i < literalSearchStrings.size()-1)
			literalSearchString.append(" ");
	}

	// early return here for literal match/contains
	if (literalSearchString.length() > 0)
	{
		for (int i=0; i<diffs.size(); i++)
		{
			bool atLeastOneFullMatch = true;

			for (int s=0; s<literalSearchStrings.size(); s++)
			{
				if (!findSubstringInDifficulty(diffs[i], literalSearchStrings[s]))
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

bool OsuSongBrowser2::findSubstringInDifficulty(OsuBeatmapDifficulty *diff, UString &searchString)
{
	std::string stdSearchString = searchString.toUtf8();

	if (diff->title.length() > 0)
	{
		std::string difficultySongTitle = diff->title.toUtf8();
		if (Osu::findIgnoreCase(difficultySongTitle, stdSearchString))
			return true;
	}

	if (diff->artist.length() > 0)
	{
		std::string difficultySongArtist = diff->artist.toUtf8();
		if (Osu::findIgnoreCase(difficultySongArtist, stdSearchString))
			return true;
	}

	if (diff->creator.length() > 0)
	{
		std::string difficultySongCreator = diff->creator.toUtf8();
		if (Osu::findIgnoreCase(difficultySongCreator, stdSearchString))
			return true;
	}

	if (diff->name.length() > 0)
	{
		std::string difficultyName = diff->name.toUtf8();
		if (Osu::findIgnoreCase(difficultyName, stdSearchString))
			return true;
	}

	if (diff->source.length() > 0)
	{
		std::string difficultySongSource = diff->source.toUtf8();
		if (Osu::findIgnoreCase(difficultySongSource, stdSearchString))
			return true;
	}

	if (diff->tags.length() > 0)
	{
		std::string difficultySongTags = diff->tags.toUtf8();
		if (Osu::findIgnoreCase(difficultySongTags, stdSearchString))
			return true;
	}

	return false;
}

void OsuSongBrowser2::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float uiScale = Osu::ui_scale->getFloat();
	const float dpiScale = Osu::getUIScale();

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
		m_topbarRightSortButtons[i]->setSize(topbarRightSortButtonWidth, topbarRightSortButtonHeight);
		m_topbarRightSortButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightSortButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightSortButtonWidth), topbarRightSortButtonMargin);
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
	float navBarStart = std::max(m_osu->getScreenWidth()*0.175f, m_backButton->getSize().x);
	if (m_osu->getScreenWidth()*0.175f * uiScale < m_backButton->getSize().x + 25)
		navBarStart += m_backButton->getSize().x - m_osu->getScreenWidth()*0.175f * uiScale;

	// bottombar cont
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(m_osu->getScreenWidth(), bottomBarHeight);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setRelPosX((i == 0 ? navBarStart : 0) + (i > 0 ? m_bottombarNavButtons[i-1]->getRelPos().x + m_bottombarNavButtons[i-1]->getSize().x : 0));
	}

	const int userButtonHeight = m_bottombar->getSize().y*0.9f;
	m_userButton->setSize(userButtonHeight*3.5f, userButtonHeight);
	m_userButton->setRelPos(std::max(m_bottombar->getSize().x/2 - m_userButton->getSize().x/2, m_bottombarNavButtons[m_bottombarNavButtons.size()-1]->getRelPos().x + m_bottombarNavButtons[m_bottombarNavButtons.size()-1]->getSize().x + 10), m_bottombar->getSize().y - m_userButton->getSize().y - 1);

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
	const float dpiScale = Osu::getUIScale();

	if (m_osu_scores_enabled->getBool() != m_scoreBrowser->isVisible())
		m_scoreBrowser->setVisible(m_osu_scores_enabled->getBool());

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
	for (int i=0; i<m_scoreBrowser->getContainer()->getAllBaseUIElementsPointer()->size(); i++)
	{
		CBaseUIElement *scoreButton = (*m_scoreBrowser->getContainer()->getAllBaseUIElementsPointer())[i];
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

	const bool validBeatmap = (getSelectedBeatmap() != NULL && getSelectedBeatmap()->getSelectedDifficulty() != NULL);
	const int numScores = (validBeatmap ? ((*m_db->getScores())[getSelectedBeatmap()->getSelectedDifficulty()->md5hash]).size() : 0);

	// top up cache as necessary
	if (numScores > m_scoreButtonCache.size())
	{
		const int numNewButtons = numScores - m_scoreButtonCache.size();
		for (int i=0; i<numNewButtons; i++)
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
		m_db->sortScores(getSelectedBeatmap()->getSelectedDifficulty()->md5hash);

		// build
		std::vector<OsuUISongBrowserScoreButton*> scoreButtons;
		for (int i=0; i<numScores; i++)
		{
			OsuUISongBrowserScoreButton *button = m_scoreButtonCache[i];
			button->setName(UString(getSelectedBeatmap()->getSelectedDifficulty()->md5hash.c_str()));
			button->setScore((*m_db->getScores())[getSelectedBeatmap()->getSelectedDifficulty()->md5hash][i], i+1);
			scoreButtons.push_back(button);
		}

		// add
		for (int i=0; i<numScores; i++)
		{
			scoreButtons[i]->setIndex(i+1);
			m_scoreBrowser->getContainer()->addBaseUIElement(scoreButtons[i]);
		}

		// reset
		for (int i=0; i<scoreButtons.size(); i++)
		{
			scoreButtons[i]->resetHighlight();
		}
	}

	// layout
	updateScoreBrowserLayout();
}

void OsuSongBrowser2::scheduleSearchUpdate(bool immediately)
{
	m_fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : 0.5f);
}

OsuUISelectionButton *OsuSongBrowser2::addBottombarNavButton(std::function<Image*()> getImageFunc, std::function<Image*()> getImageOverFunc)
{
	OsuUISelectionButton *btn = new OsuUISelectionButton(getImageFunc, getImageOverFunc, 0, 0, 0, 0, "");
	m_bottombar->addBaseUIElement(btn);
	m_bottombarNavButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightTabButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	btn->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightTabButtons.push_back(btn);
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
	m_beatmaps = std::vector<OsuBeatmap*>(m_db->getBeatmaps()); // having a copy of the vector in here is actually completely unnecessary

	debugLog("OsuSongBrowser2::onDatabaseLoadingFinished() : %i beatmaps.\n", m_beatmaps.size());

	// build buttons
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		OsuUISongBrowserSongButton *songButton = new OsuUISongBrowserSongButton(m_osu, this, m_songBrowser, 250, 250 + m_beatmaps.size()*50, 200, 50, "", m_beatmaps[i]);

		m_songButtons.push_back(songButton);
		m_visibleSongButtons.push_back(songButton);
	}

	std::vector<OsuDatabase::Collection> collections = m_db->getCollections();
	for (int i=0; i<collections.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> children;
		for (int b=0; b<collections[i].beatmaps.size(); b++)
		{
			OsuBeatmap *beatmap = collections[i].beatmaps[b].first;
			std::vector<OsuBeatmapDifficulty*> colDiffs = collections[i].beatmaps[b].second;
			for (int sb=0; sb<m_songButtons.size(); sb++)
			{
				if (m_songButtons[sb]->getBeatmap() == beatmap)
				{
					std::vector<OsuUISongBrowserButton*> diffChildren = m_songButtons[sb]->getChildrenAbs();
					std::vector<OsuUISongBrowserButton*> matchingDiffs;

					for (int d=0; d<diffChildren.size(); d++)
					{
						OsuUISongBrowserSongButton *songButtonPointer = dynamic_cast<OsuUISongBrowserSongButton*>(diffChildren[d]);
						for (int cd=0; cd<colDiffs.size(); cd++)
						{
							if (songButtonPointer != NULL && songButtonPointer->getDiff() == colDiffs[cd])
								matchingDiffs.push_back(songButtonPointer);
						}
					}

					// HACKHACK: fuck/laziness
					if (matchingDiffs.size() != beatmap->getDifficultiesPointer()->size())
					{
						for (int md=0; md<matchingDiffs.size(); md++)
						{
							matchingDiffs[md]->setCollectionDiffHack(true);
						}
					}

					// TODO: only add matched diffs, instead of the whole beatmap (not supported by UI structure atm)
					/*
					if (matchingDiffs.size() > 1)
						children.push_back(m_songButtons[sb]);
					else if (matchingDiffs.size() == 1)
						children.push_back(matchingDiffs[0]);
					*/

					children.push_back(m_songButtons[sb]);

					break;
				}
			}
		}
		OsuUISongBrowserCollectionButton *collectionButton = new OsuUISongBrowserCollectionButton(m_osu, this, m_songBrowser, 250, 250 + m_beatmaps.size()*50, 200, 50, "", collections[i].name, children);

		m_collectionButtons.push_back(collectionButton);
		//m_visibleSongButtons.push_back(collectionButton); // for debugging only
	}

	// TODO: this would require all stars to already be calculated, i.e. a complete database.
	// would also need support for diff buttons without a parent, but thumbnail loading is a complete clusterfuck atm, as are the song button classes
	/*
	for (int i=0; i<12; i++)
	{
		UString difficultyCollectionName = UString::format(i == 1 ? "%i star" : "%i stars", i);
		if (i < 1)
			difficultyCollectionName = "Below 1 star";
		if (i > 10)
			difficultyCollectionName = "Above 10 stars";

		std::vector<OsuUISongBrowserButton*> children;

		OsuUISongBrowserDifficultyCollectionButton *b = new OsuUISongBrowserDifficultyCollectionButton(m_osu, this, m_songBrowser, 250, 250, 200, 50, "", difficultyCollectionName, children);
		m_difficultyCollectionButtons.push_back(b);
	}
	*/

	onSortChange(osu_songbrowser_sortingtype.getString());
	onSortScoresChange(osu_songbrowser_scores_sortingtype.getString());

	// update rich presence (discord total pp)
	OsuRichPresence::onSongBrowser(m_osu);

	// update user name/stats
	onUserButtonChange(m_name_ref->getString(), -1);
}

void OsuSongBrowser2::onSearchUpdate()
{
	m_bInSearch = (m_sSearchString.length() > 0);

	// empty the container
	m_songBrowser->getContainer()->empty();

	// rebuild visible song buttons, scroll to top search result
	m_visibleSongButtons.clear();
	if (m_bInSearch)
	{
		m_searchPrevGroup = m_group;

		// search for possible matches, add the children below the possibly visible currently selected song button (which owns them)
		switch (m_group)
		{
		case GROUP::GROUP_NO_GROUPING:
			for (int i=0; i<m_songButtons.size(); i++)
			{
				if (searchMatcher(m_songButtons[i]->getBeatmap(), m_sSearchString))
					m_visibleSongButtons.push_back(m_songButtons[i]);
			}
			break;

		case GROUP::GROUP_COLLECTIONS:
			for (int i=0; i<m_collectionButtons.size(); i++)
			{
				bool match = false;

				std::vector<OsuUISongBrowserButton*> &children = m_collectionButtons[i]->getChildrenAbs();
				for (int c=0; c<children.size(); c++)
				{
					const bool searchMatch = searchMatcher(children[c]->getBeatmap(), m_sSearchString);
					match |= searchMatch;
					children[c]->setCollectionSearchHack(searchMatch); // flag every match
				}

				if (match)
					m_visibleSongButtons.push_back(m_collectionButtons[i]);
			}
			break;
		}

		rebuildSongButtons();

		// scroll to top result, or select the only result
		if (m_visibleSongButtons.size() > 1)
			scrollToSongButton(m_visibleSongButtons[0]);
		else if (m_visibleSongButtons.size() > 0)
		{
			selectSongButton(m_visibleSongButtons[0]);
			m_songBrowser->scrollY(1);
		}
	}
	else // exit search
	{
		// reset match flag
		for (int i=0; i<m_collectionButtons.size(); i++)
		{
			std::vector<OsuUISongBrowserButton*> &children = m_collectionButtons[i]->getChildrenAbs();
			for (int c=0; c<children.size(); c++)
			{
				children[c]->setCollectionSearchHack(true);
			}
		}

		// remember which tab was selected, instead of defaulting back to no grouping
		switch (m_searchPrevGroup)
		{
		case GROUP::GROUP_NO_GROUPING:
			onGroupNoGrouping(m_noGroupingButton);
			break;

		case GROUP::GROUP_COLLECTIONS:
			onGroupCollections(m_collectionsButton);
			break;
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
		for (int i=0; i<scoreSortingMethods.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(scoreSortingMethods[i].name);
			if (scoreSortingMethods[i].name == osu_songbrowser_scores_sortingtype.getString())
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortScoresChange) );
}

void OsuSongBrowser2::onSortScoresChange(UString text, int id)
{
	osu_songbrowser_scores_sortingtype.setValue(text); // remember
	m_scoreSortButton->setText(text);
	rebuildScoreButtons();
	m_scoreBrowser->scrollToTop();
}

void OsuSongBrowser2::onWebClicked(CBaseUIButton *button)
{
	if (m_songInfo->getBeatmapID() > 0)
	{
		env->openURLInDefaultBrowser(UString::format("https://osu.ppy.sh/b/%ld", m_songInfo->getBeatmapID()));
		m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	}
}

void OsuSongBrowser2::onSortClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(button->getPos());
	m_contextMenu->setRelPos(button->getRelPos());
	m_contextMenu->begin(button->getSize().x);
	{
		for (int i=0; i<m_sortingMethods.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(m_sortingMethods[i].name);
			if (m_sortingMethods[i].type == m_sortingMethod)
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSortChange) );

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
	SORTING_METHOD *sortingMethod = (m_sortingMethods.size() > 3 ? &m_sortingMethods[3] : NULL);
	for (int i=0; i<m_sortingMethods.size(); i++)
	{
		// laziness wins again :(
		if (m_sortingMethods[i].name == text)
		{
			sortingMethod = &m_sortingMethods[i];
			break;
		}
	}
	if (sortingMethod == NULL) return;

	m_sortingMethod = sortingMethod->type;
	m_sortButton->setText(sortingMethod->name);
	osu_songbrowser_sortingtype.setValue(sortingMethod->name); // remember

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

	// resort Collection button array (each group of songbuttons inside each Collection)
	for (int i=0; i<m_collectionButtons.size(); i++)
	{
		std::vector<OsuUISongBrowserButton*> children = m_collectionButtons[i]->getChildrenAbs();
		std::sort(children.begin(), children.end(), comparatorWrapper);
		m_collectionButtons[i]->setChildren(children);
	}

	// we only need to update the visible buttons array if we are in no grouping (because Collections always get sorted by the collection name on the first level)
	// this will obviously change once new grouping options are added
	if (m_group == GROUP::GROUP_NO_GROUPING)
		m_visibleSongButtons = std::vector<OsuUISongBrowserButton*>(m_songButtons.begin(), m_songButtons.end());
	else if (m_group != GROUP::GROUP_COLLECTIONS)
		engine->showMessageError("Nope", "Group type is not yet implemented in OsuSongBrowser2::onSortChange()!");

	rebuildSongButtons();
	onAfterSortingOrGroupChange(NULL);
}

void OsuSongBrowser2::onGroupNoGrouping(CBaseUIButton *b)
{
	m_group = GROUP::GROUP_NO_GROUPING;

	m_visibleSongButtons = std::vector<OsuUISongBrowserButton*>(m_songButtons.begin(), m_songButtons.end());
	rebuildSongButtons();
	onAfterSortingOrGroupChange(b);
}

void OsuSongBrowser2::onGroupCollections(CBaseUIButton *b)
{
	m_group = GROUP::GROUP_COLLECTIONS;

	m_visibleSongButtons = std::vector<OsuUISongBrowserButton*>(m_collectionButtons.begin(), m_collectionButtons.end());
	rebuildSongButtons();
	onAfterSortingOrGroupChange(b);
}

void OsuSongBrowser2::onGroupDifficulty(CBaseUIButton *b)
{
	m_group = GROUP::GROUP_DIFFICULTY;

	m_visibleSongButtons = std::vector<OsuUISongBrowserButton*>(m_difficultyCollectionButtons.begin(), m_difficultyCollectionButtons.end());
	rebuildSongButtons();
	onAfterSortingOrGroupChange(b);
}

void OsuSongBrowser2::onAfterSortingOrGroupChange(CBaseUIButton *b)
{
	// keep search state consistent between tab changes
	if (m_bInSearch)
		onSearchUpdate();

	// highlight current
	for (int i=0; i<m_topbarRightTabButtons.size(); i++)
	{
		if (m_topbarRightTabButtons[i] == b)
			m_topbarRightTabButtons[i]->setTextBrightColor(COLOR(255, 0, 255, 0));
		else if (b != NULL)
			m_topbarRightTabButtons[i]->setTextBrightColor(COLOR(255, 255, 255, 255));
	}

	// if anything was selected, scroll to that. otherwise scroll to top
	std::vector<CBaseUIElement*> elements = m_songBrowser->getContainer()->getAllBaseUIElements();
	bool isAnythingSelected = false;
	for (int i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserButton *button = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
		if (button != NULL && button->isSelected())
		{
			isAnythingSelected = true;
			break;
		}
	}

	if (isAnythingSelected)
		scrollToSelectedSongButton();
	else
		m_songBrowser->scrollToTop();
}

void OsuSongBrowser2::onSelectionMode()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_contextMenu->setPos(m_bottombarNavButtons[0]->getPos());
	m_contextMenu->setRelPos(m_bottombarNavButtons[0]->getRelPos());
	m_contextMenu->begin();
	m_contextMenu->addButton("std");
	m_contextMenu->addButton("mania");
	m_contextMenu->setPos(m_contextMenu->getPos() - Vector2(0, m_contextMenu->getSize().y));
	m_contextMenu->setRelPos(m_contextMenu->getRelPos() - Vector2(0, m_contextMenu->getSize().y));
	m_contextMenu->end();
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
}

void OsuSongBrowser2::onModeChange(UString text)
{
	onModeChange2(text);
}

void OsuSongBrowser2::onModeChange2(UString text, int id)
{
	if (m_bottombarNavButtons.size() > 2)
		m_bottombarNavButtons[0]->setText(text);

	if (text == "std")
	{
		if (m_osu->getGamemode() != Osu::GAMEMODE::STD)
		{
			m_osu->setGamemode(Osu::GAMEMODE::STD);
			refreshBeatmaps();
		}
	}
	else if (text == "mania")
	{
		if (m_osu->getGamemode() != Osu::GAMEMODE::MANIA)
		{
			m_osu->setGamemode(Osu::GAMEMODE::MANIA);
			refreshBeatmaps();
		}
	}
}

void OsuSongBrowser2::onUserButtonClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	std::vector<UString> names = m_db->getPlayerNamesWithPPScores();
	if (names.size() > 0)
	{
		m_contextMenu->setPos(m_userButton->getPos());
		m_contextMenu->setRelPos(m_userButton->getPos());
		m_contextMenu->begin(m_userButton->getSize().x);
		m_contextMenu->addButton("Switch User", 0)->setTextColor(0xff888888)->setTextDarkColor(0xff000000)->setTextLeft(false)->setEnabled(false);
		//m_contextMenu->addButton("", 0)->setEnabled(false);
		for (int i=0; i<names.size(); i++)
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
		m_contextMenu->end(true);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onUserButtonChange) );
	}
}

void OsuSongBrowser2::onUserButtonChange(UString text, int id)
{
	if (id == 0) return;

	if (id == 1)
	{
		osu_songbrowser_draw_top_ranks_available_info_message.setValue(0.0f);
		m_osu->toggleUserStatsScreen();
		return;
	}

	m_name_ref->setValue(text);
	m_osu->getOptionsMenu()->setUsername(text); // force update textbox to avoid shutdown inconsistency
	m_userButton->setText(text);

	m_userButton->updateUserStats();
}

void OsuSongBrowser2::onScoreClicked(CBaseUIButton *button)
{
	OsuUISongBrowserScoreButton *scoreButton = (OsuUISongBrowserScoreButton*)button;

	m_osu->getRankingScreen()->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty());
	m_osu->getRankingScreen()->setScore(scoreButton->getScore(), scoreButton->getDateTime());
	m_osu->getSongBrowser()->setVisible(false);
	m_osu->getRankingScreen()->setVisible(true);
}

void OsuSongBrowser2::onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, UString text)
{
	if (text == "Delete Score")
	{
		m_db->deleteScore(std::string(scoreButton->getName().toUtf8()), scoreButton->getScoreUnixTimestamp());

		rebuildScoreButtons();
		m_userButton->updateUserStats();
	}
}

void OsuSongBrowser2::highlightScore(uint64_t unixTimestamp)
{
	for (int i=0; i<m_scoreButtonCache.size(); i++)
	{
		if (m_scoreButtonCache[i]->getScore().unixTimestamp == unixTimestamp)
		{
			m_scoreBrowser->scrollToElement(m_scoreButtonCache[i], 0, 10);
			m_scoreButtonCache[i]->highlight();
			break;
		}
	}
}

void OsuSongBrowser2::selectSongButton(OsuUISongBrowserButton *songButton)
{
	if (songButton != NULL && !songButton->isSelected())
		songButton->select();
}

void OsuSongBrowser2::selectRandomBeatmap()
{
	// filter songButtons
	std::vector<CBaseUIElement*> elements = m_songBrowser->getContainer()->getAllBaseUIElements();
	std::vector<OsuUISongBrowserSongButton*> songButtons;
	for (int i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>(elements[i]);
		if (songButton != NULL && songButton->getBeatmap() != NULL)	// only allow songButtons
			songButtons.push_back(songButton);
	}

	if (songButtons.size() < 1) return;

	// remember previous
	if (m_previousRandomBeatmaps.size() == 0 && m_selectedBeatmap != NULL)
		m_previousRandomBeatmaps.push_back(m_selectedBeatmap);

	std::uniform_int_distribution<int> rng(0, songButtons.size()-1);
	int randomIndex = rng(m_rngalg);
	OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>(songButtons[randomIndex]);
	selectSongButton(songButton);
}

void OsuSongBrowser2::selectPreviousRandomBeatmap()
{
	if (m_previousRandomBeatmaps.size() > 0)
	{
		OsuBeatmap *currentRandomBeatmap = m_previousRandomBeatmaps.back();
		if (m_previousRandomBeatmaps.size() > 1 && m_previousRandomBeatmaps[m_previousRandomBeatmaps.size()-1] == m_selectedBeatmap)
			m_previousRandomBeatmaps.pop_back(); // deletes the current beatmap which may also be at the top (so we don't switch to ourself)

		// filter songButtons
		std::vector<CBaseUIElement*> elements = m_songBrowser->getContainer()->getAllBaseUIElements();
		std::vector<OsuUISongBrowserSongButton*> songButtons;
		for (int i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserSongButton *songButton = dynamic_cast<OsuUISongBrowserSongButton*>(elements[i]);
			if (songButton != NULL && songButton->getBeatmap() != NULL)	// only allow songButtons
				songButtons.push_back(songButton);
		}

		// select it, if we can find it (and remove it from memory)
		bool foundIt = false;
		OsuBeatmap *previousRandomBeatmap = m_previousRandomBeatmaps.back();
		for (int i=0; i<songButtons.size(); i++)
		{
			if (songButtons[i]->getBeatmap() != NULL && songButtons[i]->getBeatmap() == previousRandomBeatmap)
			{
				m_previousRandomBeatmaps.pop_back();
				selectSongButton(songButtons[i]);
				foundIt = true;
				break;
			}
		}

		// if we didn't find it then restore the current random beatmap, which got pop_back()'d above (shit logic)
		if (!foundIt)
			m_previousRandomBeatmaps.push_back(currentRandomBeatmap);
	}
}

void OsuSongBrowser2::playSelectedDifficulty()
{
	std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
	for (int i=0; i<elements->size(); i++)
	{
		OsuUISongBrowserSongDifficultyButton *songDifficultyButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>((*elements)[i]);
		if (songDifficultyButton != NULL && songDifficultyButton->isSelected())
		{
			songDifficultyButton->select();
			break;
		}
	}
}
