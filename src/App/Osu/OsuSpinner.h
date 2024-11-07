//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		spinner. spin logic has been taken from opsu!, I didn't have time to rewrite it yet
//
// $NoKeywords: $spin
//=====================================================================================================//

#ifndef OSUSPINNER_H
#define OSUSPINNER_H

#include "OsuHitObject.h"

class OsuSpinner : public OsuHitObject
{
public:
	OsuSpinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, OsuBeatmapStandard *beatmap);
	virtual ~OsuSpinner();

	virtual void draw(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void draw3D(Graphics *g);
	virtual void update(long curPos);

	virtual bool isSpinner() {return true;}

	void updateStackPosition(float stackOffset) {;}
	void miss(long curPos) {;}

	Vector2 getRawPosAt(long pos) {return m_vRawPos;}
	Vector2 getOriginalRawPosAt(long pos) {return m_vOriginalRawPos;}
	Vector2 getAutoCursorPos(long curPos);

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onReset(long curPos);

private:
	void onHit();
	void rotate(float rad);

	OsuBeatmapStandard *m_beatmap;

	Vector2 m_vRawPos;
	Vector2 m_vOriginalRawPos;

	bool m_bClickedOnce;
	float m_fPercent;

	float m_fDrawRot;
	float m_fRotations;
	float m_fRotationsNeeded;
	float m_fDeltaOverflow;
	float m_fSumDeltaAngle;

	int m_iMaxStoredDeltaAngles;
	float *m_storedDeltaAngles;
	int m_iDeltaAngleIndex;
	float m_fDeltaAngleOverflow;

	float m_fRPM;

	float m_fLastMouseAngle;
	float m_fLastVRCursorAngle1;
	float m_fLastVRCursorAngle2;
	float m_fRatio;
};

#endif
