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

#include "CBaseUIScrollView.h"

class OsuUISongBrowserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name);
	~OsuUISongBrowserButton();
	void deleteAnimations();

	enum class MOVE_AWAY_STATE
	{
		MOVE_UP,
		MOVE_DOWN,
		MOVE_CENTER
	};

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void updateLayout();

	void select();
	void deselect();

	void setVisible(bool visible);
	void setActiveBackgroundColor(Color activeBackgroundColor) {m_activeBackgroundColor = activeBackgroundColor;}
	void setInactiveBackgroundColor(Color inactiveBackgroundColor) {m_inactiveBackgroundColor = inactiveBackgroundColor;}
	void setOffsetPercent(float offsetPercent) {m_fOffsetPercent = offsetPercent;}
	void setHideIfSelected(bool hideIfSelected) {m_bHideIfSelected = hideIfSelected;}
	void setOffsetAwaySelected(float offsetAwaySelected) {m_fOffsetAwaySelected = offsetAwaySelected;}
	void setMoveAwayState(MOVE_AWAY_STATE moveAway) {moveAwayState = moveAway;}

	Vector2 getActualOffset();
	inline Vector2 getActualSize() {return m_vSize - 2*getActualOffset();}
	inline Vector2 getActualPos() {return m_vPos + getActualOffset();}
	inline float getOffsetAwaySelected() const {return m_fOffsetAwaySelected;}
	inline MOVE_AWAY_STATE getMoveAwayState() const {return moveAwayState;}


	virtual OsuBeatmap *getBeatmap() {return NULL;}
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

	std::vector<OsuUISongBrowserButton*> m_children;

private:
	static int marginPixelsX;
	static int marginPixelsY;
	static float lastHoverSoundTime;
	static float maxOffsetAwaySelected;
	static float moveAwaySpeed;

	virtual void onClicked();
	virtual void onMouseInside();
	virtual void onMouseOutside();

	float m_fScale;
	float m_fOffsetPercent;
	float m_fHoverOffsetAnimation;
	float m_fCenterOffsetAnimation;
	float m_fCenterOffsetVelocityAnimation;
	float m_fOffsetAwaySelected;

	bool m_bHideIfSelected;

	Color m_activeBackgroundColor;
	Color m_inactiveBackgroundColor;


	MOVE_AWAY_STATE moveAwayState;

};

#endif
