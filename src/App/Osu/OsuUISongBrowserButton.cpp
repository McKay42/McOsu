//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser element (beatmap, diff, collection, group, etc.)
//
// $NoKeywords: $osusbb
//===============================================================================//

#include "OsuUISongBrowserButton.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"

OsuUISongBrowserButton *OsuUISongBrowserButton::previousParent = NULL;
OsuUISongBrowserButton *OsuUISongBrowserButton::previousChild = NULL;
int OsuUISongBrowserButton::marginPixelsX = 9;
int OsuUISongBrowserButton::marginPixelsY = 9;
float OsuUISongBrowserButton::thumbnailYRatio = 1.333333f;
float OsuUISongBrowserButton::lastHoverSoundTime = 0;
Color OsuUISongBrowserButton::inactiveBackgroundColor = COLOR(255, 235, 73, 153); // pink
Color OsuUISongBrowserButton::activeBackgroundColor = COLOR(255, 255, 255, 255); // white
Color OsuUISongBrowserButton::inactiveDifficultyBackgroundColor = COLOR(255, 0, 150, 236); // blue

OsuUISongBrowserButton::OsuUISongBrowserButton(OsuBeatmap *beatmap, CBaseUIScrollView *view, OsuSongBrowser2 *songBrowser, float xPos, float yPos, float xSize, float ySize, UString name, OsuUISongBrowserButton *parent, OsuBeatmapDifficulty *child, bool isFirstChild) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_beatmap = beatmap;
	m_view = view;
	m_songBrowser = songBrowser;
	m_parent = parent;
	m_child = child;
	m_bIsFirstChild = isFirstChild;
	m_font = beatmap->getOsu()->getSongBrowserFont();
	m_fontBold = beatmap->getOsu()->getSongBrowserFontBold();

	// force loading
	m_bVisible = false;

	/*
	m_sTitle = "Title";
	m_sArtist = "Artist";
	m_sMapper = "Mapper";
	m_sDiff = "Difficulty";
	*/

	OsuBeatmapDifficulty *diff = isChild() ? m_child : m_beatmap->getDifficulties()[0];

	m_sTitle = diff->title;
	m_sArtist = diff->artist;
	m_sMapper = diff->creator;
	if (isChild())
		m_sDiff = diff->name;

	m_bSelected = false;
	m_fScale = 1.0f;

	m_fTextSpacingScale = 0.075f;
	m_fTextMarginScale = 0.075f;
	m_fTitleScale = 0.22f;
	m_fSubTitleScale = 0.14f;
	m_fDiffScale = 0.18f;

	m_fImageLoadScheduledTime = 0.0f;
	m_fHoverOffsetAnimation = 0.0f;
	m_fCenterOffsetAnimation = 0.0f;
	m_fCenterOffsetVelocityAnimation = 0.0f;

	// build children
	if (!isChild())
	{
		// sort difficulties by difficulty
		std::vector<OsuBeatmapDifficulty*> difficulties = m_beatmap->getDifficulties();
		struct SortComparator
		{
		    bool operator() (OsuBeatmapDifficulty const *a, OsuBeatmapDifficulty const *b) const
		    {
		    	const unsigned long diff1 = (a->AR+1)*(a->CS+1)*(a->HP+1)*(a->OD+1)*(a->maxBPM > 0 ? a->maxBPM : 1);
		    	const unsigned long diff2 = (b->AR+1)*(b->CS+1)*(b->HP+1)*(b->OD+1)*(b->maxBPM > 0 ? b->maxBPM : 1);

		        return diff1 < diff2;
		    }
		};
		std::sort(difficulties.begin(), difficulties.end(), SortComparator());

		// and add them
		for (int i=0; i<difficulties.size(); i++)
		{
			OsuUISongBrowserButton *songButton = new OsuUISongBrowserButton(beatmap, m_view, m_songBrowser, 0, 0, 0, 0, "", this, difficulties[i], i == 0);
			m_children.push_back(songButton);
		}
	}

	updateLayout();
}

OsuUISongBrowserButton::~OsuUISongBrowserButton()
{
	if (!m_bSelected)
	{
		for (int i=0; i<m_children.size(); i++)
		{
			delete m_children[i];
		}
		m_children.clear();
	}

	previousParent = NULL;
	previousChild = NULL;

	deleteAnimations();
}

void OsuUISongBrowserButton::deleteAnimations()
{
	anim->deleteExistingAnimation(&m_fCenterOffsetAnimation);
	anim->deleteExistingAnimation(&m_fCenterOffsetVelocityAnimation);
	anim->deleteExistingAnimation(&m_fHoverOffsetAnimation);
}

void OsuUISongBrowserButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	OsuSkin *skin = m_beatmap->getSkin();

	// debug bounding box
	/*
	g->setColor(0xffff0000);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	*/

	// build strings
	UString titleText = buildTitleString();
	UString subTitleText = buildSubTitleString();
	UString diffText = buildDiffString();

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	// draw menu button background
	g->setColor(m_bSelected ? activeBackgroundColor : (isChild() ? inactiveDifficultyBackgroundColor : inactiveBackgroundColor));
	g->pushTransform();
	g->scale(m_fScale, m_fScale);
	g->translate(m_vPos.x + m_vSize.x/2, m_vPos.y + m_vSize.y/2);
	g->drawImage(skin->getMenuButtonBackground());
	g->popTransform();

	// draw beatmap background
	Image *beatmapBackground = isChild() ? m_child->backgroundImage : (m_beatmap->getDifficulties().size() > 0 ? m_beatmap->getDifficulties()[0]->backgroundImage : NULL);
	int textXOffset = size.y*thumbnailYRatio + size.x*0.02f;
	if (beatmapBackground != NULL)
	{
		float beatmapBackgroundScale = Osu::getImageScaleToFillResolution(beatmapBackground, Vector2(size.y*thumbnailYRatio, size.y))*1.05f;

		Vector2 centerOffset = Vector2((size.y*thumbnailYRatio)/2, size.y/2);
		Rect clipRect = Rect(pos.x-1, pos.y + 2, (int)(size.y*thumbnailYRatio)+4, size.y+1);

		g->setColor(0xffffffff);
		g->pushTransform();
		g->scale(beatmapBackgroundScale, beatmapBackgroundScale);
		g->translate(pos.x + (int)centerOffset.x, pos.y + (int)centerOffset.y);
		g->pushClipRect(clipRect);
		g->drawImage(beatmapBackground);
		g->popClipRect();
		g->popTransform();

		// debug cliprect bounding box
		/*
		Vector2 clipRectPos = Vector2(clipRect.getX(), clipRect.getY());
		Vector2 clipRectSize = Vector2(clipRect.getWidth(), clipRect.getHeight());

		g->setColor(0xffffff00);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x, clipRectPos.y+clipRectSize.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x+clipRectSize.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
		*/
	}

	// draw
	float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
	if (isChild() && !m_bSelected)
		g->setAlpha(0.2f);
	g->pushTransform();
	g->scale(titleScale, titleScale);
	g->translate(pos.x + textXOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale);
	g->drawString(m_font, titleText);
	g->popTransform();

	float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
	if (isChild() && !m_bSelected)
		g->setAlpha(0.2f);
	g->pushTransform();
	g->scale(subTitleScale, subTitleScale);
	g->translate(pos.x + textXOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale);
	g->drawString(m_font, subTitleText);
	g->popTransform();

	if (isChild())
	{
		float diffScale = (size.y*m_fDiffScale) / m_fontBold->getHeight();
		g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
		g->pushTransform();
		g->scale(diffScale, diffScale);
		g->translate(pos.x + textXOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale + size.y*m_fTextSpacingScale + m_fontBold->getHeight()*diffScale);
		g->drawString(m_fontBold, diffText);
		g->popTransform();
	}

	// debug inner bounding box
	/*
	g->setColor(0xffff00ff);
	g->drawLine(pos.x, pos.y, pos.x+size.x, pos.y);
	g->drawLine(pos.x, pos.y, pos.x, pos.y+size.y);
	g->drawLine(pos.x, pos.y+size.y, pos.x+size.x, pos.y+size.y);
	g->drawLine(pos.x+size.x, pos.y, pos.x+size.x, pos.y+size.y);
	*/
}

void OsuUISongBrowserButton::update()
{
	// HACKHACK: DAYUM SON this is literally hitler
	// temporarily fool CBaseUIElement with modified position and size
	Vector2 posBackup = m_vPos;
	Vector2 sizeBackup = m_vSize;
	m_vPos = getActualPos();
	m_vSize = getActualSize();
	CBaseUIButton::update();
	m_vPos = posBackup;
	m_vSize = sizeBackup;

	if (!m_bVisible) return;

	// delayed image loading
	if (m_fImageLoadScheduledTime != 0.0f && engine->getTime() > m_fImageLoadScheduledTime)
	{
		m_fImageLoadScheduledTime = 0.0f;
		OsuBeatmapDifficulty *diff = isChild() ? m_child : (m_beatmap->getDifficulties().size() > 0 ? m_beatmap->getDifficulties()[0] : NULL);
		if (diff != NULL)
			diff->loadBackgroundImage();
	}

	// animations need constant layout updates anyway
	updateLayout();
}

void OsuUISongBrowserButton::updateLayout()
{
	Image *menuButtonBackground = m_beatmap->getOsu()->getSkin()->getMenuButtonBackground();
	Vector2 minimumSize = Vector2(699.0f, 103.0f)*(m_beatmap->getOsu()->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
	float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
	m_fScale = Osu::getImageScale(m_beatmap->getOsu(), menuButtonBackground->getSize()*minimumScale, 64.0f);

	if (m_bVisible) // lag prevention (animationHandler overflow)
	{
		const float centerOffsetAnimationTarget = clamp<float>(std::abs((m_vPos.y+(m_vSize.y/2)-m_view->getPos().y-m_view->getSize().y/2)/(m_view->getSize().y/2)), 0.0f, 1.0f);
		anim->moveQuadOut(&m_fCenterOffsetAnimation, centerOffsetAnimationTarget, 0.5f, true);

		float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y))/3500.0f, 0.0f, 1.0f);
		if (m_view->isScrolling())
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, 0.0f, 1.0f, true);
		else
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, centerOffsetVelocityAnimationTarget, 1.25f, true);
	}

	setSize((int)(menuButtonBackground->getWidth()*m_fScale), (int)(menuButtonBackground->getHeight()*m_fScale));
	setRelPosX(m_view->getSize().x - m_view->getSize().x*0.55f - (isChild() ? m_view->getSize().x*0.075f : 0) - m_view->getSize().x*0.075f*m_fHoverOffsetAnimation + 0.04f*m_view->getSize().x*m_fCenterOffsetAnimation + 0.3f*m_view->getSize().x*m_fCenterOffsetVelocityAnimation);

	if (!isChild())
	{
		for (int i=0; i<m_children.size(); i++)
		{
			m_children[i]->updateLayout();
		}
	}
}

void OsuUISongBrowserButton::select(bool clicked, bool selectTopChild)
{
	// child block
	if (isChild())
	{
		if (previousChild == this)
		{
			// start playing
			// notify songbrowser
			m_songBrowser->onDifficultySelected(m_beatmap, m_child, clicked, true);
			m_songBrowser->scrollToSongButton(this);
			return;
		}

		// deselect previous
		if (previousChild != NULL)
			previousChild->deselect(true);
		previousChild = this;

		// select it
		m_beatmap->selectDifficulty(m_child, false);
		m_bSelected = true;

		// notify songbrowser
		m_songBrowser->onDifficultySelected(m_beatmap, m_child, clicked);
		m_songBrowser->scrollToSongButton(this);
		return;
	}

	// parent block
	if (previousParent == this)
		return;

	// this must be first, because of shared resource deletion by name
	int scrollDiffY = m_vPos.y; // keep position consistent if children get removed above us
	if (previousParent != NULL)
	{
		previousParent->deselect();

		// compensate
		scrollDiffY = scrollDiffY - m_vPos.y;
		if (scrollDiffY > 0)
			m_view->scrollY(scrollDiffY, false);
	}
	if (!isChild())
		previousParent = this;

	// now we can select ourself
	m_beatmap->select();
	m_bSelected = true;

	// hide ourself, show children
	if (!isChild())
	{
		if (m_children.size() > 0)
		{
			for (int i=0; i<m_children.size(); i++)
			{
				m_view->getContainer()->insertBaseUIElement(m_children[i], this);
				m_children[i]->setRelPos(getRelPos());
				m_children[i]->setPos(getPos());
				m_children[i]->setVisible(true);
			}
			m_view->getContainer()->removeBaseUIElement(this);

			// update layout
			m_songBrowser->onResolutionChange(m_beatmap->getOsu()->getScreenSize());

			// select diff immediately, but after the layout update!
			if (selectTopChild)
				m_children[0]->select();
			else
				m_children[m_children.size()-1]->select(clicked);

			this->setVisible(false);
		}
	}
}

void OsuUISongBrowserButton::deselect(bool child)
{
	// child block
	if (child)
	{
		m_bSelected = false;
		return;
	}

	// parent block
	m_beatmap->deselect(false);
	m_bSelected = false;

	// show ourself, hide children
	if (m_children.size() > 0)
	{
		// note that this does work fine with searching, because it adds elements based on relative other elements
		// if the relative other elements can't be found, no insert or remove takes place
		m_view->getContainer()->insertBaseUIElement(this, m_children[0]);
		setRelPos(m_children[0]->getRelPos());
		setPos(m_children[0]->getPos());
		for (int i=0; i<m_children.size(); i++)
		{
			m_view->getContainer()->removeBaseUIElement(m_children[i]);
			m_children[i]->setVisible(false);
		}

		// update layout
		m_songBrowser->onResolutionChange(m_beatmap->getOsu()->getScreenSize());
	}
}

void OsuUISongBrowserButton::checkLoadUnloadImage()
{
	// dynamically load thumbnails
	OsuBeatmapDifficulty *diff = isChild() ? m_child : (m_beatmap->getDifficulties().size() > 0 ? m_beatmap->getDifficulties()[0] : NULL);
	if (m_bVisible)
	{
		// visibility delay, don't load immediately but only if we are visible for a minimum amount of time (to avoid useless loads)
		m_fImageLoadScheduledTime = engine->getTime() + 0.1f/* + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*0.1f*(isChild() ? 0.0f : 1.0f)*/;
	}
	else
	{
		m_fImageLoadScheduledTime = 0.0f;
		if (diff != NULL)
		{
			// only allow unloading if we are not selected and no children of us are selected
			bool areChildrenSelected = false;
			if (!isChild())
			{
				for (int i=0; i<m_children.size(); i++)
				{
					if (m_children[i]->isSelected())
					{
						areChildrenSelected = true;
						break;
					}
				}
			}
			else // if we are a child, only allow unloading if the parent is also invisible and we are not the first child
				areChildrenSelected = areChildrenSelected || m_parent->isSelected() || (m_parent->isVisible() && m_bIsFirstChild);

			if (!isSelected() && !areChildrenSelected)
				diff->unloadBackgroundImage();
		}
	}
}

void OsuUISongBrowserButton::onClicked()
{
	if (previousParent == this)
		return;

	engine->getSound()->play(m_beatmap->getOsu()->getSkin()->getMenuClick());
	CBaseUIButton::onClicked();
	select(true);
}

void OsuUISongBrowserButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();

	if (engine->getTime() > lastHoverSoundTime + 0.05f) // to avoid earraep
	{
		if (engine->hasFocus())
			engine->getSound()->play(m_beatmap->getOsu()->getSkin()->getMenuClick());
		lastHoverSoundTime = engine->getTime();
	}

	anim->moveQuadOut(&m_fHoverOffsetAnimation, 1.0f, 1.0f*(1.0f - m_fHoverOffsetAnimation), true);
}

void OsuUISongBrowserButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();

	anim->moveQuadOut(&m_fHoverOffsetAnimation, 0.0f, 1.0f*m_fHoverOffsetAnimation, true);
}

void OsuUISongBrowserButton::setVisible(bool visible)
{
	m_bVisible = visible;

	// this is called in all cases (outside viewing volume of scrollView, and if not visible because replaced by children)
	// and also if the selection changes (due to previousParent and previousChild)
	checkLoadUnloadImage();

	// animation feel
	m_fCenterOffsetAnimation = 1.0f;
	m_fHoverOffsetAnimation = 0.0f;
	const float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y))/3500.0f, 0.0f, 1.0f);
	m_fCenterOffsetVelocityAnimation = centerOffsetVelocityAnimationTarget;
	deleteAnimations();

	updateLayout();
}

Vector2 OsuUISongBrowserButton::getActualOffset()
{
	const float hd2xMultiplier = m_beatmap->getOsu()->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f;
	///const float correctedMarginPixelsX = (2*marginPixelsX + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getWidth()/hd2xMultiplier - 699)/2.0f;
	const float correctedMarginPixelsY = (2*marginPixelsY + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getHeight()/hd2xMultiplier - 103.0f)/2.0f;
	return Vector2((int)(marginPixelsX*m_fScale*hd2xMultiplier), (int)(correctedMarginPixelsY*m_fScale*hd2xMultiplier));
}
