//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		base class for all gameplay objects
//
// $NoKeywords: $hitobj
//===============================================================================//

#ifndef OSUHITOBJECT_H
#define OSUHITOBJECT_H

#include "OsuBeatmap.h"

class ConVar;

class OsuBeatmapStandard;

class OsuHitObject
{
public:
	static void drawHitResult(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent);
	static void drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent);

public:
	OsuHitObject(long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap);
	virtual ~OsuHitObject() {;}

	virtual void draw(Graphics *g);
	virtual void draw2(Graphics *g){;}
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr){;}
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset) = 0;
	void addHitResult(OsuScore::HIT result, long delta, Vector2 posRaw, float targetDelta = 0.0f, float targetAngle = 0.0f, bool ignoreOnHitErrorBar = false, bool ignoreCombo = false);
	void misAimed() {m_bMisAim = true;}

	void setStack(int stack) {m_iStack = stack;}
	void setForceDrawApproachCircle(bool firstNote) {m_bOverrideHDApproachCircle = firstNote;}
	void setAutopilotDelta(long delta) {m_iAutopilotDelta = delta;}
	void setBlocked(bool blocked) {m_bBlocked = blocked;}

	virtual Vector2 getRawPosAt(long pos) = 0; // with stack calculation modifications
	virtual Vector2 getOriginalRawPosAt(long pos) = 0; // without stack calculations
	virtual Vector2 getAutoCursorPos(long curPos) = 0;

	inline long getTime() const {return m_iTime;}
	inline long getDuration() const {return m_iObjectDuration;}
	inline int getStack() const {return m_iStack;}
	inline int getComboNumber() const {return m_iComboNumber;}
	inline long getAutopilotDelta() const {return m_iAutopilotDelta;}

	inline bool isVisible() const {return m_bVisible;}
	inline bool isFinished() const {return m_bFinished;}
	inline bool isBlocked() const {return m_bBlocked;}
	inline bool hasMisAimed() const {return m_bMisAim;}

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) {;}
	virtual void onReset(long curPos);

protected:
	static ConVar *m_osu_approach_scale_multiplier_ref;

	OsuBeatmap *m_beatmap;

	bool m_bVisible;
	bool m_bFinished;

	long m_iTime; // the time at which this object must be clicked
	int m_iSampleType;
	int m_iComboNumber;
	int m_iColorCounter;

	float m_fAlpha;
	float m_fApproachScale;
	float m_fFadeInScale;
	long m_iDelta; // this must be signed
	long m_iApproachTime;
	long m_iFadeInTime; // extra time added before the approachTime to let the object smoothly become visible
	long m_iObjectTime; // the duration an object is visible (excluding the exact moment in time where it has to be clicked and onwards, this also includes the extra fadeInTime)
	long m_iObjectDuration; // how long this object takes to click (circle = 0, slider = sliderTime, spinner = spinnerTime etc.), the object will stay visible this long extra after m_iTime;
							// it should be set by the actual object inheriting from this class

	long m_iHiddenDecayTime;
	long m_iHiddenTimeDiff;

	int m_iStack;

	bool m_bBlocked;
	bool m_bOverrideHDApproachCircle;
	bool m_bMisAim;
	long m_iAutopilotDelta;

private:
	struct HITRESULTANIM
	{
		float anim;
		float duration;
		float defaultanim;
		float defaultduration;
		Vector2 rawPos;
		OsuScore::HIT result;
		long delta;
	};

	std::vector<HITRESULTANIM> m_hitResults;
};

#endif
