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
#include "Environment.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"
#include "DirectX11Interface.h"

Shader *OsuSliderRenderer::BLEND_SHADER = NULL;
Shader *OsuSliderRenderer::BLEND_SHADER_VR = NULL;

float OsuSliderRenderer::MESH_CENTER_HEIGHT = 0.5f; // Camera::buildMatrixOrtho2D() uses -1 to 1 for zn/zf, so don't make this too high
int OsuSliderRenderer::UNIT_CIRCLE_SUBDIVISIONS = 0; // see osu_slider_body_unit_circle_subdivisions now
std::vector<float> OsuSliderRenderer::UNIT_CIRCLE;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_BAKED = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_TRIANGLES = NULL;
float OsuSliderRenderer::UNIT_CIRCLE_VAO_DIAMETER = 0.0f;

float OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
float OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;

ConVar osu_slider_debug_draw("osu_slider_debug_draw", false, FCVAR_NONE, "draw hitcircle at every curve point and nothing else (no vao, no rt, no shader, nothing) (requires enabling legacy slider renderer)");
ConVar osu_slider_debug_draw_square_vao("osu_slider_debug_draw_square_vao", false, FCVAR_NONE, "generate square vaos and nothing else (no rt, no shader) (requires disabling legacy slider renderer)");
ConVar osu_slider_debug_wireframe("osu_slider_debug_wireframe", false, FCVAR_NONE, "unused");

ConVar osu_slider_alpha_multiplier("osu_slider_alpha_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f, FCVAR_NONE);
ConVar osu_slider_border_feather("osu_slider_border_feather", 0.0f, FCVAR_NONE);
ConVar osu_slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_slider_border_tint_combo_color("osu_slider_border_tint_combo_color", false, FCVAR_NONE);
ConVar osu_slider_osu_next_style("osu_slider_osu_next_style", false, FCVAR_NONE);
ConVar osu_slider_rainbow("osu_slider_rainbow", false, FCVAR_NONE);
ConVar osu_slider_use_gradient_image("osu_slider_use_gradient_image", false, FCVAR_NONE);

ConVar osu_slider_body_unit_circle_subdivisions("osu_slider_body_unit_circle_subdivisions", 42, FCVAR_NONE);
ConVar osu_slider_legacy_use_baked_vao("osu_slider_legacy_use_baked_vao", false, FCVAR_NONE, "use baked cone mesh instead of raw mesh for legacy slider renderer (disabled by default because usually slower on very old gpus even though it should not be)");

VertexArrayObject *OsuSliderRenderer::generateVAO(Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, Vector3 translation, bool skipOOBPoints)
{
	checkUpdateVars(osu, hitcircleDiameter);

	engine->getResourceManager()->requestNextLoadUnmanaged();
	VertexArrayObject *vao = engine->getResourceManager()->createVertexArrayObject();

	const Vector3 xOffset = Vector3(hitcircleDiameter, 0, 0);
	const Vector3 yOffset = Vector3(0, hitcircleDiameter, 0);

	const bool debugSquareVao = osu_slider_debug_draw_square_vao.getBool();

	for (int i=0; i<points.size(); i++)
	{
		// fuck oob sliders
		if (skipOOBPoints)
		{
			if (points[i].x < -hitcircleDiameter-OsuGameRules::OSU_COORD_WIDTH*2 || points[i].x > osu->getScreenWidth()+hitcircleDiameter+OsuGameRules::OSU_COORD_WIDTH*2 || points[i].y < -hitcircleDiameter-OsuGameRules::OSU_COORD_HEIGHT*2 || points[i].y > osu->getScreenHeight()+hitcircleDiameter+OsuGameRules::OSU_COORD_HEIGHT*2)
				continue;
		}

		if (!debugSquareVao)
		{
			const std::vector<Vector3> &meshVertices = UNIT_CIRCLE_VAO_TRIANGLES->getVertices();
			const std::vector<std::vector<Vector2>> &meshTexCoords = UNIT_CIRCLE_VAO_TRIANGLES->getTexcoords();
			for (int v=0; v<meshVertices.size(); v++)
			{
				vao->addVertex(meshVertices[v] + Vector3(points[i].x, points[i].y, 0) + translation);
				vao->addTexcoord(meshTexCoords[0][v]);
			}
		}
		else
		{
			const Vector3 topLeft = Vector3(points[i].x, points[i].y, 0) - xOffset/2.0f - yOffset/2.0f + translation;
			const Vector3 topRight = topLeft + xOffset;
			const Vector3 bottomLeft = topLeft + yOffset;
			const Vector3 bottomRight = bottomLeft + xOffset;

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomLeft);
			vao->addTexcoord(0, 1);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topRight);
			vao->addTexcoord(1, 0);
		}
	}

	if (vao->getNumVertices() > 0)
		engine->getResourceManager()->loadResource(vao);
	else
		debugLog("OsuSliderRenderer::generateSliderVAO() ERROR: Zero triangles!\n");

	return vao;
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// debug sliders
	if (osu_slider_debug_draw.getBool())
	{
		const float circleImageScale = hitcircleDiameter / (float)osu->getSkin()->getHitCircle()->getWidth();
		const float circleImageScaleInv = (1.0f / circleImageScale);

		const float width = (float)osu->getSkin()->getHitCircle()->getWidth();
		const float height = (float)osu->getSkin()->getHitCircle()->getHeight();

		const float x = (-width / 2.0f);
		const float y = (-height / 2.0f);
		const float z = -1.0f;

		g->pushTransform();
		{
			g->scale(circleImageScale, circleImageScale);

			const Color dimmedColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedColor)*colorRGBMultiplier));

			g->setColor(dimmedColor);
			g->setAlpha(alpha*osu_slider_alpha_multiplier.getFloat());
			osu->getSkin()->getHitCircle()->bind();
			{
				for (int i=drawFromIndex; i<drawUpToIndex; i++)
				{
					const Vector2 point = points[i] * circleImageScaleInv;

					static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
					vao.empty();
					{
						vao.addTexcoord(0, 0);
						vao.addVertex(point.x + x, point.y + y, z);

						vao.addTexcoord(0, 1);
						vao.addVertex(point.x + x, point.y + y + height, z);

						vao.addTexcoord(1, 1);
						vao.addVertex(point.x + x + width, point.y + y + height, z);

						vao.addTexcoord(1, 0);
						vao.addVertex(point.x + x + width, point.y + y, z);
					}
					g->drawVAO(&vao);
				}
			}
			osu->getSkin()->getHitCircle()->unbind();
		}
		g->popTransform();

		return; // nothing more to draw here
	}

	// reset
	resetRenderTargetBoundingBox();

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();
		{
			const Color undimmedBorderColor = osu_slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
			const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

			Color dimmedBorderColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBorderColor)*colorRGBMultiplier));
			Color dimmedBodyColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBodyColor)*colorRGBMultiplier));

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

				dimmedBorderColor = COLOR(255, red1, green1, blue1);
				dimmedBodyColor = COLOR(255, red2, green2, blue2);
			}

			if (!osu_slider_use_gradient_image.getBool())
			{
				BLEND_SHADER->enable();
				BLEND_SHADER->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
				BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
				BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
				BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
				BLEND_SHADER->setUniform1f("borderFeather", osu_slider_border_feather.getFloat());
				BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(dimmedBorderColor), COLOR_GET_Gf(dimmedBorderColor), COLOR_GET_Bf(dimmedBorderColor));
				BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(dimmedBodyColor), COLOR_GET_Gf(dimmedBodyColor), COLOR_GET_Bf(dimmedBodyColor));
			}

			g->setColor(COLORf(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // this only affects the gradient image if used (meaning shaders either don't work or are disabled on purpose)
			osu->getSkin()->getSliderGradient()->bind();
			{
				// draw curve mesh
				{
					drawFillSliderBodyPeppy(g, osu, points, (osu_slider_legacy_use_baked_vao.getBool() ? UNIT_CIRCLE_VAO_BAKED : UNIT_CIRCLE_VAO), hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex, BLEND_SHADER);

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}
		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	const float pixelFudge = 2.0f;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->drawRect(g, (int)m_fBoundingBoxMinX, (int)m_fBoundingBoxMinY, (int)(m_fBoundingBoxMaxX - m_fBoundingBoxMinX), (int)(m_fBoundingBoxMaxY - m_fBoundingBoxMinY));
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, VertexArrayObject *vao, Vector4 bounds, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow, bool doEnableRenderTarget, bool doDisableRenderTarget, bool doDrawSliderFrameBufferToScreen)
{
	if ((osu_slider_alpha_multiplier.getFloat() <= 0.0f && doDrawSliderFrameBufferToScreen) || (alpha <= 0.0f && doDrawSliderFrameBufferToScreen) || vao == NULL) return;

	checkUpdateVars(osu, hitcircleDiameter);

	if (osu_slider_debug_draw_square_vao.getBool())
	{
		const Color dimmedColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedColor)*colorRGBMultiplier));

		g->setColor(dimmedColor);
		g->setAlpha(alpha*osu_slider_alpha_multiplier.getFloat());
		osu->getSkin()->getHitCircle()->bind();

		vao->setDrawPercent(from, to, 6); // HACKHACK: hardcoded magic number
		{
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(translation.x, translation.y);

				g->drawVAO(vao);
			}
			g->popTransform();
		}

		return; // nothing more to draw here
	}

	// NOTE: this would add support for aspire slider distortions, but calculating the bounding box live is a waste of performance, not worth it
	/*
	{
		m_fBoundingBoxMinX = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxX = std::numeric_limits<float>::min();
		m_fBoundingBoxMinY = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxY = std::numeric_limits<float>::min();

		// NOTE: to get the animated effect, would have to use from -> to (not implemented atm)
		for (int i=0; i<points.size(); i++)
		{
			const float &x = points[i].x;
			const float &y = points[i].y;
			const float radius = hitcircleDiameter/2.0f;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}

		const Vector4 tLS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMinX, m_fBoundingBoxMinY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;
		const Vector4 bRS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMaxX, m_fBoundingBoxMaxY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;

		float scaleToApplyAfterTranslationX = 1.0f;
		float scaleToApplyAfterTranslationY = 1.0f;

		const float sclX = (32768.0f / (float)osu->getScreenWidth());
		const float sclY = (32768.0f / (float)osu->getScreenHeight());

		if (-tLS.x + bRS.x > sclX)
			scaleToApplyAfterTranslationX = sclX / (-tLS.x + bRS.x);
		if (-tLS.y + bRS.y > sclY)
			scaleToApplyAfterTranslationY = sclY / (-tLS.y + bRS.y);
	}
	*/

	// reset
	resetRenderTargetBoundingBox();

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		if (doEnableRenderTarget)
			osu->getSliderFrameBuffer()->enable();

		// render
		{
			const Color undimmedBorderColor = osu_slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
			const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

			Color dimmedBorderColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBorderColor)*colorRGBMultiplier));
			Color dimmedBodyColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBodyColor)*colorRGBMultiplier));

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

				dimmedBorderColor = COLOR(255, red1, green1, blue1);
				dimmedBodyColor = COLOR(255, red2, green2, blue2);
			}

			if (!osu_slider_use_gradient_image.getBool())
			{
				BLEND_SHADER->enable();
				BLEND_SHADER->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
				BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
				BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
				BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
				BLEND_SHADER->setUniform1f("borderFeather", osu_slider_border_feather.getFloat());
				BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(dimmedBorderColor), COLOR_GET_Gf(dimmedBorderColor), COLOR_GET_Bf(dimmedBorderColor));
				BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(dimmedBodyColor), COLOR_GET_Gf(dimmedBodyColor), COLOR_GET_Bf(dimmedBodyColor));
			}

			g->setColor(COLORf(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // this only affects the gradient image if used (meaning shaders either don't work or are disabled on purpose)
			osu->getSkin()->getSliderGradient()->bind();
			{
				// draw curve mesh
				{
					vao->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
					g->pushTransform();
					{
						g->scale(scale, scale);
						g->translate(translation.x, translation.y);
						///g->scale(scaleToApplyAfterTranslationX, scaleToApplyAfterTranslationY); // aspire slider distortions

#ifdef MCENGINE_FEATURE_OPENGLES

						if (!osu_slider_use_gradient_image.getBool())
						{
							OpenGLES2Interface *gles2 = dynamic_cast<OpenGLES2Interface*>(g);
							if (gles2 != NULL)
							{
								g->forceUpdateTransform();
								Matrix4 mvp = g->getMVP();
								BLEND_SHADER->setUniformMatrix4fv("mvp", mvp);
							}
						}

#endif

#ifdef MCENGINE_FEATURE_DIRECTX11

						if (!osu_slider_use_gradient_image.getBool())
						{
							DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(g);
							if (dx11 != NULL)
							{
								g->forceUpdateTransform();
								Matrix4 mvp = g->getMVP();
								BLEND_SHADER->setUniformMatrix4fv("mvp", mvp);
							}
						}

#endif

						g->drawVAO(vao);
					}
					g->popTransform();

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}

		if (doDisableRenderTarget)
			osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// optional bounds performance optimization to reduce rt blending overdraw
	if (bounds.x != 0.0f || bounds.y != 0.0f || bounds.z != 0.0f || bounds.w != 0.0f)
	{
		const float pixelFudge = 2.0f;
		m_fBoundingBoxMinX = std::max(0.0f, bounds.x - hitcircleDiameter/2.0f - pixelFudge);
		m_fBoundingBoxMaxX = std::min((float)osu->getScreenWidth(), bounds.z + hitcircleDiameter/2.0f + pixelFudge);
		m_fBoundingBoxMinY = std::max(0.0f, bounds.y - hitcircleDiameter/2.0f - pixelFudge);
		m_fBoundingBoxMaxY = std::min((float)osu->getScreenHeight(), bounds.w + hitcircleDiameter/2.0f + pixelFudge);
	}
	else
	{
		m_fBoundingBoxMinX = 0.0f;
		m_fBoundingBoxMaxX = (float)osu->getScreenWidth();
		m_fBoundingBoxMinY = 0.0f;
		m_fBoundingBoxMaxY = (float)osu->getScreenHeight();
	}

	// now draw the slider to the screen (with alpha blending enabled again)
	if (doDrawSliderFrameBufferToScreen)
	{
		osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
		osu->getSliderFrameBuffer()->drawRect(g, (int)m_fBoundingBoxMinX, (int)m_fBoundingBoxMinY, (int)(m_fBoundingBoxMaxX - m_fBoundingBoxMinX), (int)(m_fBoundingBoxMaxY - m_fBoundingBoxMinY));
	}
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

#if defined(MCENGINE_FEATURE_OPENGL)

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#endif

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	//g->setBlending(false);
	{
		const Color undimmedBorderColor = osu_slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
		const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

		Color dimmedBorderColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBorderColor)*colorRGBMultiplier));
		Color dimmedBodyColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBodyColor)*colorRGBMultiplier));

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

			dimmedBorderColor = COLOR(255, red1, green1, blue1);
			dimmedBodyColor = COLOR(255, red2, green2, blue2);
		}

		//if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER_VR->enable();
			BLEND_SHADER_VR->setUniformMatrix4fv("matrix", mvp);
			BLEND_SHADER_VR->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER_VR->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER_VR->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform3f("colBorder", COLOR_GET_Rf(dimmedBorderColor), COLOR_GET_Gf(dimmedBorderColor), COLOR_GET_Bf(dimmedBorderColor));
			BLEND_SHADER_VR->setUniform3f("colBody", COLOR_GET_Rf(dimmedBodyColor), COLOR_GET_Gf(dimmedBodyColor), COLOR_GET_Bf(dimmedBodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();
		{
#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

#endif

			// draw curve mesh
			{
				BLEND_SHADER_VR->setUniform1i("part", 1);
				drawFillSliderBodyPeppyVR2(g, vr, mvp, points, (osu_slider_legacy_use_baked_vao.getBool() ? UNIT_CIRCLE_VAO_BAKED : UNIT_CIRCLE_VAO), hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

				if (alwaysPoints.size() > 0)
					drawFillSliderBodyPeppyVR2(g, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());

				BLEND_SHADER_VR->setUniform1i("part", 0);
				drawFillSliderBodyPeppyVR(g, osu, vr, mvp, points, (osu_slider_legacy_use_baked_vao.getBool() ? UNIT_CIRCLE_VAO_BAKED : UNIT_CIRCLE_VAO), hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

				if (alwaysPoints.size() > 0)
					drawFillSliderBodyPeppyVR(g, osu, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
			}

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

#endif

			//if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER_VR->disable();
		}
	}
	//g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, VertexArrayObject *vao1, VertexArrayObject *vao2, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f || vao1 == NULL || vao2 == NULL) return;

#if defined(MCENGINE_FEATURE_OPENGL)

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

	//const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

	checkUpdateVars(osu, hitcircleDiameter);

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	//g->setBlending(false);
	{
		const Color undimmedBorderColor = osu_slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
		const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

		Color dimmedBorderColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBorderColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBorderColor)*colorRGBMultiplier));
		Color dimmedBodyColor = COLOR(255, (int)(COLOR_GET_Ri(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Gi(undimmedBodyColor)*colorRGBMultiplier), (int)(COLOR_GET_Bi(undimmedBodyColor)*colorRGBMultiplier));

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

			dimmedBorderColor = COLOR(255, red1, green1, blue1);
			dimmedBodyColor = COLOR(255, red2, green2, blue2);
		}

		//if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER_VR->enable();
			BLEND_SHADER_VR->setUniformMatrix4fv("matrix", mvp);
			BLEND_SHADER_VR->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER_VR->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER_VR->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform3f("colBorder", COLOR_GET_Rf(dimmedBorderColor), COLOR_GET_Gf(dimmedBorderColor), COLOR_GET_Bf(dimmedBorderColor));
			BLEND_SHADER_VR->setUniform3f("colBody", COLOR_GET_Rf(dimmedBodyColor), COLOR_GET_Gf(dimmedBodyColor), COLOR_GET_Bf(dimmedBodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();
		{

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

#endif

			// draw curve mesh
			{
				BLEND_SHADER_VR->setUniform1i("part", 1);
				vao2->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
				g->drawVAO(vao2);

				if (alwaysPoints.size() > 0)
					drawFillSliderBodyPeppyVR2(g, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());

				BLEND_SHADER_VR->setUniform1i("part", 0);
				vao1->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
				g->drawVAO(vao1);

				if (alwaysPoints.size() > 0)
					drawFillSliderBodyPeppyVR(g, osu, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
			}

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

#endif

			//if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER_VR->disable();
		}
	}
	//g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawFillSliderBodyPeppy(Graphics *g, Osu *osu, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex, Shader *shader)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

#ifdef MCENGINE_FEATURE_OPENGLES

	OpenGLES2Interface *gles2 = dynamic_cast<OpenGLES2Interface*>(g);

#endif

#ifdef MCENGINE_FEATURE_DIRECTX11

	DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(g);

#endif

	g->pushTransform();
	{
		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			// fuck oob sliders
			if (x < -radius*2 || x > osu->getScreenWidth()+radius*2 || y < -radius*2 || y > osu->getScreenHeight()+radius*2)
				continue;

			g->translate(x-startX, y-startY, 0);

#ifdef MCENGINE_FEATURE_OPENGLES

			if (shader != NULL && gles2 != NULL)
			{
				g->forceUpdateTransform();
				Matrix4 mvp = g->getMVP();
				shader->setUniformMatrix4fv("mvp", mvp);
			}

#endif

#ifdef MCENGINE_FEATURE_DIRECTX11

			if (shader != NULL && dx11 != NULL)
			{
				g->forceUpdateTransform();
				Matrix4 mvp = g->getMVP();
				shader->setUniformMatrix4fv("mvp", mvp);
			}

#endif

			g->drawVAO(circleMesh);

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
	}
	g->popTransform();
}

void OsuSliderRenderer::drawFillSliderBodyPeppyVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	g->pushTransform();
	{
		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			g->translate(x-startX, y-startY, 0);
			g->drawVAO(circleMesh);

			startX = x;
			startY = y;
		}
	}
	g->popTransform();
}

void OsuSliderRenderer::drawFillSliderBodyPeppyVR2(Graphics *g, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	// HACKHACK: magic numbers from shader
	const float defaultBorderSize = 0.11;
	const float defaultTransitionSize = 0.011f;
	const float defaultOuterShadowSize = 0.08f;
	const float scale = 1.0f - osu_slider_border_size_multiplier.getFloat()*defaultBorderSize - defaultTransitionSize - defaultOuterShadowSize;

	// now, translate and draw the master vao for every curve point
	for (int i=drawFromIndex; i<drawUpToIndex; ++i)
	{
		const float x = points[i].x;
		const float y = points[i].y;

		g->pushTransform();
		{
			g->scale(scale, scale, 1.0f);
			g->translate(x, y, 0.25f);
			g->drawVAO(circleMesh);
		}
		g->popTransform();
	}
}

void OsuSliderRenderer::checkUpdateVars(Osu *osu, float hitcircleDiameter)
{
	// static globals

#ifdef MCENGINE_FEATURE_DIRECTX11

	{
		DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(engine->getGraphics());
		if (dx11 != NULL)
		{
			// NOTE: compensate for zflip
			if (MESH_CENTER_HEIGHT > 0.0f)
				MESH_CENTER_HEIGHT = -MESH_CENTER_HEIGHT;
		}
	}

#endif

	// build shaders and circle mesh
	if (BLEND_SHADER == NULL) // only do this once
	{
		// build shaders
		BLEND_SHADER = engine->getResourceManager()->loadShader2("slider.mcshader", "slider");
		if (osu != NULL && osu->isInVRMode())
			BLEND_SHADER_VR = engine->getResourceManager()->loadShader("sliderVR.vsh", "sliderVR.fsh", "sliderVR");
	}

	const int subdivisions = osu_slider_body_unit_circle_subdivisions.getInt();
	if (subdivisions != UNIT_CIRCLE_SUBDIVISIONS)
	{
		UNIT_CIRCLE_SUBDIVISIONS = subdivisions;

		// build unit cone
		{
			UNIT_CIRCLE.clear();

			// tip of the cone
			// texture coordinates
			UNIT_CIRCLE.push_back(1.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// position
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(MESH_CENTER_HEIGHT);

			for (int j=0; j<subdivisions; ++j)
			{
				float phase = j * (float) PI * 2.0f / subdivisions;

				// texture coordinates
				UNIT_CIRCLE.push_back(0.0f);
				UNIT_CIRCLE.push_back(0.0f);

				// positon
				UNIT_CIRCLE.push_back((float)std::sin(phase));
				UNIT_CIRCLE.push_back((float)std::cos(phase));
				UNIT_CIRCLE.push_back(0.0f);
			}

			// texture coordinates
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// positon
			UNIT_CIRCLE.push_back((float)std::sin(0.0f));
			UNIT_CIRCLE.push_back((float)std::cos(0.0f));
			UNIT_CIRCLE.push_back(0.0f);
		}
	}

	// build vaos
	if (UNIT_CIRCLE_VAO == NULL)
		UNIT_CIRCLE_VAO = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_BAKED == NULL)
		UNIT_CIRCLE_VAO_BAKED = engine->getResourceManager()->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_TRIANGLES == NULL)
		UNIT_CIRCLE_VAO_TRIANGLES = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);

	// (re-)generate master circle mesh (centered) if the size changed
	// dynamic mods like minimize or wobble have to use the legacy renderer anyway, since the slider shape may change every frame
	if (hitcircleDiameter != UNIT_CIRCLE_VAO_DIAMETER)
	{
		const float radius = hitcircleDiameter/2.0f;

		UNIT_CIRCLE_VAO_BAKED->release();

		// triangle fan
		UNIT_CIRCLE_VAO_DIAMETER = hitcircleDiameter;
		UNIT_CIRCLE_VAO->clear();
		for (int i=0; i<UNIT_CIRCLE.size()/5; i++)
		{
			Vector3 vertexPos = Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]);
			Vector2 vertexTexcoord = Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]);

			UNIT_CIRCLE_VAO->addVertex(vertexPos);
			UNIT_CIRCLE_VAO->addTexcoord(vertexTexcoord);

			UNIT_CIRCLE_VAO_BAKED->addVertex(vertexPos);
			UNIT_CIRCLE_VAO_BAKED->addTexcoord(vertexTexcoord);
		}

		engine->getResourceManager()->loadResource(UNIT_CIRCLE_VAO_BAKED);

		// pure triangles (needed for VertexArrayObject, because we can't merge multiple triangle fan meshes into one VertexArrayObject)
		UNIT_CIRCLE_VAO_TRIANGLES->clear();
		Vector3 startVertex = Vector3((radius * UNIT_CIRCLE[0 * 5 + 2]), (radius * UNIT_CIRCLE[0 * 5 + 3]), UNIT_CIRCLE[0 * 5 + 4]);
		Vector2 startUV = Vector2(UNIT_CIRCLE[0 * 5 + 0], UNIT_CIRCLE[0 * 5 + 1]);
		for (int i=1; i<UNIT_CIRCLE.size()/5 - 1; i++)
		{
			// center
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(startVertex);
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(startUV);

			// pizza slice edge 1
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]));

			// pizza slice edge 2
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[(i+1) * 5 + 2]), (radius * UNIT_CIRCLE[(i+1) * 5 + 3]), UNIT_CIRCLE[(i+1) * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[(i+1) * 5 + 0], UNIT_CIRCLE[(i+1) * 5 + 1]));
		}
	}
}

void OsuSliderRenderer::resetRenderTargetBoundingBox()
{
	OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
	OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;
}
