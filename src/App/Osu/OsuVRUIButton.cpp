//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple empty VR button
//
// $NoKeywords: $osuvrbt
//===============================================================================//

#include "OsuVRUIButton.h"

#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "Shader.h"
#include "VertexArrayObject.h"

#include "OsuVR.h"

OsuVRUIButton::OsuVRUIButton(OsuVR *vr, float x, float y, float width, float height) : OsuVRUIElement(vr, x, y, width, height)
{
	m_bClickCheck = false;
	m_clickVoidCallback = NULL;
}

void OsuVRUIButton::drawVR(Graphics *g, Matrix4 &mvp)
{
	OsuVRUIElement::drawVR(g, mvp);
	if (!m_bIsVisible) return;

	const float lineThickness = 0.0035f;

	// border line mesh
	VertexArrayObject vao2(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

	vao2.addVertex(0, 0, 0);
	vao2.addVertex(lineThickness, 0, 0);
	vao2.addVertex(lineThickness, -m_vSize.y, 0);
	vao2.addVertex(0, -m_vSize.y, 0);

	vao2.addVertex(m_vSize.x, 0, 0);
	vao2.addVertex(m_vSize.x - lineThickness, 0, 0);
	vao2.addVertex(m_vSize.x - lineThickness, -m_vSize.y, 0);
	vao2.addVertex(m_vSize.x, -m_vSize.y, 0);

	vao2.addVertex(0, 0, 0);
	vao2.addVertex(m_vSize.x, 0, 0);
	vao2.addVertex(m_vSize.x, -lineThickness, 0);
	vao2.addVertex(0, -lineThickness, 0);

	vao2.addVertex(0, -m_vSize.y, 0);
	vao2.addVertex(m_vSize.x, -m_vSize.y, 0);
	vao2.addVertex(m_vSize.x, -m_vSize.y + lineThickness, 0);
	vao2.addVertex(0, -m_vSize.y + lineThickness, 0);

	// position
	Matrix4 translation;
	translation.translate(m_vPos.x, m_vPos.y, 0);
	Matrix4 finalMVP = mvp * translation;

	// draw border line
	m_vr->getShaderUntexturedGeneric()->enable();
	m_vr->getShaderUntexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
	{
		g->setColor(m_bIsCursorInside ? 0xffffffff : 0xff666666);
		g->drawVAO(&vao2);
	}
	m_vr->getShaderUntexturedGeneric()->disable();
}

void OsuVRUIButton::update(Vector2 cursorPos)
{
	OsuVRUIElement::update(cursorPos);
	if (!m_bIsVisible) return;

	OpenVRController *controller = openvr->getController();

	if (controller->getTrigger() > 0.95f || controller->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD))
	{
		// trigger click callback once (until released again)
		if (!m_bClickCheck)
		{
			m_bClickCheck = true;

			// within bounds
			if (m_bIsCursorInside)
			{
				m_bIsActive = true;
				if (m_clickVoidCallback != NULL)
					m_clickVoidCallback();
			}
		}
	}
	else
	{
		m_bClickCheck = false;
		m_bIsActive = false;
	}
}
