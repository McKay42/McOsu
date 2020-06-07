//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		single note
//
// $NoKeywords: $osumanian
//===============================================================================//

#ifndef OSU_OSUMANIANOTE_H
#define OSU_OSUMANIANOTE_H

#include "OsuHitObject.h"

class OsuBeatmapMania;

class OsuManiaNote : public OsuHitObject
{
public:
	OsuManiaNote(int column, long sliderTime, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmapMania *beatmap);

	virtual void draw(Graphics *g);
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset) {;}
	virtual void miss(long curPos) {;}

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onKeyUpEvent(std::vector<OsuBeatmap::CLICK> &keyUps);
	virtual void onReset(long curPos);

	virtual Vector2 getRawPosAt(long pos) {return Vector2(0,0);}
	virtual Vector2 getOriginalRawPosAt(long pos) {return Vector2(0,0);}
	virtual Vector2 getAutoCursorPos(long curPos) {return Vector2(0,0);}

private:
	inline bool isHoldNote() {return m_iObjectDuration > 0;}

	void onHit(OsuScore::HIT result, long delta, bool start = false, bool ignoreOnHitErrorBar = false);

	OsuBeatmapMania *m_beatmap;

	int m_iColumn;

	bool m_bStartFinished;
	OsuScore::HIT m_startResult;
	OsuScore::HIT m_endResult;
};

#endif
