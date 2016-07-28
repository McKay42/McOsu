//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		base class for all gameplay objects
//
// $NoKeywords: $hitobj
//===============================================================================//

#ifndef OSUHITOBJECT_H
#define OSUHITOBJECT_H

#include "OsuBeatmap.h"

class OsuHitObject
{
public:
	static void drawHitResult(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, OsuBeatmap::HIT result, float animPercent);
	static void drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuBeatmap::HIT result, float animPercent);

public:
	OsuHitObject(long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap);
	virtual ~OsuHitObject() {;}

	virtual void draw(Graphics *g);
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset) = 0;
	void addHitResult(OsuBeatmap::HIT result, long delta, Vector2 posRaw, float targetDelta = 0.0f, float targetAngle = 0.0f, bool ignoreOnHitErrorBar = false, bool ignoreCombo = false);
	void misAimed() {m_bMisAim = true;}

	void setStack(int stack) {m_iStack = stack;}
	void setForceDrawApproachCircle(bool firstNote) {m_bOverrideHDApproachCircle = firstNote;}
	void setAutopilotDelta(long delta) {m_iAutopilotDelta = delta;}

	inline long getTime() const {return m_iTime;}
	inline long getDuration() const {return m_iObjectDuration;}
	inline int getStack() const {return m_iStack;}
	inline int getComboNumber() const {return m_iComboNumber;}
	inline long getAutopilotDelta() const {return m_iAutopilotDelta;}

	inline bool isVisible() const {return m_bVisible;}
	inline bool isFinished() const {return m_bFinished;}
	inline bool hasMisAimed() const {return m_bMisAim;}

	virtual Vector2 getRawPos() = 0;
	virtual Vector2 getRawPosAt(long pos) = 0;
	virtual Vector2 getAutoCursorPos(long curPos) = 0;

	virtual void onClickEvent(Vector2 cursorPos, std::vector<OsuBeatmap::CLICK> &clicks) {;}
	virtual void onReset(long curPos);

protected:
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
	long m_iFadeInTime; // extra time added before the approachTime to let the object smoothly become visible
	long m_iObjectTime; // the duration an object is visible (excluding the exact moment in time where it has to be clicked and onwards, this also includes the extra fadeInTime)
	long m_iObjectDuration; // how long this object takes to click (circle = 0, slider = sliderTime, spinner = spinnerTime etc.), the object will stay visible this long extra after m_iTime;
							// it should be set by the actual object inheriting from this class

	long m_iHiddenDecayTime;
	long m_iHiddenTimeDiff;

	int m_iStack;

	bool m_bOverrideHDApproachCircle;
	bool m_bMisAim;
	long m_iAutopilotDelta;

private:
	struct HITRESULTANIM
	{
		float anim;
		Vector2 rawPos;
		OsuBeatmap::HIT result;
	};

	std::vector<HITRESULTANIM> m_hitResults;
};

#endif
