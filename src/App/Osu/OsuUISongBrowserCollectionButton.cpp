//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		collection button (also used for grouping)
//
// $NoKeywords: $osusbcb
//===============================================================================//

#include "OsuUISongBrowserCollectionButton.h"

#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuSongBrowser2.h"

#include "OsuBeatmapDifficulty.h"

OsuUISongBrowserCollectionButton *OsuUISongBrowserCollectionButton::s_previousButton = NULL;

OsuUISongBrowserCollectionButton::OsuUISongBrowserCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children) : OsuUISongBrowserButton(osu, songBrowser, view, xPos, yPos, xSize, ySize, name)
{
	m_sCollectionName = collectionName;
	m_children = children;

	s_previousButton = NULL; // reset

	m_fTitleScale = 0.35f;

	// settings
	setOffsetPercent(0.075f*0.5f);
	setInactiveBackgroundColor(COLOR(255, 35, 50, 143));
	setActiveBackgroundColor(COLOR(255, 163, 240, 44));
}

void OsuUISongBrowserCollectionButton::draw(Graphics *g)
{
	OsuUISongBrowserButton::draw(g);
	if (!m_bVisible) return;

	OsuSkin *skin = m_osu->getSkin();

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	// draw title
	UString titleString = buildTitleString();
	int textXOffset = size.x*0.02f;
	float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
	g->pushTransform();
		g->scale(titleScale, titleScale);
		g->translate(pos.x + textXOffset, pos.y + size.y/2 + (m_font->getHeight()*titleScale)/2);
		g->drawString(m_font, titleString);
	g->popTransform();
}

void OsuUISongBrowserCollectionButton::setPreviousButton(OsuUISongBrowserCollectionButton *previousButton)
{
	s_previousButton = previousButton;
}

OsuUISongBrowserCollectionButton *OsuUISongBrowserCollectionButton::getPreviousButton()
{
	return s_previousButton;
}

std::vector<OsuUISongBrowserButton*> OsuUISongBrowserCollectionButton::getChildren()
{
	std::vector<OsuUISongBrowserButton*> children;
	if (m_bSelected)
	{
		for (int i=0; i<m_children.size(); i++)
		{
			// TODO:
			// parent
			// special case here, since the beatmap button is hidden while it is selected (gets replaced by the diff buttons)
			///if (!m_children[i]->isSelected())
				children.push_back(m_children[i]);

			// children
			std::vector<OsuUISongBrowserButton*> recursiveChildren = m_children[i]->getChildren();
			if (recursiveChildren.size() > 0)
			{
				children.reserve(children.size() + recursiveChildren.size());
				children.insert(children.end(), recursiveChildren.begin(), recursiveChildren.end());
			}
		}
	}
	return children;
}

void OsuUISongBrowserCollectionButton::onSelected(bool wasSelected)
{
	if (wasSelected)
	{
		deselect();
		return;
	}

	// automatically deselect previous selection
	if (getPreviousButton() != NULL && getPreviousButton() != this)
		getPreviousButton()->deselect();
	else
		m_songBrowser->rebuildSongButtons(); // this is in the else-branch to avoid double execution (since it is already executed in deselect())
	setPreviousButton(this);

	m_songBrowser->scrollToSongButton(this, true);
}

void OsuUISongBrowserCollectionButton::onDeselected()
{
	m_songBrowser->rebuildSongButtons();
}

UString OsuUISongBrowserCollectionButton::buildTitleString()
{
	UString titleString = m_sCollectionName;
	int numChildren = 0;
	for (int i=0; i<m_children.size(); i++)
	{
		if (m_children[i]->getBeatmap() != NULL)
			numChildren += m_children[i]->getBeatmap()->getDifficulties().size();
	}
	titleString.append(UString::format((numChildren == 1 ? " (%i map)" : " (%i maps)"), numChildren));

	// debugging
	/*
	titleString.append(" ");
	if (m_children.size() > 0)
		titleString.append(UString::format("%i, %i", m_children[0]->getBeatmap()->getDifficulties()[0]->setID, m_children[0]->getBeatmap()->getDifficulties()[0]->ID));
	*/

	return titleString;
}
