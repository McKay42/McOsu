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
class OsuDatabaseBeatmap;
class OsuSongBrowser2;
class OsuUIContextMenu;

class CBaseUIScrollView;

class OsuUISongBrowserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name);
	virtual ~OsuUISongBrowserButton();
	void deleteAnimations();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void updateLayoutEx();

	OsuUISongBrowserButton *setVisible(bool visible);

	void select(bool fireCallbacks = true, bool wasClicked = false);
	void deselect();

	void resetAnimations();

	void setTargetRelPosY(float targetRelPosY);
	void setChildren(std::vector<OsuUISongBrowserButton*> children) {m_children = children;}
	void setActiveBackgroundColor(Color activeBackgroundColor) {m_activeBackgroundColor = activeBackgroundColor;}
	void setInactiveBackgroundColor(Color inactiveBackgroundColor) {m_inactiveBackgroundColor = inactiveBackgroundColor;}
	void setOffsetPercent(float offsetPercent) {m_fOffsetPercent = offsetPercent;}
	void setHideIfSelected(bool hideIfSelected) {m_bHideIfSelected = hideIfSelected;}
	void setIsSearchMatch(bool isSearchMatch) {m_bIsSearchMatch = isSearchMatch;}

	Vector2 getActualOffset() const;
	inline Vector2 getActualSize() const {return m_vSize - 2*getActualOffset();}
	inline Vector2 getActualPos() const {return m_vPos + getActualOffset();}
	inline std::vector<OsuUISongBrowserButton*> &getChildren() {return m_children;}
	inline int getSortHack() const {return m_iSortHack;}

	virtual OsuDatabaseBeatmap *getDatabaseBeatmap() const {return NULL;}

	inline bool isSelected() const {return m_bSelected;}
	inline bool isHiddenIfSelected() const {return m_bHideIfSelected;}
	inline bool isSearchMatch() const {return m_bIsSearchMatch.load();}

protected:
	void drawMenuButtonBackground(Graphics *g);

	virtual void onSelected(bool wasSelected, bool wasClicked) {;}
	virtual void onRightMouseUpInside() {;}

	Osu *m_osu;
	CBaseUIScrollView *m_view;
	OsuSongBrowser2 *m_songBrowser;
	OsuUIContextMenu *m_contextMenu;

	McFont *m_font;
	McFont *m_fontBold;

	bool m_bSelected;

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

	bool m_bRightClick;
	bool m_bRightClickCheck;

	float m_fTargetRelPosY;
	float m_fScale;
	float m_fOffsetPercent;
	float m_fHoverOffsetAnimation;
	float m_fHoverMoveAwayAnimation;
	float m_fCenterOffsetAnimation;
	float m_fCenterOffsetVelocityAnimation;

	int m_iSortHack;
	std::atomic<bool> m_bIsSearchMatch;

	bool m_bHideIfSelected;

	Color m_activeBackgroundColor;
	Color m_inactiveBackgroundColor;

	MOVE_AWAY_STATE m_moveAwayState;
};

#endif
