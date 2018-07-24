//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		clickable button displaying score, grade, name, acc, mods, combo
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISONGBROWSERSCOREBUTTON_H
#define OSUUISONGBROWSERSCOREBUTTON_H

#include "CBaseUIButton.h"

#include "OsuScore.h"
#include "OsuDatabase.h"

class Osu;
class OsuSkinImage;

class OsuUIContextMenu;

class OsuUISongBrowserScoreButton : public CBaseUIButton
{
public:
	static OsuSkinImage *getGradeImage(Osu *osu, OsuScore::GRADE grade);

public:
	OsuUISongBrowserScoreButton(Osu *osu, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name);
	virtual ~OsuUISongBrowserScoreButton();

	void draw(Graphics *g);
	void update();

	void setScore(OsuDatabase::Score score, int index = 1);
	void setIndex(int index) {m_iScoreIndexNumber = index;}

	inline OsuDatabase::Score getScore() const {return m_score;}
	inline uint64_t getScoreUnixTimestamp() const {return m_score.unixTimestamp;}
	inline unsigned long long getScoreScore() const {return m_score.score;}
	inline float getScorePP() const {return m_score.pp;}

	inline UString getDateTime() const {return m_sScoreDateTime;}
	inline int getIndex() const {return m_iScoreIndexNumber;}

private:
	static ConVar *m_osu_scores_sort_by_pp;
	static UString recentScoreIconString;

	void updateElapsedTimeString();

	virtual void onClicked();

	virtual void onMouseInside();
	virtual void onMouseOutside();

	virtual void onFocusStolen();

	void onRightMouseUpInside();
	void onContextMenu(UString text);

	bool isContextMenuVisible();


	UString getModsString(int mods);

	Osu *m_osu;
	OsuUIContextMenu *m_contextMenu;
	float m_fIndexNumberAnim;

	bool m_bRightClick;
	bool m_bRightClickCheck;

	// score data
	OsuDatabase::Score m_score;
	int m_iScoreIndexNumber;
	uint64_t m_iScoreUnixTimestamp;

	OsuScore::GRADE m_scoreGrade;

	UString m_sScoreTime;
	UString m_sScoreUsername;
	UString m_sScoreScore;
	UString m_sScoreScorePP;
	UString m_sScoreAccuracy;
	UString m_sScoreMods;

	std::vector<UString> m_tooltipLines;
	UString m_sScoreDateTime;
};

#endif
