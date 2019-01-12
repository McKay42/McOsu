//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		context menu, dropdown style
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUICONTEXTMENU_H
#define OSUUICONTEXTMENU_H

#include "CBaseUIElement.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIButton;

class Osu;

class OsuUIContextMenu : public CBaseUIElement
{
public:
	OsuUIContextMenu(Osu *osu, float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "", CBaseUIScrollView *parent = NULL);
	virtual ~OsuUIContextMenu();

	virtual void draw(Graphics *g);
	virtual void update();

	typedef fastdelegate::FastDelegate2<UString, int> ButtonClickCallback;
	void setClickCallback(ButtonClickCallback clickCallback) {m_clickCallback = clickCallback;}

	void begin(int minWidth = 0);
	CBaseUIButton *addButton(UString text, int id = -1);
	void end(bool invertAnimation = false);

	void setVisible2(bool visible2);

	virtual bool isVisible() {return m_bVisible && m_bVisible2;}

private:
	virtual void onResized();
	virtual void onMoved();
	virtual void onMouseDownOutside();
	virtual void onFocusStolen();

	void onClick(CBaseUIButton *button);

	Osu *m_osu;

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_parent;

	ButtonClickCallback m_clickCallback;

	int m_iYCounter;
	int m_iWidthCounter;

	bool m_bVisible2;
	float m_fAnimation;
	bool m_bInvertAnimation;
};

#endif
