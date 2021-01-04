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
#include "OsuSongBrowser2.h"

OsuUISongBrowserCollectionButton::OsuUISongBrowserCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children) : OsuUISongBrowserButton(osu, songBrowser, view, contextMenu, xPos, yPos, xSize, ySize, name)
{
	m_sCollectionName = collectionName;
	m_children = children;

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
	{
		g->scale(titleScale, titleScale);
		g->translate(pos.x + textXOffset, pos.y + size.y/2 + (m_font->getHeight()*titleScale)/2);
		g->drawString(m_font, titleString);
	}
	g->popTransform();
}

void OsuUISongBrowserCollectionButton::onSelected(bool wasSelected)
{
	OsuUISongBrowserButton::onSelected(wasSelected);

	m_songBrowser->onSelectionChange(this, true);
	m_songBrowser->scrollToSongButton(this, true);
}

UString OsuUISongBrowserCollectionButton::buildTitleString()
{
	UString titleString = m_sCollectionName;

	// count children (also with support for search results in collections)
	int numChildren = 0;
	{
		for (size_t c=0; c<m_children.size(); c++)
		{
			const std::vector<OsuUISongBrowserButton*> &childrenChildren = m_children[c]->getChildren();
			if (childrenChildren.size() > 0)
			{
				for (size_t cc=0; cc<childrenChildren.size(); cc++)
				{
					if (childrenChildren[cc]->isSearchMatch())
						numChildren++;
				}
			}
			else if (m_children[c]->isSearchMatch())
				numChildren++;
		}
	}

	titleString.append(UString::format((numChildren == 1 ? " (%i map)" : " (%i maps)"), numChildren));

	return titleString;
}
