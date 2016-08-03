//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap browser and selector
//
// $NoKeywords: $osusb
//===============================================================================//

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

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuModSelector.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"
#include "OsuUISelectionButton.h"

ConVar osu_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f);
ConVar osu_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f);
ConVar osu_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f);
ConVar osu_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.5f);



class OsuSongBrowserTopBarSongInfo : public CBaseUIElement
{
public:
	OsuSongBrowserTopBarSongInfo(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
	{
		m_osu = osu;
		m_font = m_osu->getSubTitleFont();

		m_iMargin = 10;

		float globalScaler = 1.3f;
		m_fSubTitleScale = 0.6f*globalScaler;
		m_fSongInfoScale = 0.7f*globalScaler;
		m_fDiffInfoScale = 0.65f*globalScaler;

		m_sArtist = "Artist";
		m_sTitle = "Title";
		m_sDiff = "Difficulty";
		m_sMapper = "Mapper";

		m_iLengthMS = 0;
		m_iMinBPM = 0;
		m_iMaxBPM = 0;
		m_iNumObjects = 0;

		m_fCS = 5.0f;
		m_fAR = 5.0f;
		m_fOD = 5.0f;
		m_fHP = 5.0f;
		m_fStars = 5.0f;
	}

	virtual void draw(Graphics *g)
	{
		// debug bounding box
		/*
		g->setColor(0xffffffff);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
		*/

		// build strings
		UString titleText = buildTitleString();
		UString subTitleText = buildSubTitleString();
		UString songInfoText = buildSongInfoString();
		UString diffInfoText = buildDiffInfoString();

		// draw
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate(m_vPos.x, m_vPos.y + m_font->getHeight());
		g->drawString(m_font, titleText);
		g->popTransform();

		float subTitleStringWidth = m_font->getStringWidth(subTitleText);
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate((int)(-subTitleStringWidth/2), (int)(m_font->getHeight()/2));
		g->scale(m_fSubTitleScale, m_fSubTitleScale);
		g->translate((int)(m_vPos.x + (subTitleStringWidth/2)*m_fSubTitleScale), (int)(m_vPos.y + m_font->getHeight() + (m_font->getHeight()/2)*m_fSubTitleScale + m_iMargin));
		g->drawString(m_font, subTitleText);
		g->popTransform();

		float songInfoStringWidth = m_font->getStringWidth(songInfoText);
		g->setColor(0xffffffff);
		if (m_osu->getSpeedMultiplier() != 1.0f)
			g->setColor(0xffff0000);
		g->pushTransform();
		g->translate((int)(-songInfoStringWidth/2), (int)(m_font->getHeight()/2));
		g->scale(m_fSongInfoScale, m_fSongInfoScale);
		g->translate((int)(m_vPos.x + (songInfoStringWidth/2)*m_fSongInfoScale), (int)(m_vPos.y + m_font->getHeight() + m_font->getHeight()*m_fSubTitleScale + (m_font->getHeight()/2)*m_fSongInfoScale + m_iMargin*2));
		g->drawString(m_font, songInfoText);
		g->popTransform();

		float diffInfoStringWidth = m_font->getStringWidth(diffInfoText);
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate((int)(-diffInfoStringWidth/2), (int)(m_font->getHeight()/2));
		g->scale(m_fDiffInfoScale, m_fDiffInfoScale);
		g->translate((int)(m_vPos.x + (diffInfoStringWidth/2)*m_fDiffInfoScale), (int)(m_vPos.y + m_font->getHeight() + m_font->getHeight()*m_fSubTitleScale + m_font->getHeight()*m_fSongInfoScale + (m_font->getHeight()/2)*m_fDiffInfoScale + m_iMargin*3));
		g->drawString(m_font, diffInfoText);
		g->popTransform();
	}

	void setArtist(UString artist) {m_sArtist = artist;}
	void setTitle(UString title) {m_sTitle = title;}
	void setDiff(UString diff) {m_sDiff = diff;}
	void setMapper(UString mapper) {m_sMapper = mapper;}

	void setLengthMS(unsigned long lengthMS) {m_iLengthMS = lengthMS;}
	void setBPM(int minBPM, int maxBPM) {m_iMinBPM = minBPM;m_iMaxBPM = maxBPM;}
	void setNumObjects(int numObjects) {m_iNumObjects = numObjects;}

	void setCS(float CS) {m_fCS = CS;}
	void setAR(float AR) {m_fAR = AR;}
	void setOD(float OD) {m_fOD = OD;}
	void setHP(float HP) {m_fHP = HP;}
	void setStars(float stars) {m_fStars = stars;}

	float getMinimumWidth()
	{
		/*
		float titleWidth = m_font->getStringWidth(buildTitleString());
		float subTitleWidth = m_font->getStringWidth(buildSubTitleString()) * m_fSubTitleScale;
		*/
		float titleWidth = 0;
		float subTitleWidth = 0;
		float songInfoWidth = m_font->getStringWidth(buildSongInfoString()) * m_fSongInfoScale;
		float diffInfoWidth = m_font->getStringWidth(buildDiffInfoString()) * m_fDiffInfoScale;

		return std::max(std::max(std::max(titleWidth, subTitleWidth), songInfoWidth), diffInfoWidth);
	}

	float getMinimumHeight()
	{
		float titleHeight = m_font->getHeight();
		float subTitleHeight = m_font->getHeight() * m_fSubTitleScale;
		float songInfoHeight = m_font->getHeight() * m_fSongInfoScale;
		float diffInfoHeight = m_font->getHeight() * m_fDiffInfoScale;

		return titleHeight + subTitleHeight + songInfoHeight + diffInfoHeight + m_iMargin*3;
	}

private:
	UString buildTitleString()
	{
		UString titleString = m_sArtist;
		titleString.append(" - ");
		titleString.append(m_sTitle);
		titleString.append(" [");
		titleString.append(m_sDiff);
		titleString.append("]");

		return titleString;
	}

	UString buildSubTitleString()
	{
		UString subTitleString = "Mapped by ";
		subTitleString.append(m_sMapper);

		return subTitleString;
	}

	UString buildSongInfoString()
	{
		const unsigned long fullSeconds = (m_iLengthMS*(1.0 / m_osu->getSpeedMultiplier())) / 1000;
		const int minutes = fullSeconds / 60;
		const int seconds = fullSeconds % 60;

		const int minBPM = m_iMinBPM * m_osu->getSpeedMultiplier();
		const int maxBPM = m_iMaxBPM * m_osu->getSpeedMultiplier();

		if (m_iMinBPM == m_iMaxBPM)
			return UString::format("Length: %02i:%02i BPM: %i Objects: %i", minutes, seconds, maxBPM, m_iNumObjects);
		else
			return UString::format("Length: %02i:%02i BPM: %i-%i Objects: %i", minutes, seconds, minBPM, maxBPM, m_iNumObjects);
	}

	UString buildDiffInfoString()
	{
		return UString::format("CS:%.2g AR:%.2g OD:%.2g HP:%.2g Stars:%.2g", m_fCS, m_fAR, m_fOD, m_fHP, m_fStars);
	}

	Osu *m_osu;
	McFont *m_font;

	int m_iMargin;
	float m_fSubTitleScale;
	float m_fSongInfoScale;
	float m_fDiffInfoScale;

	UString m_sArtist;
	UString m_sTitle;
	UString m_sDiff;
	UString m_sMapper;

	unsigned long m_iLengthMS;
	int m_iMinBPM;
	int m_iMaxBPM;
	int m_iNumObjects;

	float m_fCS;
	float m_fAR;
	float m_fOD;
	float m_fHP;
	float m_fStars;
};



// TODO: put this mess in separate files, and properly generalize the class
class OsuSongBrowserSongButton : public CBaseUIButton
{
public:
	OsuSongBrowserSongButton(OsuBeatmap *beatmap, CBaseUIScrollView *view, OsuSongBrowser2 *songBrowser, float xPos, float yPos, float xSize, float ySize, UString name, OsuSongBrowserSongButton *parent = NULL, OsuBeatmapDifficulty *child = NULL, bool isFirstChild = false) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
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
			    	unsigned long diff1 = (a->AR+1)*(a->CS+1)*(a->HP+1)*(a->OD+1)*a->maxBPM;
			    	unsigned long diff2 = (b->AR+1)*(b->CS+1)*(b->HP+1)*(b->OD+1)*b->maxBPM;

			        return diff1 < diff2;
			    }
			};
			std::sort(difficulties.begin(), difficulties.end(), SortComparator());

			// and add them
			for (int i=0; i<difficulties.size(); i++)
			{
				OsuSongBrowserSongButton *songButton = new OsuSongBrowserSongButton(beatmap, m_view, m_songBrowser, 0, 0, 0, 0, "", this, difficulties[i], i == 0);
				m_children.push_back(songButton);
			}
		}

		updateLayout();
	}

	~OsuSongBrowserSongButton()
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

	virtual void draw(Graphics *g)
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

	virtual void update()
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

	void updateLayout()
	{
		Image *menuButtonBackground = m_beatmap->getOsu()->getSkin()->getMenuButtonBackground();
		Vector2 minimumSize = Vector2(699.0f, 103.0f)*(m_beatmap->getOsu()->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
		float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
		m_fScale = Osu::getImageScale(menuButtonBackground->getSize()*minimumScale, 64.0f);

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

	void select(bool clicked = false, bool selectTopChild = false)
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
				m_songBrowser->onResolutionChange(Osu::getScreenSize());

				// select diff immediately, but after the layout update!
				if (selectTopChild)
					m_children[0]->select();
				else
					m_children[m_children.size()-1]->select(clicked);

				this->setVisible(false);
			}
		}
	}

	void deselect(bool child = false)
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
			m_songBrowser->onResolutionChange(Osu::getScreenSize());
		}
	}

	void setVisible(bool visible)
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

	void setTitle(UString title) {m_sTitle = title;}
	void setArtist(UString artist) {m_sArtist = artist;}
	void setMapper(UString mapper) {m_sMapper = mapper;}
	void setDiff(UString diff) {m_sDiff = diff;}

	void setSelected(bool selected) {m_bSelected = selected;}

	inline OsuBeatmap *getBeatmap() const {return m_beatmap;}
	Vector2 getActualOffset()
	{
		const float hdMultiplier = m_beatmap->getOsu()->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f;
		///const float correctedMarginPixelsX = (2*marginPixelsX + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getWidth()/hdMultiplier - 699)/2.0f;
		const float correctedMarginPixelsY = (2*marginPixelsY + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getHeight()/hdMultiplier - 103.0f)/2.0f;
		return Vector2((int)(marginPixelsX*m_fScale*hdMultiplier), (int)(correctedMarginPixelsY*m_fScale*hdMultiplier));
	}
	inline Vector2 getActualSize() {return m_vSize - 2*getActualOffset();}
	inline Vector2 getActualPos() {return m_vPos + getActualOffset();}
	inline std::vector<OsuSongBrowserSongButton*> getChildren() {return m_children;}

	inline bool isChild() {return m_child != NULL;}
	inline bool isSelected() const {return m_bSelected;}

private:
	static OsuSongBrowserSongButton *previousParent;
	static OsuSongBrowserSongButton *previousChild;
	static int marginPixelsX;
	static int marginPixelsY;
	static float thumbnailYRatio;
	static float lastHoverSoundTime;
	static Color inactiveBackgroundColor;
	static Color activeBackgroundColor;
	static Color inactiveDifficultyBackgroundColor;

	virtual void onClicked()
	{
		if (previousParent == this)
			return;

		engine->getSound()->play(m_beatmap->getOsu()->getSkin()->getMenuClick());
		CBaseUIButton::onClicked();
		select(true);
	}

	virtual void onMouseInside()
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

	virtual void onMouseOutside()
	{
		CBaseUIButton::onMouseOutside();

		anim->moveQuadOut(&m_fHoverOffsetAnimation, 0.0f, 1.0f*m_fHoverOffsetAnimation, true);
	}

	void checkLoadUnloadImage()
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

	void deleteAnimations()
	{
		anim->deleteExistingAnimation(&m_fCenterOffsetAnimation);
		anim->deleteExistingAnimation(&m_fCenterOffsetVelocityAnimation);
		anim->deleteExistingAnimation(&m_fHoverOffsetAnimation);
	}

	UString buildTitleString()
	{
		return m_sTitle;
	}

	UString buildSubTitleString()
	{
		UString subTitleString = m_sArtist;
		subTitleString.append(" // ");
		subTitleString.append(m_sMapper);

		return subTitleString;
	}

	UString buildDiffString()
	{
		return m_sDiff;
	}

	OsuBeatmap *m_beatmap;
	CBaseUIScrollView *m_view;
	OsuSongBrowser2 *m_songBrowser;
	OsuSongBrowserSongButton *m_parent;
	OsuBeatmapDifficulty *m_child;
	McFont *m_font;
	McFont *m_fontBold;

	UString m_sTitle;
	UString m_sArtist;
	UString m_sMapper;
	UString m_sDiff;

	bool m_bSelected;
	bool m_bIsFirstChild;
	float m_fScale;

	float m_fTextSpacingScale;
	float m_fTextMarginScale;
	float m_fTitleScale;
	float m_fSubTitleScale;
	float m_fDiffScale;

	float m_fImageLoadScheduledTime;
	float m_fHoverOffsetAnimation;
	float m_fCenterOffsetAnimation;
	float m_fCenterOffsetVelocityAnimation;

	std::vector<OsuSongBrowserSongButton*> m_children;
};
OsuSongBrowserSongButton *OsuSongBrowserSongButton::previousParent = NULL;
OsuSongBrowserSongButton *OsuSongBrowserSongButton::previousChild = NULL;
int OsuSongBrowserSongButton::marginPixelsX = 9;
int OsuSongBrowserSongButton::marginPixelsY = 9;
float OsuSongBrowserSongButton::thumbnailYRatio = 1.333333f;
float OsuSongBrowserSongButton::lastHoverSoundTime = 0;
Color OsuSongBrowserSongButton::inactiveBackgroundColor = COLOR(255, 235, 73, 153); // pink
Color OsuSongBrowserSongButton::activeBackgroundColor = COLOR(255, 255, 255, 255); // white
Color OsuSongBrowserSongButton::inactiveDifficultyBackgroundColor = COLOR(255, 0, 150, 236); // blue




OsuSongBrowser2::OsuSongBrowser2(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// convar refs
	m_fps_max_ref = convar->getConVarByName("fps_max");
	m_osu_songbrowser_bottombar_percent_ref = convar->getConVarByName("osu_songbrowser_bottombar_percent");

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

	m_songInfo = new OsuSongBrowserTopBarSongInfo(m_osu, 0, 0, 0, 0, "");
	m_topbarLeft->addBaseUIElement(m_songInfo);

	// build topbar right
	m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");

	// build bottombar
	m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

	addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser2::onSelectionMods) );
	addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser2::onSelectionRandom) );
	///addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser::onSelectionOptions) );

	// build songbrowser
	m_songBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_songBrowser->setDrawBackground(false);
	m_songBrowser->setDrawFrame(false);
	m_songBrowser->setHorizontalScrolling(false);
	m_songBrowser->setScrollResistance(15);

	// beatmap database
	m_importTimer = new Timer();
	m_bBeatmapRefreshNeedsFileRefresh = true;
	m_iCurRefreshBeatmap = 0;
	m_iCurRefreshNumBeatmaps = 0;
	m_bBeatmapRefreshScheduled = true;

	// behaviour
	m_bHasSelectedAndIsPlaying = false;
	m_selectedBeatmap = NULL;
	m_fPulseAnimation = 0.0f;

	// search
	m_fSearchWaitTime = 0.0f;

	updateLayout();
}

OsuSongBrowser2::~OsuSongBrowser2()
{
	SAFE_DELETE(m_topbarLeft);
	SAFE_DELETE(m_topbarRight);
	SAFE_DELETE(m_bottombar);
	SAFE_DELETE(m_songBrowser);
}

void OsuSongBrowser2::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	// refreshing (blocks every other call in draw() below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffffffff);
		UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(((float)m_iCurRefreshBeatmap/(float)m_iCurRefreshNumBeatmaps)*100.0f));
		g->pushTransform();
		g->translate(Osu::getScreenWidth()/2 - m_osu->getTitleFont()->getStringWidth(loadingMessage)/2, Osu::getScreenHeight() - 15);
		g->drawString(m_osu->getTitleFont(), loadingMessage);
		g->popTransform();

		m_osu->getHUD()->drawBeatmapImportSpinner(g);
		return;
	}


	// draw background
	g->setColor(0xff000000);
	g->fillRect(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight());

	// draw background image
	if (m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty() != NULL)
	{
		Image *backgroundImage = m_selectedBeatmap->getSelectedDifficulty()->backgroundImage;
		if (backgroundImage != NULL)
		{
			float scale = Osu::getImageScaleToFillResolution(backgroundImage, Osu::getScreenSize());

			g->setColor(0xff999999);
			g->pushTransform();
			g->scale(scale, scale);
			g->translate(Osu::getScreenWidth()/2, Osu::getScreenHeight()/2);
			g->drawImage(backgroundImage);
			g->popTransform();
		}
	}

	// draw song browser
	m_songBrowser->draw(g);

	// draw search text and background
	UString searchText1 = "Search: ";
	UString searchText2 = "Type to search!";
	UString combinedSearchText = searchText1;
	combinedSearchText.append(searchText2);
	McFont *searchTextFont = m_osu->getSubTitleFont();
	float searchTextScale = 0.75f;
	bool hasSearchSubTextVisible = m_sSearchString.length() > 0 && (m_fSearchWaitTime == 0.0f || m_visibleSongButtons.size() != m_songButtons.size());
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
	//m_topbarLeft->draw_debug(g);
	m_topbarRight->draw(g);
	//m_topbarRight->draw_debug(g);

	// draw bottom bar
	float songSelectBottomScale = m_bottombar->getSize().y / m_osu->getSkin()->getSongSelectBottom()->getHeight();
	songSelectBottomScale *= 0.8f;

	g->setColor(0xff000000);
	g->fillRect(0, m_bottombar->getPos().y + 10, Osu::getScreenWidth(), m_bottombar->getSize().y);

	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(songSelectBottomScale, songSelectBottomScale);
		g->translate(0, (int)(m_bottombar->getPos().y) + (int)((m_osu->getSkin()->getSongSelectBottom()->getHeight()*songSelectBottomScale)/2) - 1);
		m_osu->getSkin()->getSongSelectBottom()->bind();
		g->drawQuad(0, -(int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)/2), (int)(Osu::getScreenWidth()*(1.0f/songSelectBottomScale)), (int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)));
	g->popTransform();

	m_bottombar->draw(g);
	//m_bottombar->draw_debug(g);
	m_backButton->draw(g);


	// no beatmaps found (osu folder is probably invalid)
	if (m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffff0000);
		UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
		errorMessage1.append(m_sLastOsuFolder);
		UString errorMessage2 = "Go to Options -> osu!folder";
		g->pushTransform();
		g->translate((int)(Osu::getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage1)/2), (int)(Osu::getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()));
		g->drawString(m_osu->getSubTitleFont(), errorMessage1);
		g->popTransform();

		g->setColor(0xff00ff00);
		g->pushTransform();
		g->translate((int)(Osu::getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage2)/2), (int)(Osu::getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()*2 + 15));
		g->drawString(m_osu->getSubTitleFont(), errorMessage2);
		g->popTransform();
	}

	// click pulse animation overlay
	if (m_fPulseAnimation > 0.0f)
	{
		Color topColor = 0x00ffffff;
		Color bottomColor = COLOR((int)(25*m_fPulseAnimation), 255, 255, 255);

		g->fillGradient(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), topColor, topColor, bottomColor, bottomColor);
	}

	// debug previous random beatmap
	/*
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(Osu::getScreenWidth()/5, Osu::getScreenHeight()/3);
	for (int i=0; i<m_previousRandomBeatmaps.size(); i++)
	{
		UString message = UString::format("#%i = ", i);
		message.append(m_previousRandomBeatmaps[i]->getTitle());

		g->drawString(m_osu->getSongBrowserFont(), message);
		g->translate(0, m_osu->getSongBrowserFont()->getHeight()+10);
	}
	g->popTransform();
	*/
}

void OsuSongBrowser2::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// refresh logic (blocks every other call in the update() function below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		Timer t;
		t.start();

		if (m_bBeatmapRefreshNeedsFileRefresh)
		{
			m_bBeatmapRefreshNeedsFileRefresh = false;
			refreshBeatmaps();
		}

		while (m_bBeatmapRefreshScheduled && t.getElapsedTime() < 0.033f)
		{
			if (m_refreshBeatmaps.size() > 0)
			{
				UString fullBeatmapPath = m_sCurRefreshOsuSongFolder;
				fullBeatmapPath.append(m_refreshBeatmaps[m_iCurRefreshBeatmap++]);
				fullBeatmapPath.append("/");

				OsuBeatmap *bm = new OsuBeatmap(m_osu, fullBeatmapPath);

				// if the beatmap is valid, add it
				if (bm->load())
				{
					OsuSongBrowserSongButton *songButton = new OsuSongBrowserSongButton(bm, m_songBrowser, this, 250, 250 + m_beatmaps.size()*50, 200, 50, "");

					m_songBrowser->getContainer()->addBaseUIElement(songButton);
					m_songButtons.push_back(songButton);
					m_visibleSongButtons.push_back(songButton);
					m_beatmaps.push_back(bm);
				}
				else
				{
					if (Osu::debug->getBool())
						debugLog("Couldn't load(), deleting object.\n");
					SAFE_DELETE(bm);
				}
			}

			if (m_iCurRefreshBeatmap >= m_iCurRefreshNumBeatmaps)
			{
				m_bBeatmapRefreshScheduled = false;
				m_importTimer->update();
				debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_beatmaps.size(), m_importTimer->getElapsedTime());
				updateLayout();
			}

			t.update();
		}
		return;
	}

	m_songBrowser->update();
	m_songBrowser->getContainer()->update_pos(); // necessary due to constant animations
	m_topbarLeft->update();
	m_topbarRight->update();
	m_bottombar->update();

	// handle right click absolute drag scrolling
	if (m_songBrowser->isMouseInside() && engine->getMouse()->isRightDown())
		m_songBrowser->scrollToY(-((engine->getMouse()->getPos().y - m_songBrowser->getPos().y)/m_songBrowser->getSize().y)*m_songBrowser->getScrollSize().y);

	// handle random beatmap selection
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

	// if mouse is to the left, force center currently selected beatmap/diff
	if (engine->getMouse()->getPos().x < Osu::getScreenWidth()*0.08f)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				scrollToSongButton(songButton);
				break;
			}
		}
	}

	// handle searching
	if (m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime)
	{
		m_fSearchWaitTime = 0.0f;

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
				{
					m_visibleSongButtons.push_back(m_songButtons[i]);

					// add children again if the song button is the currently selected one
					if (m_songButtons[i]->getBeatmap() == m_selectedBeatmap)
					{
						std::vector<OsuSongBrowserSongButton*> children = m_songButtons[i]->getChildren();
						for (int c=0; c<children.size(); c++)
						{
							m_songBrowser->getContainer()->addBaseUIElement(children[c]);
						}
					}
					else // add the parent
						m_songBrowser->getContainer()->addBaseUIElement(m_songButtons[i]);
				}
			}

			updateLayout();

			// scroll to top result
			if (m_visibleSongButtons.size() > 0)
				scrollToSongButton(m_visibleSongButtons[0]);
		}
		else
		{
			OsuSongBrowserSongButton *selectedSongButton = NULL;

			for (int i=0; i<m_songButtons.size(); i++)
			{
				m_songButtons[i]->setVisible(false); // unload images

				// add children again if the song button is the currently selected one
				if (m_songButtons[i]->getBeatmap() == m_selectedBeatmap)
				{
					std::vector<OsuSongBrowserSongButton*> children = m_songButtons[i]->getChildren();
					for (int c=0; c<children.size(); c++)
					{
						m_songBrowser->getContainer()->addBaseUIElement(children[c]);

						if (children[c]->isSelected())
							selectedSongButton = children[c];
					}
				}
				else // add the parent
					m_songBrowser->getContainer()->addBaseUIElement(m_songButtons[i]);
			}

			m_visibleSongButtons = m_songButtons;
			updateLayout();

			// scroll back to selection
			if (selectedSongButton != NULL)
				scrollToSongButton(selectedSongButton);
		}
	}
}

void OsuSongBrowser2::onKeyDown(KeyboardEvent &key)
{
	if (!m_bVisible) return;

	if (m_bVisible && m_bBeatmapRefreshScheduled && (key == KEY_ESCAPE || key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt()))
	{
		m_bBeatmapRefreshScheduled = false;
		updateLayout();
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
		for (int i=0; i<elements->size(); i++)
		{
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				int nextSelectionIndex = i+1;
				if (nextSelectionIndex > elements->size()-1)
					nextSelectionIndex = 0;

				if (nextSelectionIndex != i)
				{
					OsuSongBrowserSongButton *nextSongButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[nextSelectionIndex]);
					if (!nextSongButton->isChild())
						nextSongButton->select(true, true);
					else
						nextSongButton->select(true);
				}
				break;
			}
		}
	}

	if (!engine->getKeyboard()->isAltDown() && key == KEY_UP)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				int nextSelectionIndex = i-1;
				if (nextSelectionIndex < 0)
					nextSelectionIndex = elements->size()-1;

				if (nextSelectionIndex != i)
				{
					OsuSongBrowserSongButton *nextSongButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[nextSelectionIndex]);
					nextSongButton->select(true);
				}
				break;
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
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);

			if (foundSelected && !songButton->isChild())
			{
				songButton->select(true);
				break;
			}

			if (songButton != NULL && songButton->isSelected())
				foundSelected = true;
		}
	}

	if (key == KEY_RIGHT && !m_bRight)
	{
		m_bRight = true;
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		bool foundSelected = false;
		for (int i=0; i<elements->size(); i++)
		{
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);

			if (foundSelected && !songButton->isChild())
			{
				songButton->select(true);
				break;
			}

			if (songButton != NULL && songButton->isSelected())
				foundSelected = true;
		}
	}

	// selection select
	if (key == KEY_ENTER)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				songButton->select(true);
				break;
			}
		}
	}

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

void OsuSongBrowser2::setVisible(bool visible)
{
	m_bVisible = visible;

	// this seems to get stuck sometimes otherwise
	m_bShiftPressed = false;

	if (m_bVisible)
	{
		updateLayout();

		// we have to re-select the current beatmap to start playing music again
		if (m_bHasSelectedAndIsPlaying && m_selectedBeatmap != NULL)
			m_selectedBeatmap->select();

		m_bHasSelectedAndIsPlaying = false;

		// try another refresh, maybe the osu!folder has changed
		if (m_beatmaps.size() == 0)
			refreshBeatmaps();
	}
}

void OsuSongBrowser2::updateLayout()
{
	m_bottombarNavButtons[0]->setImageResourceName(m_osu->getSkin()->getSelectionMods()->getName());
	m_bottombarNavButtons[0]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModsOver()->getName());
	m_bottombarNavButtons[1]->setImageResourceName(m_osu->getSkin()->getSelectionRandom()->getName());
	m_bottombarNavButtons[1]->setImageResourceNameOver(m_osu->getSkin()->getSelectionRandomOver()->getName());
	///m_bottombarNavButtons[2]->setImageResourceName(m_osu->getSkin()->getSelectionOptions()->getName());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	const int margin = 5;

	// top bar
	m_fSongSelectTopScale = Osu::getImageScaleToFitResolution(m_osu->getSkin()->getSongSelectTop(), Osu::getScreenSize());
	float songSelectTopHeightScaled = std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale, m_songInfo->getMinimumHeight()*1.5f + margin); // NOTE: the height is a heuristic here more or less
	m_fSongSelectTopScale = std::max(m_fSongSelectTopScale, songSelectTopHeightScaled / m_osu->getSkin()->getSongSelectTop()->getHeight());

	// topbar left
	m_topbarLeft->setSize(std::max(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_width_percent.getFloat() + margin, m_songInfo->getMinimumWidth() + margin), std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_percent.getFloat(), m_songInfo->getMinimumHeight() + margin));
	m_songInfo->setRelPos(margin, margin);
	m_songInfo->setSize(m_topbarLeft->getSize().x - margin, std::max(m_topbarLeft->getSize().y*0.65f - margin, m_songInfo->getMinimumHeight()));

	// topbar right
	m_topbarRight->setPosX(m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + margin);
	m_topbarRight->setSize(Osu::getScreenWidth() - m_topbarRight->getPos().x, m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_percent.getFloat());

	// bottombar
	int bottomBarHeight = Osu::getScreenHeight()*m_osu_songbrowser_bottombar_percent_ref->getFloat();

	m_bottombar->setPosY(Osu::getScreenHeight() - bottomBarHeight);
	m_bottombar->setSize(Osu::getScreenWidth(), bottomBarHeight);

	// nav bar
	float navBarStart = std::max(Osu::getScreenWidth()*0.175f, m_backButton->getSize().x);
	if (Osu::getScreenWidth()*0.175f < m_backButton->getSize().x + 25)
		navBarStart += m_backButton->getSize().x - Osu::getScreenWidth()*0.175f;

	// bottombar cont
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(Osu::getScreenWidth(), bottomBarHeight);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setRelPosX(navBarStart + i*m_bottombarNavButtons[i]->getSize().x);
	}
	m_bottombar->update_pos();

	// song browser
	m_songBrowser->setPos(m_topbarRight->getPos().x, m_topbarRight->getPos().y + m_topbarRight->getSize().y + 2);
	m_songBrowser->setSize(Osu::getScreenWidth() - m_topbarRight->getPos().x + 1, Osu::getScreenHeight() - m_songBrowser->getPos().y - m_bottombar->getSize().y+2);

	// this rebuilds the entire songButton layout (songButtons in relation to others)
	std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
	int yCounter = m_songBrowser->getSize().y/2;
	bool isChild = false;
	for (int i=0; i<elements->size(); i++)
	{
		OsuSongBrowserSongButton *songButton = dynamic_cast<OsuSongBrowserSongButton*>((*elements)[i]);

		if (songButton != NULL)
		{
			if (songButton->isChild() || isChild)
				yCounter += songButton->getSize().y*0.1f;
			isChild = songButton->isChild();

			songButton->setRelPosY(yCounter);
			songButton->updateLayout();

			yCounter += songButton->getActualSize().y;
		}
	}
	m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y/2);
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

void OsuSongBrowser2::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
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

void OsuSongBrowser2::onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool fromClick, bool play)
{
	// remember it
	// if this is the last one, and we selected with a click, restart the memory
	if (fromClick && m_previousRandomBeatmaps.size() < 2)
		m_previousRandomBeatmaps.clear();
	if (beatmap != m_selectedBeatmap)
		m_previousRandomBeatmaps.push_back(beatmap);

	m_selectedBeatmap = beatmap;

	// update song info
	m_songInfo->setArtist(diff->artist);
	m_songInfo->setTitle(diff->title);
	m_songInfo->setDiff(diff->name);
	m_songInfo->setMapper(diff->creator);

	m_songInfo->setLengthMS(beatmap->getLength()); // TODO:
	m_songInfo->setBPM(diff->minBPM, diff->maxBPM);
	m_songInfo->setNumObjects(0); // TODO:

	m_songInfo->setCS(diff->CS);
	m_songInfo->setAR(diff->AR);
	m_songInfo->setOD(diff->OD);
	m_songInfo->setHP(diff->HP);
	m_songInfo->setStars(0); // TODO:

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

	// notify mod selector (for BPM override slider)
	m_osu->getModSelector()->checkUpdateBPMSliderSlaves();
}

void OsuSongBrowser2::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying)
		return;

	m_bBeatmapRefreshNeedsFileRefresh = false;

	m_selectedBeatmap = NULL;

	// delete database
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
	m_songBrowser->clear();
	m_songButtons.clear();
	m_visibleSongButtons.clear();
	m_beatmaps.clear();
	m_refreshBeatmaps.clear();
	m_previousRandomBeatmaps.clear();

	// load all subfolders
	UString osuSongFolder = convar->getConVarByName("osu_folder")->getString();
	m_sLastOsuFolder = osuSongFolder;
	osuSongFolder.append("Songs/");
	m_sCurRefreshOsuSongFolder = osuSongFolder;
	//osuSongFolder.append("*.*");

	m_refreshBeatmaps = env->getFoldersInFolder(osuSongFolder);
	debugLog("Building beatmap database ...\n");
	debugLog("Found %i folders in osu! song folder.\n", m_refreshBeatmaps.size());
	m_iCurRefreshNumBeatmaps = m_refreshBeatmaps.size();
	m_iCurRefreshBeatmap = 0;

	// if there are more than 0 beatmaps, schedule a refresh
	if (m_refreshBeatmaps.size() > 0)
	{
		m_bBeatmapRefreshScheduled = true;
		m_importTimer->start();
	}
}

void OsuSongBrowser2::selectSongButton(OsuSongBrowserSongButton *songButton)
{
	songButton->select();
}

void OsuSongBrowser2::selectRandomBeatmap()
{
	if (m_visibleSongButtons.size() < 1)
		return;

	// remember previous
	if (m_previousRandomBeatmaps.size() == 0 && m_selectedBeatmap != NULL)
		m_previousRandomBeatmaps.push_back(m_selectedBeatmap);

	int randomIndex = rand() % m_visibleSongButtons.size();
	OsuSongBrowserSongButton *songButton = m_visibleSongButtons[randomIndex];
	selectSongButton(songButton);
}

void OsuSongBrowser2::selectPreviousRandomBeatmap()
{
	if (m_previousRandomBeatmaps.size() > 0)
	{
		if (m_previousRandomBeatmaps.size() > 1 && m_previousRandomBeatmaps[m_previousRandomBeatmaps.size()-1] == m_selectedBeatmap)
			m_previousRandomBeatmaps.pop_back(); // deletes the current beatmap which may also be at the top (so we don't switch to ourself)

		// select it, if we can find it (and remove it from memory)
		OsuBeatmap *previousRandomBeatmap = m_previousRandomBeatmaps.back();
		for (int i=0; i<m_visibleSongButtons.size(); i++)
		{
			if (m_visibleSongButtons[i]->getBeatmap() == previousRandomBeatmap)
			{
				m_previousRandomBeatmaps.pop_back();
				selectSongButton(m_visibleSongButtons[i]);
				break;
			}
		}
	}
}

void OsuSongBrowser2::scrollToSongButton(OsuSongBrowserSongButton *songButton)
{
	m_songBrowser->scrollToY(-songButton->getRelPos().y + m_songBrowser->getSize().y/2 - songButton->getSize().y/2);
}

bool OsuSongBrowser2::searchMatcher(OsuBeatmap *beatmap, UString searchString)
{
	std::vector<OsuBeatmapDifficulty*> diffs = beatmap->getDifficulties();

	// intelligent search parser
	// all strings which are not expressions get appended with spaces between, then checked with one call to findSubstringInDifficulty()
	// the rest is interpreted
	// WARNING: this code is quite shitty. the order of the operators array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
	// TODO: write proper parser (or beatmap database)
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
		BPM
	};
	const std::vector<std::pair<UString, keywordId>> keywords =
	{
		std::pair<UString, keywordId>("ar", AR),
		std::pair<UString, keywordId>("cs", CS),
		std::pair<UString, keywordId>("od", OD),
		std::pair<UString, keywordId>("hp", HP),
		std::pair<UString, keywordId>("bpm",BPM)
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
				// TODO: this check is only necessary due to multiple redundant parser executions (one per diff!), make more elegant
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

		if (expressionsMatch) // as soon as one difficulty matches all expressions, we are done here (TODO: gui stuff to only show matched diffs)
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
