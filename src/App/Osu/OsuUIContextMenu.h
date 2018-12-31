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
	OsuUIContextMenu(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, CBaseUIScrollView *parent = NULL);
	virtual ~OsuUIContextMenu();

	void draw(Graphics *g);
	void update();

	typedef fastdelegate::FastDelegate1<UString> ButtonClickCallback;
	void setClickCallback(ButtonClickCallback clickCallback) {m_clickCallback = clickCallback;}

	void onResized();
	void onMoved();
	void onMouseDownOutside();
	void onFocusStolen();

	void begin(int minWidth = 0);
	CBaseUIButton *addButton(UString text);
	void end(bool invertAnimation = false);

	void setVisible2(bool visible2);

	bool isVisible() {return m_bVisible && m_bVisible2;}

private:
	Osu *m_osu;

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_parent;

	void onClick(CBaseUIButton *button);
	ButtonClickCallback m_clickCallback;

	int m_iYCounter;
	int m_iWidthCounter;

	bool m_bVisible2;
	float m_fAnimation;
	bool m_bInvertAnimation;
};

#endif
