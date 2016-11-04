//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		static renderer class, so it can be used outside of OsuSlider
//
// $NoKeywords: $sliderrender
//===============================================================================//

#include "OsuSliderRenderer.h"

#include "Engine.h"
#include "Shader.h"
#include "VertexArrayObject.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"

Shader *OsuSliderRenderer::BLEND_SHADER = NULL;

int OsuSliderRenderer::UNIT_CONE_DIVIDES = 42; // can probably lower this a little bit, but not under 32
std::vector<float> OsuSliderRenderer::UNIT_CONE;
VertexArrayObject *OsuSliderRenderer::MASTER_CIRCLE_VAO = NULL;
float OsuSliderRenderer::MASTER_CIRCLE_VAO_RADIUS = 0.0f;

float OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
float OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;

ConVar osu_slider_debug("osu_slider_debug", false);
ConVar osu_slider_rainbow("osu_slider_rainbow", false);

ConVar osu_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f);
ConVar osu_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f);
ConVar osu_slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f);

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_body_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f)
		return;

	checkUpdateVars(hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// debug sliders
	if (osu_slider_debug.getBool())
	{
		const float circleImageScale = hitcircleDiameter / (float) osu->getSkin()->getHitCircle()->getWidth();

		g->setColor(color);
		g->setAlpha(alpha);
		for (int i=drawFromIndex; i<drawUpToIndex; i++)
		{
			VertexArrayObject vao;
			vao.setType(VertexArrayObject::TYPE_QUADS);
			Vector2 point = points[i];

			int width = osu->getSkin()->getHitCircle()->getWidth();
			int height = osu->getSkin()->getHitCircle()->getHeight();

			g->pushTransform();
				g->scale(circleImageScale, circleImageScale);
				g->translate(point.x, point.y);

				int x = -width/2;
				int y = -height/2;

				vao.addTexcoord(0, 0);
				vao.addVertex(x, y, -1.0f);

				vao.addTexcoord(0, 1);
				vao.addVertex(x, y+height, -1.0f);

				vao.addTexcoord(1, 1);
				vao.addVertex(x+width, y+height, -1.0f);

				vao.addTexcoord(1, 0);
				vao.addVertex(x+width, y, -1.0f);

				osu->getSkin()->getHitCircle()->bind();
				g->drawVAO(&vao);
				osu->getSkin()->getHitCircle()->unbind();
			g->popTransform();
		}
		return; // nothing more to draw here
	}

	// reset
	m_fBoundingBoxMinX = std::numeric_limits<float>::max();
	m_fBoundingBoxMaxX = 0.0f;
	m_fBoundingBoxMinY = std::numeric_limits<float>::max();
	m_fBoundingBoxMaxY = 0.0f;

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getFrameBuffer()->enable();

		Color borderColor = osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		// bit of a hack, but it works. better than rough edges
		if (osu_slider_border_size_multiplier.getFloat() < 0.01f)
			borderColor = bodyColor;

		BLEND_SHADER->enable();
		BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
		BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
		BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
		BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();
		drawFillSliderBody2(g, points, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

		BLEND_SHADER->disable();

		osu->getFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getFrameBuffer()->setColor(COLORf(alpha*osu_slider_body_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderRenderer::drawFillSliderBody2(Graphics *g, const std::vector<Vector2> &points, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	g->pushTransform();

		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			g->translate(x-startX, y-startY, 0);
			g->drawVAO(MASTER_CIRCLE_VAO);

			startX = x;
			startY = y;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}

	g->popTransform();
}

void OsuSliderRenderer::checkUpdateVars(float hitcircleDiameter)
{
	// static globals

	// build shader
	if (BLEND_SHADER == NULL)
	{
		// build shader
		BLEND_SHADER = new Shader("slider.vsh", "slider.fsh");

		// build unit cone
		{
			// tip of the cone
			// texture coordinates
			UNIT_CONE.push_back(1.0f);
			UNIT_CONE.push_back(0.0f);

			// position
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);

			for (int j=0; j<UNIT_CONE_DIVIDES; ++j)
			{
				float phase = j * (float) PI * 2.0f / UNIT_CONE_DIVIDES;

				// texture coordinates
				UNIT_CONE.push_back(0.0f);
				UNIT_CONE.push_back(0.0f);

				// positon
				UNIT_CONE.push_back((float)std::sin(phase));
				UNIT_CONE.push_back((float)std::cos(phase));
				UNIT_CONE.push_back(-1.0f);
			}

			// texture coordinates
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);

			// positon
			UNIT_CONE.push_back((float)std::sin(0.0f));
			UNIT_CONE.push_back((float)std::cos(0.0f));
			UNIT_CONE.push_back(-1.0f);
		}
	}

	// build vao
	if (MASTER_CIRCLE_VAO == NULL)
	{
		MASTER_CIRCLE_VAO = new VertexArrayObject();
		MASTER_CIRCLE_VAO->setType(VertexArrayObject::TYPE_TRIANGLE_FAN);
	}

	// generate master circle mesh (centered) if the size changed
	float radius = hitcircleDiameter/2.0f;
	if (radius != MASTER_CIRCLE_VAO_RADIUS)
	{
		MASTER_CIRCLE_VAO_RADIUS = radius;
		MASTER_CIRCLE_VAO->clear();
		for (int i=0; i<UNIT_CONE.size()/5; i++)
		{
			MASTER_CIRCLE_VAO->addVertex(Vector3((radius * UNIT_CONE[i * 5 + 2]), (radius * UNIT_CONE[i * 5 + 3]), UNIT_CONE[i * 5 + 4]));
			MASTER_CIRCLE_VAO->addTexcoord(Vector2(UNIT_CONE[i * 5 + 0], UNIT_CONE[i * 5 + 1]));
		}
	}
}
