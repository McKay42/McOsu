//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser element (beatmap, diff, collection, group, etc.)
//
// $NoKeywords: $osusbb
//===============================================================================//

#ifndef OSUUISONGBROWSERBUTTON_H
#define OSUUISONGBROWSERBUTTON_H

#include "CBaseUIButton.h"

class OsuBeatmap;
class OsuBeatmapDifficulty;
class OsuSongBrowser2;

class CBaseUIScrollView;

class OsuUISongBrowserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserButton(OsuBeatmap *beatmap, CBaseUIScrollView *view, OsuSongBrowser2 *songBrowser, float xPos, float yPos, float xSize, float ySize, UString name, OsuUISongBrowserButton *parent = NULL, OsuBeatmapDifficulty *child = NULL, bool isFirstChild = false);
	~OsuUISongBrowserButton();
	void deleteAnimations();

	virtual void draw(Graphics *g);
	virtual void update();

	void updateLayout();

	void select(bool clicked = false, bool selectTopChild = false);
	void deselect(bool child = false);

	void setVisible(bool visible);

	void setTitle(UString title) {m_sTitle = title;}
	void setArtist(UString artist) {m_sArtist = artist;}
	void setMapper(UString mapper) {m_sMapper = mapper;}
	void setDiff(UString diff) {m_sDiff = diff;}

	inline OsuBeatmap *getBeatmap() const {return m_beatmap;}

	Vector2 getActualOffset();
	inline Vector2 getActualSize() {return m_vSize - 2*getActualOffset();}
	inline Vector2 getActualPos() {return m_vPos + getActualOffset();}

	inline std::vector<OsuUISongBrowserButton*> getChildren() {return m_children;}

	inline bool isChild() {return m_child != NULL;}
	inline bool isSelected() const {return m_bSelected;}

private:
	static OsuUISongBrowserButton *previousParent;
	static OsuUISongBrowserButton *previousChild;
	static int marginPixelsX;
	static int marginPixelsY;
	static float thumbnailYRatio;
	static float lastHoverSoundTime;
	static Color inactiveBackgroundColor;
	static Color activeBackgroundColor;
	static Color inactiveDifficultyBackgroundColor;

	void checkLoadUnloadImage();

	virtual void onClicked();
	virtual void onMouseInside();
	virtual void onMouseOutside();

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
	OsuUISongBrowserButton *m_parent;
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

	std::vector<OsuUISongBrowserButton*> m_children;
};

#endif
