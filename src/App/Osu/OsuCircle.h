//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		circle
//
// $NoKeywords: $circle
//===============================================================================//

#ifndef OSUCIRCLE_H
#define OSUCIRCLE_H

#include "OsuHitObject.h"

class OsuCircle : public OsuHitObject
{
public:
	// main
	static void drawCircle(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, bool modHD, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderCircle(Graphics *g, OsuBeatmap *beatmap, Image *hitCircleImage, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);

	// split helper functions
	static void drawApproachCircle(Graphics *g, OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
	static void drawHitCircleOverlay(Graphics *g, OsuSkin *skin, Vector2 pos, float circleOverlayImageScale, float alpha);
	static void drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha);
	static void drawHitCircleNumber(Graphics *g, OsuBeatmap *beatmap, Vector2 pos, int number, float numberAlpha);
	static void drawHitCircleNumber(Graphics *g, OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha);

public:
	OsuCircle(int x, int y, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap);
	virtual ~OsuCircle() {;}

	virtual void draw(Graphics *g);
	virtual void update(long curPos);

	void updateStackPosition(float stackOffset);

	inline Vector2 getRawPos() {return m_vRawPos;}
	Vector2 getRawPosAt(long pos) {return m_vRawPos;}
	Vector2 getAutoCursorPos(long curPos);

	virtual void onClickEvent(Vector2 cursorPos, std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onReset(long curPos);

private:
	// necessary due to the static draw functions
	static int rainbowNumber;
	static int rainbowColorCounter;

	void onHit(OsuBeatmap::HIT result, long delta);

	Vector2 m_vRawPos;

	bool m_bWaiting;
	float m_fHitAnimation;
};

#endif
