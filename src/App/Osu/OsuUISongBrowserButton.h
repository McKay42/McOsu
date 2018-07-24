//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser button base class
//
// $NoKeywords: $osusbb
//===============================================================================//

#ifndef OSUUISONGBROWSERBUTTON_H
#define OSUUISONGBROWSERBUTTON_H

#include "CBaseUIButton.h"

class Osu;
class OsuBeatmap;
class OsuSongBrowser2;

class CBaseUIScrollView;

class OsuUISongBrowserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name);
	virtual ~OsuUISongBrowserButton();
	void deleteAnimations();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void updateLayoutEx();

	void select();
	void deselect();

	void resetAnimations();

	OsuUISongBrowserButton *setVisible(bool visible);
	void setTargetRelPosY(float targetRelPosY);
	void setChildren(std::vector<OsuUISongBrowserButton*> children) {m_children = children;}
	void setActiveBackgroundColor(Color activeBackgroundColor) {m_activeBackgroundColor = activeBackgroundColor;}
	void setInactiveBackgroundColor(Color inactiveBackgroundColor) {m_inactiveBackgroundColor = inactiveBackgroundColor;}
	void setOffsetPercent(float offsetPercent) {m_fOffsetPercent = offsetPercent;}
	void setHideIfSelected(bool hideIfSelected) {m_bHideIfSelected = hideIfSelected;}

	void setCollectionDiffHack(bool collectionDiffHack) {m_bCollectionDiffHack = collectionDiffHack;}

	Vector2 getActualOffset();
	inline Vector2 getActualSize() {return m_vSize - 2*getActualOffset();}
	inline Vector2 getActualPos() {return m_vPos + getActualOffset();}
	inline std::vector<OsuUISongBrowserButton*> getChildrenAbs() {return m_children;}

	inline bool getCollectionDiffHack() const {return m_bCollectionDiffHack;}
	inline int getSortHack() const {return m_iSortHack;}

	virtual OsuBeatmap *getBeatmap() const {return NULL;}
	virtual std::vector<OsuUISongBrowserButton*> getChildren() {return m_children;}

	inline bool isSelected() const {return m_bSelected;}
	inline bool isHiddenIfSelected() const {return m_bHideIfSelected;}

protected:
	void drawMenuButtonBackground(Graphics *g);

	virtual void onSelected(bool wasSelected) {;}
	virtual void onDeselected() {;}

	Osu *m_osu;
	CBaseUIScrollView *m_view;
	OsuSongBrowser2 *m_songBrowser;

	McFont *m_font;
	McFont *m_fontBold;

	bool m_bSelected;
	bool m_bCollectionDiffHack;

	std::vector<OsuUISongBrowserButton*> m_children;

private:
	static int marginPixelsX;
	static int marginPixelsY;
	static float lastHoverSoundTime;
	static int sortHackCounter;

	enum class MOVE_AWAY_STATE
	{
		MOVE_CENTER,
		MOVE_UP,
		MOVE_DOWN
	};

	virtual void onClicked();
	virtual void onMouseInside();
	virtual void onMouseOutside();

	void setMoveAwayState(MOVE_AWAY_STATE moveAwayState, bool animate = true);

	float m_fTargetRelPosY;
	float m_fScale;
	float m_fOffsetPercent;
	float m_fHoverOffsetAnimation;
	float m_fHoverMoveAwayAnimation;
	float m_fCenterOffsetAnimation;
	float m_fCenterOffsetVelocityAnimation;

	int m_iSortHack;

	bool m_bHideIfSelected;

	Color m_activeBackgroundColor;
	Color m_inactiveBackgroundColor;

	MOVE_AWAY_STATE m_moveAwayState;
};

#endif
