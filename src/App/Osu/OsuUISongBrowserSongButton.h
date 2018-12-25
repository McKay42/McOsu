//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#ifndef OSUUISONGBROWSERSONGBUTTON_H
#define OSUUISONGBROWSERSONGBUTTON_H

#include "OsuUISongBrowserButton.h"
#include "OsuScore.h"

class OsuSongBrowser2;
class OsuBeatmap;
class OsuBeatmapDifficulty;

class OsuUISongBrowserSongButton : public OsuUISongBrowserButton
{
public:
	OsuUISongBrowserSongButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, OsuBeatmap *beatmap);
	virtual ~OsuUISongBrowserSongButton();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void updateLayoutEx();
	virtual void updateGrade() {;}

	OsuUISongBrowserSongButton *setVisible(bool visible);
	OsuUISongBrowserSongButton *setParent(OsuUISongBrowserSongButton *parent) {m_parent = parent; return this;}

	virtual OsuBeatmap *getBeatmap() const {return m_beatmap;}
	virtual std::vector<OsuUISongBrowserButton*> getChildren();

	inline OsuBeatmapDifficulty *getDiff() const {return m_diff;}
	inline OsuUISongBrowserSongButton *getParent() const {return m_parent;}

protected:
	virtual void onSelected(bool wasSelected);
	virtual void onDeselected();

	void drawBeatmapBackgroundThumbnail(Graphics *g, Image *image);
	void drawGrade(Graphics *g);
	void drawTitle(Graphics *g, float deselectedAlpha = 1.0f);
	void drawSubTitle(Graphics *g, float deselectedAlpha = 1.0f);

	void checkLoadUnloadImage();

	float calculateGradeScale();
	float calculateGradeWidth();

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

	OsuBeatmapDifficulty *m_diff;

	UString m_sTitle;
	UString m_sArtist;
	UString m_sMapper;
	OsuScore::GRADE m_grade;
	bool m_bHasGrade;

	float m_fTextOffset;
	float m_fGradeOffset;
	float m_fTextSpacingScale;
	float m_fTextMarginScale;
	float m_fTitleScale;
	float m_fSubTitleScale;
	float m_fGradeScale;

private:
	static float thumbnailYRatio;
	static OsuUISongBrowserSongButton *previousButton;

	OsuBeatmap *m_beatmap;
	OsuUISongBrowserSongButton *m_parent;

	float m_fImageLoadScheduledTime;
	float m_fThumbnailFadeInTime;
};

#endif
