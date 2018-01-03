//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple filled VR slider
//
// $NoKeywords: $osuvrsl
//===============================================================================//

#include "OsuVRUISlider.h"

#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "Shader.h"
#include "VertexArrayObject.h"

#include "OsuVR.h"

OsuVRUISlider::OsuVRUISlider(OsuVR *vr, float x, float y, float width, float height) : OsuVRUIElement(vr, x, y, width, height)
{
	m_bClickCheck = false;
	m_sliderChangeCallback = NULL;

	m_fMinValue = 0.0f;
	m_fMaxValue = 1.0f;
	m_fCurValue = 0.0f;
	m_fCurPercent = 0.0f;
}

void OsuVRUISlider::drawVR(Graphics *g, Matrix4 &mvp)
{
	OsuVRUIElement::drawVR(g, mvp);
	if (!m_bIsVisible) return;

	const float lineThickness = 0.0035f;

	Color color = (m_bIsCursorInside || m_bIsActive) ? 0xffffffff : 0xff666666;

	// border line mesh
	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

	vao.addVertex(0, 0, 0);
	vao.addVertex(lineThickness, 0, 0);
	vao.addVertex(lineThickness, -m_vSize.y, 0);
	vao.addVertex(0, -m_vSize.y, 0);

	vao.addVertex(m_vSize.x, 0, 0);
	vao.addVertex(m_vSize.x - lineThickness, 0, 0);
	vao.addVertex(m_vSize.x - lineThickness, -m_vSize.y, 0);
	vao.addVertex(m_vSize.x, -m_vSize.y, 0);

	vao.addVertex(0, 0, 0);
	vao.addVertex(m_vSize.x, 0, 0);
	vao.addVertex(m_vSize.x, -lineThickness, 0);
	vao.addVertex(0, -lineThickness, 0);

	vao.addVertex(0, -m_vSize.y, 0);
	vao.addVertex(m_vSize.x, -m_vSize.y, 0);
	vao.addVertex(m_vSize.x, -m_vSize.y + lineThickness, 0);
	vao.addVertex(0, -m_vSize.y + lineThickness, 0);

	// percentage fill mesh
	VertexArrayObject vao2(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

	vao2.addVertex(0, 0, 0);
	vao2.addVertex(m_vSize.x*m_fCurPercent, 0, 0);
	vao2.addVertex(m_vSize.x*m_fCurPercent, -m_vSize.y, 0);
	vao2.addVertex(0, -m_vSize.y, 0);

	// position
	Matrix4 translation;
	translation.translate(m_vPos.x, m_vPos.y, 0);
	Matrix4 finalMVP = mvp * translation;

	// draw border line and fill
	m_vr->getShaderUntexturedGeneric()->enable();
	m_vr->getShaderUntexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
	{
		g->setColor(color);
		g->drawVAO(&vao);
		g->drawVAO(&vao2);
	}
	m_vr->getShaderUntexturedGeneric()->disable();
}

void OsuVRUISlider::update(Vector2 cursorPos)
{
	OsuVRUIElement::update(cursorPos);
	if (!m_bIsVisible) return;

	OpenVRController *controller = openvr->getController();

	if (controller->getTrigger() > 0.95f || controller->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD))
	{
		// trigger once if clicked
		if (!m_bClickCheck)
		{
			m_bClickCheck = true;

			// within bounds
			if (m_bIsCursorInside)
				m_bIsActive = true;
		}
	}
	else
	{
		m_bClickCheck = false;
		m_bIsActive = false;
	}

	// handle sliding
	if (m_bIsActive)
	{
		const float percent = clamp<float>((cursorPos.x - m_vPos.x)/m_vSize.x, 0.0f, 1.0f);
		bool hasChanged = percent != m_fCurPercent;
		m_fCurPercent = percent;
		m_fCurValue = lerp<float>(m_fMinValue, m_fMaxValue, m_fCurPercent);

		if (m_sliderChangeCallback != NULL && hasChanged)
			m_sliderChangeCallback(this);
	}
}

void OsuVRUISlider::setValue(float value, bool ignoreCallback)
{
	if (value == m_fCurValue || m_bIsActive) return;

	float newValue = clamp<float>(value, m_fMinValue, m_fMaxValue);
	if (newValue == m_fCurValue) return;

	m_fCurValue = newValue;
	m_fCurPercent = clamp<float>((m_fCurValue-m_fMinValue) / (std::abs(m_fMaxValue-m_fMinValue)), 0.0f, 1.0f);

	if (m_sliderChangeCallback != NULL && !ignoreCallback)
		m_sliderChangeCallback(this);
}
