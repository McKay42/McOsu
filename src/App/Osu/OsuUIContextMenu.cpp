//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		context menu, dropdown style
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUIContextMenu.h"

#include "AnimationHandler.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUIButton.h"

#include "Osu.h"

OsuUIContextMenu::OsuUIContextMenu(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, CBaseUIScrollView *parent) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_parent = parent;
	m_container = new CBaseUIContainer(xPos, yPos, xSize, ySize, name);
	m_iYCounter = 0;
	m_iWidthCounter = 0;

	m_bVisible = false;
	m_bVisible2 = false;
	m_clickCallback = NULL;

	m_fAnimation = 0.0f;
	m_bInvertAnimation = false;
}

OsuUIContextMenu::~OsuUIContextMenu()
{
	SAFE_DELETE(m_container);
}

void OsuUIContextMenu::draw(Graphics *g)
{
	if (!m_bVisible || !m_bVisible2) return;

	if (m_fAnimation > 0.0f && m_fAnimation < 1.0f)
	{
		g->push3DScene(McRect(m_vPos.x, m_vPos.y + ((m_vSize.y/2.0f) * (m_bInvertAnimation ? 1.0f : -1.0f)), m_vSize.x, m_vSize.y));
		g->rotate3DScene((1.0f - m_fAnimation)*90.0f * (m_bInvertAnimation ? 1.0f : -1.0f), 0, 0);
	}

	// draw background
	g->setColor(0xff222222);
	g->setAlpha(m_fAnimation);
	g->fillRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1);

	// draw frame
	g->setColor(0xffffffff);
	g->setAlpha(m_fAnimation*m_fAnimation);
	g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

	m_container->draw(g);

	if (m_fAnimation > 0.0f && m_fAnimation < 1.0f)
		g->pop3DScene();
}

void OsuUIContextMenu::update()
{
	CBaseUIElement::update();
	if (!m_bVisible || !m_bVisible2) return;

	m_container->update();
}

void OsuUIContextMenu::onResized()
{
	m_container->setSize(m_vSize);
}

void OsuUIContextMenu::onMoved()
{
	m_container->setPos(m_vPos);
}

void OsuUIContextMenu::onMouseDownOutside()
{
	setVisible2(false);
}

void OsuUIContextMenu::onFocusStolen()
{
	m_container->stealFocus();
}

void OsuUIContextMenu::begin(int minWidth)
{
	m_clickCallback = NULL;
	m_iYCounter = 0;
	m_iWidthCounter = minWidth;
	setSizeX(m_iWidthCounter);
	m_container->clear();
}

CBaseUIButton *OsuUIContextMenu::addButton(UString text)
{
	int buttonHeight = 30;
	int margin = 9;

	CBaseUIButton *button = new CBaseUIButton(margin, m_iYCounter + margin, 0, buttonHeight, text, text);
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUIContextMenu::onClick) );
	button->setWidthToContent(3);
	button->setTextLeft(true);
	button->setDrawFrame(false);
	button->setDrawBackground(false);
	m_container->addBaseUIElement(button);

	if (button->getSize().x+2*margin > m_iWidthCounter)
	{
		m_iWidthCounter = button->getSize().x + 2*margin;
		setSizeX(m_iWidthCounter);
	}

	m_iYCounter += buttonHeight;
	setSizeY(m_iYCounter + 2*margin);

	return button;
}

void OsuUIContextMenu::end(bool invertAnimation)
{
	m_bInvertAnimation = invertAnimation;

	int margin = 9;

	std::vector<CBaseUIElement*> *elements = m_container->getAllBaseUIElementsPointer();
	for (int i=0; i<elements->size(); i++)
	{
		((*elements)[i])->setSizeX(m_iWidthCounter-2*margin);
	}

	setVisible2(true);

	m_fAnimation = 0.001f;
	anim->moveQuartOut(&m_fAnimation, 1.0f, 0.15f, true);
}

void OsuUIContextMenu::setVisible2(bool visible2)
{
	m_bVisible2 = visible2;

	if (!m_bVisible2)
		setSize(1, 1); // reset size

	if (m_parent != NULL)
		m_parent->setScrollSizeToContent(); // and update parent scroll size
}

void OsuUIContextMenu::onClick(CBaseUIButton *button)
{
	setVisible2(false);

	if (m_clickCallback != NULL)
		m_clickCallback(button->getName());
}

