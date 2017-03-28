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
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuNotificationOverlay.h"
#include "OsuModSelector.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"
#include "OsuUIContextMenu.h"
#include "OsuUISelectionButton.h"
#include "OsuUISongBrowserInfoLabel.h"
#include "OsuUISongBrowserSongButton.h"
#include "OsuUISongBrowserSongDifficultyButton.h"
#include "OsuUISongBrowserCollectionButton.h"

ConVar osu_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f);
ConVar osu_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f);
ConVar osu_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f);
ConVar osu_songbrowser_topbar_right_height_percent("osu_songbrowser_topbar_right_height_percent", 0.5f);
ConVar osu_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.378f);
ConVar osu_songbrowser_bottombar_percent("osu_songbrowser_bottombar_percent", 0.116f);



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



struct SortByArtist : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByArtist() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		std::wstring artistLowercase1 = std::wstring(a->getBeatmap()->getArtist().wc_str());
		std::wstring artistLowercase2 = std::wstring(b->getBeatmap()->getArtist().wc_str());
		std::transform(artistLowercase1.begin(), artistLowercase1.end(), artistLowercase1.begin(), std::towlower);
		std::transform(artistLowercase2.begin(), artistLowercase2.end(), artistLowercase2.begin(), std::towlower);

		return artistLowercase1 < artistLowercase2;
	}
};

struct SortByBPM : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByBPM() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
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

		return bpm1 < bpm2;
	}
};

struct SortByCreator : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByCreator() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		std::wstring creatorLowercase1;
		std::wstring creatorLowercase2;
		std::vector<OsuBeatmapDifficulty*> *aDiffs = a->getBeatmap()->getDifficultiesPointer();
		if (aDiffs->size() > 0)
			creatorLowercase1 = std::wstring((*aDiffs)[aDiffs->size()-1]->creator.wc_str());

		std::vector<OsuBeatmapDifficulty*> *bDiffs = b->getBeatmap()->getDifficultiesPointer();
		if (bDiffs->size() > 0)
			creatorLowercase2 = std::wstring((*bDiffs)[bDiffs->size()-1]->creator.wc_str());

		std::transform(creatorLowercase1.begin(), creatorLowercase1.end(), creatorLowercase1.begin(), std::towlower);
		std::transform(creatorLowercase2.begin(), creatorLowercase2.end(), creatorLowercase2.begin(), std::towlower);

		return creatorLowercase1 < creatorLowercase2;
	}
};

struct SortByDateAdded : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByDateAdded() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
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

		return time1 > time2;
	}
};

struct SortByDifficulty : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByDifficulty() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
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
			return stars1 < stars2;
		else
			return diff1 < diff2;
	}
};

struct SortByLength : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByLength() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
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

		return length1 < length2;
	}
};

struct SortByTitle : public OsuSongBrowser2::SORTING_COMPARATOR
{
	virtual ~SortByTitle() {;}
	bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
	{
		std::wstring titleLowercase1 = std::wstring(a->getBeatmap()->getTitle().wc_str());
		std::wstring titleLowercase2 = std::wstring(b->getBeatmap()->getTitle().wc_str());
		std::transform(titleLowercase1.begin(), titleLowercase1.end(), titleLowercase1.begin(), std::towlower);
		std::transform(titleLowercase2.begin(), titleLowercase2.end(), titleLowercase2.begin(), std::towlower);

		return titleLowercase1 < titleLowercase2;
	}
};



OsuSongBrowser2::OsuSongBrowser2(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	m_rngalg = std::mt19937(time(0));
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

	// engine settings
	engine->getMouse()->addListener(this);

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

	// build topbar right
	m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");
	m_groupLabel = new CBaseUILabel(0, 0, 0, 0, "", "Group:");
	m_groupLabel->setSizeToContent(3);
	m_groupLabel->setDrawFrame(false);
	m_groupLabel->setDrawBackground(false);
	m_topbarRight->addBaseUIElement(m_groupLabel);

	addTopBarRightTabButton("Collections")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onGroupCollections) );
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
	addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMods) );
	addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionRandom) );
	///addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser::onSelectionOptions) );

	// build songbrowser
	m_songBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_songBrowser->setDrawBackground(false);
	m_songBrowser->setDrawFrame(false);
	m_songBrowser->setHorizontalScrolling(false);
	m_songBrowser->setScrollResistance(m_osu->isInVRMode() ? convar->getConVarByName("ui_scrollview_resistance")->getInt() : 15); // a bit shitty this check + convar, but works well enough

	// beatmap database
	m_db = new OsuDatabase(m_osu);
	m_bBeatmapRefreshScheduled = true;

	// behaviour
	m_bHasSelectedAndIsPlaying = false;
	m_selectedBeatmap = NULL;
	m_fPulseAnimation = 0.0f;

	// search
	m_fSearchWaitTime = 0.0f;
	m_bInSearch = false;

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

	SAFE_DELETE(m_topbarLeft);
	SAFE_DELETE(m_topbarRight);
	SAFE_DELETE(m_bottombar);
	SAFE_DELETE(m_songBrowser);
	SAFE_DELETE(m_db);
}

void OsuSongBrowser2::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	// refreshing (blocks every other call in draw() below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffffffff);
		UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(m_db->getProgress()*100.0f));
		g->pushTransform();
		g->translate(m_osu->getScreenWidth()/2 - m_osu->getTitleFont()->getStringWidth(loadingMessage)/2, m_osu->getScreenHeight() - 15);
		g->drawString(m_osu->getTitleFont(), loadingMessage);
		g->popTransform();

		m_osu->getHUD()->drawBeatmapImportSpinner(g);
		return;
	}

	// draw background
	g->setColor(0xff000000);
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());

	// draw background image
	drawSelectedBeatmapBackgroundImage(g, m_osu);

	// draw song browser
	m_songBrowser->draw(g);

	// draw search text and background
	UString searchText1 = "Search: ";
	UString searchText2 = "Type to search!";
	UString combinedSearchText = searchText1;
	combinedSearchText.append(searchText2);
	McFont *searchTextFont = m_osu->getSubTitleFont();
	float searchTextScale = 0.75f;
	bool hasSearchSubTextVisible = m_sSearchString.length() > 0 && m_bInSearch;
	g->setColor(COLOR(m_sSearchString.length() > 0 ? 100 : 30, 0, 0, 0));
	g->fillRect(m_songBrowser->getPos().x + m_songBrowser->getSize().x*0.75f - searchTextFont->getStringWidth(combinedSearchText)*searchTextScale/2 - (searchTextFont->getHeight()*searchTextScale)*0.5f, m_songBrowser->getPos().y, m_songBrowser->getSize().x, (searchTextFont->getHeight()*searchTextScale)*(hasSearchSubTextVisible ? 4.0f : 3.0f));
	g->setColor(0xffffffff);
	g->pushTransform();
		g->translate(0, searchTextFont->getHeight()/2);
		g->scale(searchTextScale, searchTextScale);
		g->translate(m_songBrowser->getPos().x + m_songBrowser->getSize().x*0.75f - searchTextFont->getStringWidth(combinedSearchText)*searchTextScale/2, m_songBrowser->getPos().y + (searchTextFont->getHeight()*searchTextScale)*1.5f);

		// draw search text and text
		g->pushTransform();
			g->setColor(0xff00ff00);
			g->drawString(searchTextFont, searchText1);
			g->setColor(0xffffffff);
			g->translate(searchTextFont->getStringWidth(searchText1)*searchTextScale, 0);
			if (m_sSearchString.length() < 1)
				g->drawString(searchTextFont, searchText2);
			else
				g->drawString(searchTextFont, m_sSearchString);
		g->popTransform();

		// draw number of matches
		if (hasSearchSubTextVisible)
		{
			g->setColor(0xffffffff);
			g->translate(0, (searchTextFont->getHeight()*searchTextScale)*1.5f);
			g->drawString(searchTextFont, m_visibleSongButtons.size() > 0 ? UString::format("%i matches found!", m_visibleSongButtons.size()) : "No matches found. Hit ESC to reset.");
		}
	g->popTransform();

	// draw top bar
	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(m_fSongSelectTopScale, m_fSongSelectTopScale);
		g->translate((m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale)/2, (m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale)/2);
		g->drawImage(m_osu->getSkin()->getSongSelectTop());
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
		g->scale(songSelectBottomScale, songSelectBottomScale);
		g->translate(0, (int)(m_bottombar->getPos().y) + (int)((m_osu->getSkin()->getSongSelectBottom()->getHeight()*songSelectBottomScale)/2) - 1);
		m_osu->getSkin()->getSongSelectBottom()->bind();
		g->drawQuad(0, -(int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)/2), (int)(m_osu->getScreenWidth()*(1.0f/songSelectBottomScale)), (int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)));
	g->popTransform();

	m_bottombar->draw(g);
	OsuScreenBackable::draw(g);
	if (Osu::debug->getBool())
		m_bottombar->draw_debug(g);

	// no beatmaps found (osu folder is probably invalid)
	if (m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffff0000);
		UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
		errorMessage1.append(m_sLastOsuFolder);
		UString errorMessage2 = "Go to Options -> osu!folder";
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage1)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()));
		g->drawString(m_osu->getSubTitleFont(), errorMessage1);
		g->popTransform();

		g->setColor(0xff00ff00);
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage2)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()*2 + 15));
		g->drawString(m_osu->getSubTitleFont(), errorMessage2);
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

void OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(Graphics *g, Osu *osu)
{
	if (osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		Image *backgroundImage = osu->getSelectedBeatmap()->getSelectedDifficulty()->backgroundImage;
		if (backgroundImage != NULL)
		{
			float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

			g->setColor(0xff999999);
			g->pushTransform();
			g->scale(scale, scale);
			g->translate(osu->getScreenWidth()/2, osu->getScreenHeight()/2);
			g->drawImage(backgroundImage);
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

	m_songBrowser->update();
	m_songBrowser->getContainer()->update_pos(); // necessary due to constant animations
	m_topbarLeft->update();
	m_topbarRight->update();
	m_bottombar->update();
	m_contextMenu->update();

	if (m_contextMenu->isMouseInside())
	{
		m_topbarRight->stealFocus();
		m_songBrowser->stealFocus();
	}

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
		m_bInSearch = true;

		// empty the container
		m_songBrowser->getContainer()->empty();

		// rebuild visible song buttons, scroll to top search result
		m_visibleSongButtons.clear();
		if (m_sSearchString.length() > 0)
		{
			// search for possible matches, add the children below the possibly visible currently selected song button (which owns them)
			for (int i=0; i<m_songButtons.size(); i++)
			{
				m_songButtons[i]->setVisible(false); // unload images

				if (searchMatcher(m_songButtons[i]->getBeatmap(), m_sSearchString))
					m_visibleSongButtons.push_back(m_songButtons[i]);
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
		else
		{
			// TODO: remember which tab was selected, instead of defaulting back to no grouping
			onGroupNoGrouping(m_noGroupingButton);
		}
	}
}

void OsuSongBrowser2::onKeyDown(KeyboardEvent &key)
{
	if (!m_bVisible) return;

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
			m_sSearchString = "";
			scheduleSearchUpdate(true);
			break;
		}
	}
	else
	{
		if (key == KEY_ESCAPE)
			m_osu->toggleSongBrowser();
	}

	if (key == KEY_SHIFT)
		m_bShiftPressed = true;

	// function hotkeys
	if (key == KEY_F1 && !m_bF1Pressed)
	{
		m_bF1Pressed = true;
		m_bottombarNavButtons[0]->keyboardPulse();
		onSelectionMods();
	}
	if (key == KEY_F2 && !m_bF2Pressed)
	{
		m_bF2Pressed = true;
		m_bottombarNavButtons[1]->keyboardPulse();
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
	if (e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown()))
		return;

	// handle searching
	KEYCODE charCode = e.getCharCode();
	m_sSearchString.append(UString((wchar_t*)&charCode));

	scheduleSearchUpdate();
}

void OsuSongBrowser2::onLeftChange(bool down)
{
	if (!m_bVisible) return;
}

void OsuSongBrowser2::onRightChange(bool down)
{
	if (!m_bVisible) return;
}

void OsuSongBrowser2::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);
}

void OsuSongBrowser2::onPlayEnd(bool quit)
{
	m_bHasSelectedAndIsPlaying = false;
}

void OsuSongBrowser2::onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool play)
{
	// remember it
	if (beatmap != m_selectedBeatmap)
		m_previousRandomBeatmaps.push_back(beatmap);

	m_selectedBeatmap = beatmap;

	// update song info
	m_songInfo->setFromBeatmap(beatmap, diff);

	// start playing
	if (play)
	{
		m_osu->onBeforePlayStart();
		if (beatmap->play())
		{
			m_bHasSelectedAndIsPlaying = true;
			setVisible(false);

			m_osu->onPlayStart();
		}
	}

	// animate
	m_fPulseAnimation = 1.0f;
	anim->moveLinear(&m_fPulseAnimation, 0.0f, 0.55f, true);

	// notify mod selector (for BPM override slider, else we get inconsistent values)
	m_osu->getModSelector()->checkUpdateBPMSliderSlaves();
}

void OsuSongBrowser2::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying)
		return;

	// reset
	m_selectedBeatmap = NULL;

	// delete database
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
			// HACKHACK: fuck
			if (songButton->getCollectionDiffHack())
			{
				if (m_group == GROUP::GROUP_COLLECTIONS)
					songButton->setInactiveBackgroundColor(COLOR(255, 233, 104, 0));
				else
					songButton->setInactiveBackgroundColor(COLOR(255, 0, 150, 236));
			}

			// depending on the object type, layout differently
			bool isCollectionButton = dynamic_cast<OsuUISongBrowserCollectionButton*>(songButton) != NULL;
			bool isDiffButton = dynamic_cast<OsuUISongBrowserSongDifficultyButton*>(songButton) != NULL;

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
			songButton->updateLayout();

			yCounter += songButton->getActualSize().y;
		}
	}
	m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y/2);
}

void OsuSongBrowser2::setVisible(bool visible)
{
	m_bVisible = visible;
	m_bShiftPressed = false; // this seems to get stuck sometimes otherwise

	if (m_bVisible)
	{
		updateLayout();

		// we have to re-select the current beatmap to start playing music again
		if (m_selectedBeatmap != NULL)
			m_selectedBeatmap->select();

		m_bHasSelectedAndIsPlaying = false; // sanity

		// try another refresh, maybe the osu!folder has changed
		if (m_beatmaps.size() == 0)
			refreshBeatmaps();
	}
}

bool OsuSongBrowser2::searchMatcher(OsuBeatmap *beatmap, UString searchString)
{
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
		std::pair<UString, keywordId>("stars", STARS)
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
									compareValue = diffs[d]->starsNoMod;
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

	// if we have a valid literal string to search, do that, else just return the expression match
	if (literalSearchString.length() > 0)
	{
		//debugLog("literalSearchString = %s\n", literalSearchString.toUtf8());
		for (int i=0; i<diffs.size(); i++)
		{
			if (findSubstringInDifficulty(diffs[i], literalSearchString))
				return true;
		}
		return false; // can't be matched anymore
	}
	else
		return expressionMatches;
}

bool OsuSongBrowser2::findSubstringInDifficulty(OsuBeatmapDifficulty *diff, UString searchString)
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
	m_bottombarNavButtons[0]->setImageResourceName(m_osu->getSkin()->getSelectionMods()->getName());
	m_bottombarNavButtons[0]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModsOver()->getName());
	m_bottombarNavButtons[1]->setImageResourceName(m_osu->getSkin()->getSelectionRandom()->getName());
	m_bottombarNavButtons[1]->setImageResourceNameOver(m_osu->getSkin()->getSelectionRandomOver()->getName());
	/*
	m_bottombarNavButtons[0]->setImageResourceName(m_osu->getSkin()->getSelectionMode()->getName());
	m_bottombarNavButtons[0]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModeOver()->getName());
	m_bottombarNavButtons[1]->setImageResourceName(m_osu->getSkin()->getSelectionMods()->getName());
	m_bottombarNavButtons[1]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModsOver()->getName());
	m_bottombarNavButtons[2]->setImageResourceName(m_osu->getSkin()->getSelectionRandom()->getName());
	m_bottombarNavButtons[2]->setImageResourceNameOver(m_osu->getSkin()->getSelectionRandomOver()->getName());
	*/
	///m_bottombarNavButtons[2]->setImageResourceName(m_osu->getSkin()->getSelectionOptions()->getName());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	const int margin = 5;

	// top bar
	m_fSongSelectTopScale = Osu::getImageScaleToFitResolution(m_osu->getSkin()->getSongSelectTop(), m_osu->getScreenSize());
	float songSelectTopHeightScaled = std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale, m_songInfo->getMinimumHeight()*1.5f + margin); // NOTE: the height is a heuristic here more or less
	m_fSongSelectTopScale = std::max(m_fSongSelectTopScale, songSelectTopHeightScaled / m_osu->getSkin()->getSongSelectTop()->getHeight());

	// topbar left
	m_topbarLeft->setSize(std::max(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_width_percent.getFloat() + margin, m_songInfo->getMinimumWidth() + margin), std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_percent.getFloat(), m_songInfo->getMinimumHeight() + margin));
	m_songInfo->setRelPos(margin, margin);
	m_songInfo->setSize(m_topbarLeft->getSize().x - margin, std::max(m_topbarLeft->getSize().y*0.65f - margin, m_songInfo->getMinimumHeight()));

	// topbar right
	m_topbarRight->setPosX(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_percent.getFloat());
	m_topbarRight->setSize(m_osu->getScreenWidth() - m_topbarRight->getPos().x, m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_height_percent.getFloat());

	int topbarRightTabButtonMargin = 10;
	int topbarRightTabButtonHeight = 30;
	int topbarRightTabButtonWidth = clamp<float>((float)(m_topbarRight->getSize().x - 2*topbarRightTabButtonMargin) / (float)m_topbarRightTabButtons.size(), 0.0f, 200.0f);
	for (int i=0; i<m_topbarRightTabButtons.size(); i++)
	{
		m_topbarRightTabButtons[i]->setSize(topbarRightTabButtonWidth, topbarRightTabButtonHeight);
		m_topbarRightTabButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightTabButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightTabButtonWidth), m_topbarRight->getSize().y-m_topbarRightTabButtons[i]->getSize().y);
	}

	if (m_topbarRightTabButtons.size() > 0)
		m_groupLabel->setRelPos(m_topbarRightTabButtons[0]->getRelPos() + Vector2(-m_groupLabel->getSize().x, m_topbarRightTabButtons[0]->getSize().y/2.0f - m_groupLabel->getSize().y/2.0f));

	int topbarRightSortButtonMargin = 10;
	int topbarRightSortButtonHeight = 30;
	int topbarRightSortButtonWidth = clamp<float>((float)(m_topbarRight->getSize().x - 2*topbarRightSortButtonMargin) / (float)m_topbarRightSortButtons.size(), 0.0f, 200.0f);
	for (int i=0; i<m_topbarRightSortButtons.size(); i++)
	{
		m_topbarRightSortButtons[i]->setSize(topbarRightSortButtonWidth, topbarRightSortButtonHeight);
		m_topbarRightSortButtons[i]->setRelPos(m_topbarRight->getSize().x - (topbarRightSortButtonMargin + (m_topbarRightTabButtons.size()-i)*topbarRightSortButtonWidth), topbarRightSortButtonMargin);
	}

	if (m_topbarRightSortButtons.size() > 0)
		m_sortLabel->setRelPos(m_topbarRightSortButtons[m_topbarRightSortButtons.size()-1]->getRelPos() + Vector2(-m_sortLabel->getSize().x, m_topbarRightSortButtons[m_topbarRightSortButtons.size()-1]->getSize().y/2.0f - m_sortLabel->getSize().y/2.0f));

	m_topbarRight->update_pos();

	// bottombar
	int bottomBarHeight = m_osu->getScreenHeight()*osu_songbrowser_bottombar_percent.getFloat();

	m_bottombar->setPosY(m_osu->getScreenHeight() - bottomBarHeight);
	m_bottombar->setSize(m_osu->getScreenWidth(), bottomBarHeight);

	// nav bar
	float navBarStart = std::max(m_osu->getScreenWidth()*0.175f, m_backButton->getSize().x);
	if (m_osu->getScreenWidth()*0.175f < m_backButton->getSize().x + 25)
		navBarStart += m_backButton->getSize().x - m_osu->getScreenWidth()*0.175f;

	// bottombar cont
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(m_osu->getScreenWidth(), bottomBarHeight);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setRelPosX((i == 0 ? navBarStart : 0) + (i > 0 ? m_bottombarNavButtons[i-1]->getRelPos().x + m_bottombarNavButtons[i-1]->getSize().x : 0));
	}
	m_bottombar->update_pos();

	// song browser
	m_songBrowser->setPos(m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + 1, m_topbarRight->getPos().y + m_topbarRight->getSize().y + 2);
	m_songBrowser->setSize(m_osu->getScreenWidth() - (m_topbarLeft->getPos().x + m_topbarLeft->getSize().x), m_osu->getScreenHeight() - m_songBrowser->getPos().y - m_bottombar->getSize().y+2);
	updateSongButtonLayout();
}

void OsuSongBrowser2::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
}

void OsuSongBrowser2::scheduleSearchUpdate(bool immediately)
{
	m_fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : 0.5f);
}

OsuUISelectionButton *OsuSongBrowser2::addBottombarNavButton()
{
	OsuUISelectionButton *btn = new OsuUISelectionButton("MISSING_TEXTURE", 0, 0, 0, 0, "");
	btn->setScaleToFit(true);
	m_bottombar->addBaseUIElement(btn);
	m_bottombarNavButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightTabButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightTabButtons.push_back(btn);
	return btn;
}

CBaseUIButton *OsuSongBrowser2::addTopBarRightSortButton(UString text)
{
	CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
	m_topbarRight->addBaseUIElement(btn);
	m_topbarRightSortButtons.push_back(btn);
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

					// TODO: only add matched diffs, instead of the whole beatmap
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

	// TODO:
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
	/*
	for (int i=0; i<tempDifficultyCollectionSongButtons.size(); i++)
	{

	}
	*/

	onSortChange(m_sortingMethods[3].name); // hardcoded to use "By Date Added" as the default sorting method
}

void OsuSongBrowser2::onSortClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(button->getPos());
	m_contextMenu->setRelPos(button->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<m_sortingMethods.size(); i++)
	{
		CBaseUIButton *button = m_contextMenu->addButton(m_sortingMethods[i].name);
		if (m_sortingMethods[i].type == m_sortingMethod)
			button->setTextBrightColor(0xff00ff00);
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

void OsuSongBrowser2::onSortChange(UString text)
{
	SORTING_METHOD sortingMethod;
	for (int i=0; i<m_sortingMethods.size(); i++)
	{
		// laziness wins again :(
		if (m_sortingMethods[i].name == text)
		{
			sortingMethod = m_sortingMethods[i];
			break;
		}
	}

	m_sortingMethod = sortingMethod.type;
	m_sortButton->setText(sortingMethod.name);

	struct COMPARATOR_WRAPPER
	{
		SORTING_COMPARATOR *comp;
		bool operator() (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const
		{
			return comp->operator()(a, b);
		}
	};
	COMPARATOR_WRAPPER comparatorWrapper;
	comparatorWrapper.comp = sortingMethod.comparator;

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

void OsuSongBrowser2::onAfterSortingOrGroupChange(CBaseUIButton *b)
{
	// delete possible search & text
	m_bInSearch = false;
	m_sSearchString = "";

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
	{
		m_songBrowser->scrollToTop();
	}
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

	if (songButtons.size() < 1)
		return;

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
