//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		static renderer class, so it can be used outside of OsuSlider
//
// $NoKeywords: $sliderrender
//===============================================================================//

#ifndef OSUSLIDERRENDERER_H
#define OSUSLIDERRENDERER_H

#include "cbase.h"

class Shader;
class VertexArrayObject;

class Osu;

class OsuSliderRenderer
{
public:
	static Shader *BLEND_SHADER;

public:
	static void draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color color = 0xffffffff, float alpha = 1.0f, long sliderTimeForRainbow = 0);

private:
	static void drawFillSliderBody2(Graphics *g, const std::vector<Vector2> &points, float radius, int drawFromIndex = -1, int drawUpToIndex = -1);

	static void checkUpdateVars(float hitcircleDiameter);

	// base mesh
	static int UNIT_CONE_DIVIDES;
	static std::vector<float> UNIT_CONE;
	static VertexArrayObject *MASTER_CIRCLE_VAO;
	static float MASTER_CIRCLE_VAO_RADIUS;

	// tiny rendering optimization
	static float m_fBoundingBoxMinX;
	static float m_fBoundingBoxMaxX;
	static float m_fBoundingBoxMinY;
	static float m_fBoundingBoxMaxY;
};

#endif
