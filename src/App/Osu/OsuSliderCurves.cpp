//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		curve classes have been taken from opsu!, albeit heavily modified.
//				contains all classes and functions for calculating slider curves
//
// $NoKeywords: $slidercurves
//=====================================================================================================//

#include "OsuSliderCurves.h"

#include "Engine.h"
#include "ConVar.h"

ConVar osu_slider_curve_points_separation("osu_slider_curve_points_separation", 2.5f, FCVAR_NONE, "slider body curve approximation step width in osu!pixels, don't set this lower than around 1.5");
ConVar osu_slider_curve_max_points("osu_slider_curve_max_points", 9999.0f, FCVAR_NONE, "maximum number of allowed interpolated curve points. quality will be forced to go down if a slider has more steps than this");
ConVar osu_slider_curve_max_length("osu_slider_curve_max_length", 65536/2, FCVAR_NONE, "maximum slider length in osu!pixels (i.e. pixelLength). also used to clamp all (control-)point coordinates to sane values.");



//**********************//
//	 Curve Base Class	//
//**********************//

OsuSliderCurve *OsuSliderCurve::createCurve(char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength)
{
	return createCurve(osuSliderCurveType, controlPoints, pixelLength, osu_slider_curve_points_separation.getFloat());
}

OsuSliderCurve *OsuSliderCurve::createCurve(char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation)
{
	OsuSliderCurve *curve = NULL;

	// build curve
	if (osuSliderCurveType == OSUSLIDERCURVETYPE_PASSTHROUGH && controlPoints.size() == 3)
	{
		Vector2 nora = controlPoints[1] - controlPoints[0];
		Vector2 norb = controlPoints[1] - controlPoints[2];

		float temp = nora.x;
		nora.x = -nora.y;
		nora.y = temp;
		temp = norb.x;
		norb.x = -norb.y;
		norb.y = temp;

		// TODO: to properly support all aspire sliders (e.g. Ping), need to use osu circular arc calc + subdivide line segments if they are too big

		if (std::abs(norb.x * nora.y - norb.y * nora.x) < 0.00001f)
			curve = new OsuSliderCurveLinearBezier(controlPoints, pixelLength, false, curvePointsSeparation); // vectors parallel, use linear bezier instead
		else
			curve = new OsuSliderCurveCircumscribedCircle(controlPoints, pixelLength, curvePointsSeparation);
	}
	else if (osuSliderCurveType == OSUSLIDERCURVETYPE_CATMULL)
		curve = new OsuSliderCurveCatmull(controlPoints, pixelLength, curvePointsSeparation);
	else
		curve = new OsuSliderCurveLinearBezier(controlPoints, pixelLength, (osuSliderCurveType == OSUSLIDERCURVETYPE_LINEAR), curvePointsSeparation);

	// calculate bounds
	for (const Vector2 &point : curve->getPoints())
	{
		if (point.x < curve->m_originalBounds.x)
			curve->m_originalBounds.x = point.x;
		if (point.x > curve->m_originalBounds.z)
			curve->m_originalBounds.z = point.x;
		if (point.y < curve->m_originalBounds.y)
			curve->m_originalBounds.y = point.y;
		if (point.y > curve->m_originalBounds.w)
			curve->m_originalBounds.w = point.y;
	}
	curve->m_bounds = curve->m_originalBounds;

	return curve;
}

OsuSliderCurve::OsuSliderCurve(std::vector<Vector2> controlPoints, float pixelLength)
{
	m_controlPoints = controlPoints;
	m_fPixelLength = pixelLength;

	m_fStartAngle = 0.0f;
	m_fEndAngle = 0.0f;

	m_originalBounds.x = std::numeric_limits<float>::max();	// minX
	m_originalBounds.y = std::numeric_limits<float>::max();	// minY
	m_originalBounds.z = 0.0f;								// maxX
	m_originalBounds.w = 0.0f;								// maxY
	m_bounds = m_originalBounds;
}

void OsuSliderCurve::updateStackPosition(float stackMulStackOffset, bool HR)
{
	for (int i=0; i<m_originalCurvePoints.size() && i<m_curvePoints.size(); i++)
	{
		m_curvePoints[i] = m_originalCurvePoints[i] - Vector2(stackMulStackOffset, stackMulStackOffset * (HR ? -1.0f : 1.0f));
	}

	for (int s=0; s<m_originalCurvePointSegments.size() && s<m_curvePointSegments.size(); s++)
	{
		for (int p=0; p<m_originalCurvePointSegments[s].size() && p<m_curvePointSegments[s].size(); p++)
		{
			m_curvePointSegments[s][p] = m_originalCurvePointSegments[s][p] - Vector2(stackMulStackOffset, stackMulStackOffset * (HR ? -1.0f : 1.0f));
		}
	}

	m_bounds.x = m_originalBounds.x - stackMulStackOffset;
	m_bounds.y = m_originalBounds.y - (stackMulStackOffset * (HR ? -1.0f : 1.0f));
	m_bounds.z = m_originalBounds.z - stackMulStackOffset;
	m_bounds.w = m_originalBounds.w - (stackMulStackOffset * (HR ? -1.0f : 1.0f));
}



//******************************************//
//	 Curve Type Base Class & Curve Types	//
//******************************************//

OsuSliderCurveType::OsuSliderCurveType()
{
	m_fTotalDistance = 0.0f;
}

void OsuSliderCurveType::init(float approxLength)
{
	calculateIntermediaryPoints(approxLength);
	calculateCurveDistances();
}

void OsuSliderCurveType::initCustom(std::vector<Vector2> points)
{
	m_points = std::move(points);
	calculateCurveDistances();
}

void OsuSliderCurveType::calculateIntermediaryPoints(float approxLength)
{
	// subdivide the curve, calculate all intermediary points
	const int numPoints = (int)(approxLength / 4.0f) + 2;
	m_points.reserve(numPoints);
	for (int i=0; i<numPoints; i++)
	{
		m_points.push_back(pointAt((float)i / (float)(numPoints - 1)));
	}
}

void OsuSliderCurveType::calculateCurveDistances()
{
	// reset
	m_fTotalDistance = 0.0f;
	m_curveDistances.clear();

	// find the distance of each point from the previous point (needed for some curve types)
	m_curveDistances.reserve(m_points.size());
	for (int i=0; i<m_points.size(); i++)
	{
		const float curDist = (i == 0) ? 0 : (m_points[i] - m_points[i-1]).length();

		m_curveDistances.push_back(curDist);
		m_fTotalDistance += curDist;
	}
}



OsuSliderCurveTypeBezier2::OsuSliderCurveTypeBezier2(const std::vector<Vector2> &points) : OsuSliderCurveType()
{
	initCustom(OsuSliderBezierApproximator().createBezier(points));
}



OsuSliderCurveTypeCentripetalCatmullRom::OsuSliderCurveTypeCentripetalCatmullRom(const std::vector<Vector2> &points) : OsuSliderCurveType()
{
	if (points.size() != 4)
	{
		debugLog("OsuSliderCurveTypeCentripetalCatmullRom() Error: points.size() != 4!!!\n");
		return;
	}

	m_points = std::vector<Vector2>(points); // copy
	m_time[0] = 0.0f;

	float approxLength = 0;
	for (int i=1; i<4; i++)
	{
		float len = 0;

		if (i > 0)
			len = (points[i] - points[i - 1]).length();

		if (len <= 0)
			len += 0.0001f;

		approxLength += len;

		// m_time[i] = (float) std::sqrt(len) + time[i-1]; // ^(0.5)
		m_time[i] = i;
	}

	init(approxLength / 2.0f);
}

Vector2 OsuSliderCurveTypeCentripetalCatmullRom::pointAt(float t)
{
	t = t * (m_time[2] - m_time[1]) + m_time[1];

	const Vector2 A1 = m_points[0]*((m_time[1] - t) / (m_time[1] - m_time[0]))
		  +(m_points[1]*((t - m_time[0]) / (m_time[1] - m_time[0])));
	const Vector2 A2 = m_points[1]*((m_time[2] - t) / (m_time[2] - m_time[1]))
		  +(m_points[2]*((t - m_time[1]) / (m_time[2] - m_time[1])));
	const Vector2 A3 = m_points[2]*((m_time[3] - t) / (m_time[3] - m_time[2]))
		  +(m_points[3]*((t - m_time[2]) / (m_time[3] - m_time[2])));

	const Vector2 B1 = A1*((m_time[2] - t) / (m_time[2] - m_time[0]))
		  +(A2*((t - m_time[0]) / (m_time[2] - m_time[0])));
	const Vector2 B2 = A2*((m_time[3] - t) / (m_time[3] - m_time[1]))
		  +(A3*((t - m_time[1]) / (m_time[3] - m_time[1])));

	const Vector2 C = B1*((m_time[2] - t) / (m_time[2] - m_time[1]))
		 +(B2*((t - m_time[1]) / (m_time[2] - m_time[1])));

	return C;
}



//*******************//
//	 Curve Classes	 //
//*******************//

OsuSliderCurveEqualDistanceMulti::OsuSliderCurveEqualDistanceMulti(std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation) : OsuSliderCurve(controlPoints, pixelLength)
{
	m_iNCurve = std::min((int)(m_fPixelLength / clamp<float>(curvePointsSeparation, 1.0f, 100.0f)), osu_slider_curve_max_points.getInt());
}

void OsuSliderCurveEqualDistanceMulti::init(const std::vector<OsuSliderCurveType*> &curvesList)
{
	if (curvesList.size() < 1)
	{
		debugLog("OsuSliderCurveEqualDistanceMulti::init() Error: curvesList.size() == 0!!!\n");
		return;
	}

	int curCurveIndex = 0;
	int curPoint = 0;

	float distanceAt = 0.0f;
	float lastDistanceAt = 0.0f;

	OsuSliderCurveType *curCurve = curvesList[curCurveIndex];
	{
		if (curCurve->getCurvePoints().size() < 1)
		{
			debugLog("OsuSliderCurveEqualDistanceMulti::init() Error: curCurve->getCurvePoints().size() == 0!!!\n");

			// cleanup
			for (size_t i=0; i<curvesList.size(); i++)
			{
				delete curvesList[i];
			}

			return;
		}
	}
	Vector2 lastCurve = curCurve->getCurvePoints()[curPoint];

	// length of the curve should be equal to the pixel length
	// for each distance, try to get in between the two points that are between it
	Vector2 lastCurvePointForNextSegmentStart;
	std::vector<Vector2> curCurvePoints;
	for (int i=0; i<(m_iNCurve + 1); i++)
	{
		const int prefDistance = (int)(((float)i * m_fPixelLength) / (float)m_iNCurve);

		while (distanceAt < prefDistance)
		{
			lastDistanceAt = distanceAt;
			if (curCurve->getCurvePoints().size() > 0 && curPoint > -1 && curPoint < curCurve->getCurvePoints().size())
				lastCurve = curCurve->getCurvePoints()[curPoint];

			// jump to next point
			curPoint++;

			if (curPoint >= curCurve->getNumPoints())
			{
				// jump to next curve
				curCurveIndex++;

				// when jumping to the next curve, add the current segment to the list of segments
				if (curCurvePoints.size() > 0)
				{
					m_curvePointSegments.push_back(curCurvePoints);
					curCurvePoints.clear();

					// prepare the next segment by setting/adding the starting point to the exact end point of the previous segment
					// this also enables an optimization, namely that startcaps only have to be drawn [for every segment] if the startpoint != endpoint in the loop
					if (m_curvePoints.size() > 0)
						curCurvePoints.push_back(lastCurvePointForNextSegmentStart);
				}

				if (curCurveIndex < curvesList.size())
				{
					curCurve = curvesList[curCurveIndex];
					curPoint = 0;
				}
				else
				{
					curPoint = curCurve->getNumPoints() - 1;
					if (lastDistanceAt == distanceAt)
					{
						// out of points even though the preferred distance hasn't been reached
						break;
					}
				}
			}

			if (curCurve->getCurveDistances().size() > 0 && curPoint > -1 && curPoint < curCurve->getCurveDistances().size())
				distanceAt += curCurve->getCurveDistances()[curPoint];
		}

		const Vector2 thisCurve = (curCurve->getCurvePoints().size() > 0 && curPoint > -1 && curPoint < curCurve->getCurvePoints().size() ? curCurve->getCurvePoints()[curPoint] : Vector2(0, 0));

		// interpolate the point between the two closest distances
		m_curvePoints.push_back(Vector2(0, 0));
		curCurvePoints.push_back(Vector2(0, 0));
		if (distanceAt - lastDistanceAt > 1)
		{
			const float t = (prefDistance - lastDistanceAt) / (distanceAt - lastDistanceAt);
			m_curvePoints[i] = Vector2(lerp<float>(lastCurve.x, thisCurve.x, t), lerp<float>(lastCurve.y, thisCurve.y, t));
		}
		else
			m_curvePoints[i] = thisCurve;

		// add the point to the current segment (this is not using the lerp'd point! would cause mm mesh artifacts if it did)
		lastCurvePointForNextSegmentStart = thisCurve;
		curCurvePoints[curCurvePoints.size() - 1] = thisCurve;
	}

	// if we only had one segment, no jump to any next curve has occurred (and therefore no insertion of the segment into the vector)
	// manually add the lone segment here
	if (curCurvePoints.size() > 0)
		m_curvePointSegments.push_back(curCurvePoints);

	// sanity check
	if (m_curvePoints.size() == 0)
	{
		debugLog("OsuSliderCurveEqualDistanceMulti::init() Error: m_curvePoints.size() == 0!!!\n");

		// cleanup
		for (size_t i=0; i<curvesList.size(); i++)
		{
			delete curvesList[i];
		}

		return;
	}

	// make sure that the uninterpolated segment points are exactly as long as the pixelLength
	// this is necessary because we can't use the lerp'd points for the segments
	float segmentedLength = 0.0f;
	for (int s=0; s<m_curvePointSegments.size(); s++)
	{
		for (int p=0; p<m_curvePointSegments[s].size(); p++)
		{
			segmentedLength += ((p == 0) ? 0 : (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).length());
		}
	}

	// TODO: this is still incorrect. sliders are sometimes too long or start too late, even if only for a few pixels
	if (segmentedLength > m_fPixelLength && m_curvePointSegments.size() > 1 && m_curvePointSegments[0].size() > 1)
	{
		float excess = segmentedLength - m_fPixelLength;
		while (excess > 0)
		{
			for (int s=m_curvePointSegments.size()-1; s>=0; s--)
			{
				for (int p=m_curvePointSegments[s].size()-1; p>=0; p--)
				{
					const float curLength = (p == 0) ? 0 : (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).length();
					if (curLength >= excess && p != 0)
					{
						Vector2 segmentVector = (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).normalize();
						m_curvePointSegments[s][p] = m_curvePointSegments[s][p] - segmentVector*excess;
						excess = 0.0f;
						break;
					}
					else
					{
						m_curvePointSegments[s].erase(m_curvePointSegments[s].begin() + p);
						excess -= curLength;
					}
				}
			}
		}
	}

	// calculate start and end angles for possible repeats (good enough and cheaper than calculating it live every frame)
	if (m_curvePoints.size() > 1)
	{
		Vector2 c1 = m_curvePoints[0];
		int cnt = 1;
		Vector2 c2 = m_curvePoints[cnt++];
		while (cnt <= m_iNCurve && cnt < m_curvePoints.size() && (c2-c1).length() < 1)
		{
			c2 = m_curvePoints[cnt++];
		}
		m_fStartAngle = (float)(std::atan2(c2.y - c1.y, c2.x - c1.x) * 180 / PI);
	}

	if (m_curvePoints.size() > 1)
	{
		if (m_iNCurve < m_curvePoints.size())
		{
			Vector2 c1 = m_curvePoints[m_iNCurve];
			int cnt = m_iNCurve - 1;
			Vector2 c2 = m_curvePoints[cnt--];
			while (cnt >= 0 && (c2-c1).length() < 1)
			{
				c2 = m_curvePoints[cnt--];
			}
			m_fEndAngle = (float)(std::atan2(c2.y - c1.y, c2.x - c1.x) * 180 / PI);
		}
	}

	// backup (for dynamic updateStackPosition() recalculation)
	m_originalCurvePoints = std::vector<Vector2>(m_curvePoints); // copy
	m_originalCurvePointSegments = std::vector<std::vector<Vector2>>(m_curvePointSegments); // copy

	// cleanup
	for (size_t i=0; i<curvesList.size(); i++)
	{
		delete curvesList[i];
	}
}

Vector2 OsuSliderCurveEqualDistanceMulti::pointAt(float t)
{
	if (m_curvePoints.size() < 1) return Vector2(0, 0);

	const float indexF = t * m_iNCurve;
	const int index = (int)indexF;
	if (index >= m_iNCurve)
	{
		if (m_iNCurve > -1 && m_iNCurve < m_curvePoints.size())
			return m_curvePoints[m_iNCurve];
		else
		{
			debugLog("OsuSliderCurveEqualDistanceMulti::pointAt() Error: Illegal index %i!!!\n", m_iNCurve);
			return Vector2(0, 0);
		}
	}
	else
	{
		if (index < 0 || index + 1 >= m_curvePoints.size())
		{
			debugLog("OsuSliderCurveEqualDistanceMulti::pointAt() Error: Illegal index %i!!!\n", index);
			return Vector2(0, 0);
		}

		const Vector2 poi = m_curvePoints[index];
		const Vector2 poi2 = m_curvePoints[index + 1];

		const float t2 = indexF - index;

		return Vector2(lerp(poi.x, poi2.x, t2), lerp(poi.y, poi2.y, t2));
	}
}

Vector2 OsuSliderCurveEqualDistanceMulti::originalPointAt(float t)
{
	if (m_originalCurvePoints.size() < 1) return Vector2(0, 0);

	const float indexF = t * m_iNCurve;
	const int index = (int)indexF;
	if (index >= m_iNCurve)
	{
		if (m_iNCurve > -1 && m_iNCurve < m_originalCurvePoints.size())
			return m_originalCurvePoints[m_iNCurve];
		else
		{
			debugLog("OsuSliderCurveEqualDistanceMulti::originalPointAt() Error: Illegal index %i!!!\n", m_iNCurve);
			return Vector2(0, 0);
		}
	}
	else
	{
		if (index < 0 || index + 1 >= m_originalCurvePoints.size())
		{
			debugLog("OsuSliderCurveEqualDistanceMulti::originalPointAt() Error: Illegal index %i!!!\n", index);
			return Vector2(0, 0);
		}

		const Vector2 poi = m_originalCurvePoints[index];
		const Vector2 poi2 = m_originalCurvePoints[index + 1];

		const float t2 = indexF - index;

		return Vector2(lerp(poi.x, poi2.x, t2), lerp(poi.y, poi2.y, t2));
	}
}



OsuSliderCurveLinearBezier::OsuSliderCurveLinearBezier(std::vector<Vector2> controlPoints, float pixelLength, bool line, float curvePointsSeparation) : OsuSliderCurveEqualDistanceMulti(controlPoints, pixelLength, curvePointsSeparation)
{
	const int numControlPoints = m_controlPoints.size();

	std::vector<OsuSliderCurveType*> beziers;

	std::vector<Vector2> points; // temporary list of points to separate different bezier curves

	// Beziers: splits points into different Beziers if has the same points (red points)
	// a b c - c d - d e f g
	// Lines: generate a new curve for each sequential pair
	// ab  bc  cd  de  ef  fg
	Vector2 lastPoint;
	for (int i=0; i<numControlPoints; i++)
	{
		const Vector2 currentPoint = m_controlPoints[i];

		if (line)
		{
			if (i > 0)
			{
				points.push_back(currentPoint);

				beziers.push_back(new OsuSliderCurveTypeBezier2(points));

				points.clear();
			}
		}
		else if (i > 0)
		{
			if (currentPoint == lastPoint)
			{
				if (points.size() >= 2)
				{
					beziers.push_back(new OsuSliderCurveTypeBezier2(points));
				}

				points.clear();
			}
		}

		points.push_back(currentPoint);
		lastPoint = currentPoint;
	}

	if (line || points.size() < 2)
	{
		// trying to continue Bezier with less than 2 points
		// probably ending on a red point, just ignore it
	}
	else
	{
		beziers.push_back(new OsuSliderCurveTypeBezier2(points));

		points.clear();
	}

	// build curve
	OsuSliderCurveEqualDistanceMulti::init(beziers);
}



OsuSliderCurveCatmull::OsuSliderCurveCatmull(std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation) : OsuSliderCurveEqualDistanceMulti(controlPoints, pixelLength, curvePointsSeparation)
{
	const int numControlPoints = m_controlPoints.size();

	std::vector<OsuSliderCurveType*> catmulls;

	std::vector<Vector2> points; // temporary list of points to separate different curves

	// repeat the first and last points as controls points
	// only if the first/last two points are different
	// aabb
	// aabc abcc
	// aabc abcd bcdd
	if (m_controlPoints[0].x != m_controlPoints[1].x || m_controlPoints[0].y != m_controlPoints[1].y)
		points.push_back(m_controlPoints[0]);

	for (int i=0; i<numControlPoints; i++)
	{
		points.push_back(m_controlPoints[i]);
		if (points.size() >= 4)
		{
			catmulls.push_back(new OsuSliderCurveTypeCentripetalCatmullRom(points));

			points.erase(points.begin());
		}
	}

	if (m_controlPoints[numControlPoints - 1].x != m_controlPoints[numControlPoints - 2].x || m_controlPoints[numControlPoints - 1].y != m_controlPoints[numControlPoints - 2].y)
		points.push_back(m_controlPoints[numControlPoints - 1]);

	if (points.size() >= 4)
	{
		catmulls.push_back(new OsuSliderCurveTypeCentripetalCatmullRom(points));
	}

	// build curve
	OsuSliderCurveEqualDistanceMulti::init(catmulls);
}



OsuSliderCurveCircumscribedCircle::OsuSliderCurveCircumscribedCircle(std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation) : OsuSliderCurve(controlPoints, pixelLength)
{
	if (controlPoints.size() != 3)
	{
		debugLog("OsuSliderCurveCircumscribedCircle() Error: controlPoints.size() != 3\n");
		return;
	}

	// construct the three points
	const Vector2 start = m_controlPoints[0];
	const Vector2 mid = m_controlPoints[1];
	const Vector2 end = m_controlPoints[2];

	// find the circle center
	const Vector2 mida = start + (mid-start)*0.5f;
	const Vector2 midb = end + (mid-end)*0.5f;

	Vector2 nora = mid - start;
	float temp = nora.x;
	nora.x = -nora.y;
	nora.y = temp;
	Vector2 norb = mid - end;
	temp = norb.x;
	norb.x = -norb.y;
	norb.y = temp;

	m_vOriginalCircleCenter = intersect(mida, nora, midb, norb);
	m_vCircleCenter = m_vOriginalCircleCenter;

	// find the angles relative to the circle center
	Vector2 startAngPoint = start - m_vCircleCenter;
	Vector2 midAngPoint   = mid - m_vCircleCenter;
	Vector2 endAngPoint   = end - m_vCircleCenter;

	m_fCalculationStartAngle = (float)std::atan2(startAngPoint.y, startAngPoint.x);
	const float midAng		 = (float)std::atan2(midAngPoint.y, midAngPoint.x);
	m_fCalculationEndAngle	 = (float)std::atan2(endAngPoint.y, endAngPoint.x);

	// find the angles that pass through midAng
	if (!isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle))
	{
		if (std::abs(m_fCalculationStartAngle + 2*PI - m_fCalculationEndAngle) < 2*PI && isIn(m_fCalculationStartAngle + (2*PI), midAng, m_fCalculationEndAngle))
			m_fCalculationStartAngle += 2*PI;
		else if (std::abs(m_fCalculationStartAngle - (m_fCalculationEndAngle + 2*PI)) < 2*PI && isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle + (2*PI)))
			m_fCalculationEndAngle += 2*PI;
		else if (std::abs(m_fCalculationStartAngle - 2*PI - m_fCalculationEndAngle) < 2*PI && isIn(m_fCalculationStartAngle - (2*PI), midAng, m_fCalculationEndAngle))
			m_fCalculationStartAngle -= 2*PI;
		else if (std::abs(m_fCalculationStartAngle - (m_fCalculationEndAngle - 2*PI)) < 2*PI && isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle - (2*PI)))
			m_fCalculationEndAngle -= 2*PI;
		else
		{
			debugLog("OsuSliderCurveCircumscribedCircle() Error: Cannot find angles between midAng (%.3f %.3f %.3f)\n", m_fCalculationStartAngle, midAng, m_fCalculationEndAngle);
			return;
		}
	}

	// find an angle with an arc length of pixelLength along this circle
	m_fRadius = startAngPoint.length();
	const float arcAng = m_fPixelLength / m_fRadius;  // len = theta * r / theta = len / r

	// now use it for our new end angle
	m_fCalculationEndAngle = (m_fCalculationEndAngle > m_fCalculationStartAngle) ? m_fCalculationStartAngle + arcAng : m_fCalculationStartAngle - arcAng;

	// find the angles to draw for repeats
	m_fEndAngle   = (float)((m_fCalculationEndAngle   + (m_fCalculationStartAngle > m_fCalculationEndAngle ? PI/2.0f : -PI/2.0f)) * 180.0f / PI);
	m_fStartAngle = (float)((m_fCalculationStartAngle + (m_fCalculationStartAngle > m_fCalculationEndAngle ? -PI/2.0f : PI/2.0f)) * 180.0f / PI);

	// calculate points
	const float steps = std::min(m_fPixelLength / (clamp<float>(curvePointsSeparation, 1.0f, 100.0f)), osu_slider_curve_max_points.getFloat());
	const int intSteps = (int)std::round(steps) + 2; // must guarantee an int range of 0 to steps!
	for (int i=0; i<intSteps; i++)
	{
		float t = clamp<float>((float)i / steps, 0.0f, 1.0f);
		m_curvePoints.push_back(pointAt(t));

		if (t >= 1.0f) // early break if we've already reached the end
			break;
	}

	// add the segment (no special logic here for SliderCurveCircumscribedCircle, just add the entire vector)
	m_curvePointSegments.push_back(std::vector<Vector2>(m_curvePoints)); // copy

	// backup (for dynamic updateStackPosition() recalculation)
	m_originalCurvePoints = std::vector<Vector2>(m_curvePoints); // copy
	m_originalCurvePointSegments = std::vector<std::vector<Vector2>>(m_curvePointSegments); // copy
}

void OsuSliderCurveCircumscribedCircle::updateStackPosition(float stackMulStackOffset, bool HR)
{
	OsuSliderCurve::updateStackPosition(stackMulStackOffset, HR);

	m_vCircleCenter = m_vOriginalCircleCenter - Vector2(stackMulStackOffset, stackMulStackOffset * (HR ? -1.0f : 1.0f));
}

Vector2 OsuSliderCurveCircumscribedCircle::pointAt(float t)
{
	const float sanityRange = osu_slider_curve_max_length.getFloat(); // NOTE: added to fix some aspire problems (endless drawFollowPoints and star calc etc.)
	const float ang = lerp(m_fCalculationStartAngle, m_fCalculationEndAngle, t);

	return Vector2(clamp<float>(std::cos(ang) * m_fRadius + m_vCircleCenter.x, -sanityRange, sanityRange),
				   clamp<float>(std::sin(ang) * m_fRadius + m_vCircleCenter.y, -sanityRange, sanityRange));
}

Vector2 OsuSliderCurveCircumscribedCircle::originalPointAt(float t)
{
	const float sanityRange = osu_slider_curve_max_length.getFloat(); // NOTE: added to fix some aspire problems (endless drawFollowPoints and star calc etc.)
	const float ang = lerp(m_fCalculationStartAngle, m_fCalculationEndAngle, t);

	return Vector2(clamp<float>(std::cos(ang) * m_fRadius + m_vOriginalCircleCenter.x, -sanityRange, sanityRange),
				   clamp<float>(std::sin(ang) * m_fRadius + m_vOriginalCircleCenter.y, -sanityRange, sanityRange));
}

Vector2 OsuSliderCurveCircumscribedCircle::intersect(Vector2 a, Vector2 ta, Vector2 b, Vector2 tb)
{
	const float des = (tb.x * ta.y - tb.y * ta.x);
	if (std::abs(des) < 0.0001f)
	{
		debugLog("OsuSliderCurveCircumscribedCircle::intersect() Error: Vectors are parallel!!!\n");
		return Vector2(0, 0);
	}

	const float u = ((b.y - a.y) * ta.x + (a.x - b.x) * ta.y) / des;
	return (b + Vector2(tb.x * u, tb.y * u));
}



//********************//
//	 Helper Classes	  //
//********************//

// https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Objects/BezierApproximator.cs
// https://github.com/ppy/osu-framework/blob/master/osu.Framework/MathUtils/PathApproximator.cs
// Copyright (c) ppy Pty Ltd <contact@ppy.sh>. Licensed under the MIT Licence.

double OsuSliderBezierApproximator::TOLERANCE_SQ = 0.25 * 0.25;

OsuSliderBezierApproximator::OsuSliderBezierApproximator()
{
	m_iCount = 0;
}

std::vector<Vector2> OsuSliderBezierApproximator::createBezier(const std::vector<Vector2> &controlPoints)
{
	m_iCount = controlPoints.size();

	std::vector<Vector2> output;
	if (m_iCount == 0) return output;

	m_subdivisionBuffer1.resize(m_iCount);
	m_subdivisionBuffer2.resize(m_iCount*2 - 1);

	std::stack<std::vector<Vector2>> toFlatten;
	std::stack<std::vector<Vector2>> freeBuffers;

	toFlatten.push(std::vector<Vector2>(controlPoints)); // copy

	std::vector<Vector2> &leftChild = m_subdivisionBuffer2;

	while (toFlatten.size() > 0)
	{
		std::vector<Vector2> parent = std::move(toFlatten.top());
		toFlatten.pop();

		if (isFlatEnough(parent))
		{
			approximate(parent, output);
			freeBuffers.push(parent);
			continue;
		}

		std::vector<Vector2> rightChild;
		if (freeBuffers.size() > 0)
		{
			rightChild = std::move(freeBuffers.top());
			freeBuffers.pop();
		}
		else
			rightChild.resize(m_iCount);

        subdivide(parent, leftChild, rightChild);

        for (int i=0; i<m_iCount; ++i)
        {
            parent[i] = leftChild[i];
        }

        toFlatten.push(std::move(rightChild));
        toFlatten.push(std::move(parent));
	}

	output.push_back(controlPoints[m_iCount - 1]);
	return output;
}

bool OsuSliderBezierApproximator::isFlatEnough(const std::vector<Vector2> &controlPoints)
{
	if (controlPoints.size() < 1) return true;

    for (int i=1; i<(int)(controlPoints.size() - 1); i++)
    {
        if (std::pow((double)(controlPoints[i - 1] - 2 * controlPoints[i] + controlPoints[i + 1]).length(), 2.0) > TOLERANCE_SQ * 4)
            return false;
    }

    return true;
}

void OsuSliderBezierApproximator::subdivide(std::vector<Vector2> &controlPoints, std::vector<Vector2> &l, std::vector<Vector2> &r)
{
	std::vector<Vector2> &midpoints = m_subdivisionBuffer1;

    for (int i=0; i<m_iCount; ++i)
    {
        midpoints[i] = controlPoints[i];
    }

    for (int i=0; i<m_iCount; i++)
    {
        l[i] = midpoints[0];
        r[m_iCount - i - 1] = midpoints[m_iCount - i - 1];

        for (int j=0; j<m_iCount-i-1; j++)
        {
            midpoints[j] = (midpoints[j] + midpoints[j + 1]) / 2;
        }
    }
}

void OsuSliderBezierApproximator::approximate(std::vector<Vector2> &controlPoints, std::vector<Vector2> &output)
{
    std::vector<Vector2> &l = m_subdivisionBuffer2;
    std::vector<Vector2> &r = m_subdivisionBuffer1;

    subdivide(controlPoints, l, r);

    for (int i=0; i<m_iCount-1; ++i)
    {
        l[m_iCount + i] = r[i + 1];
    }

    output.push_back(controlPoints[0]);
    for (int i=1; i<m_iCount-1; ++i)
    {
        const int index = 2 * i;
        Vector2 p = 0.25f * (l[index - 1] + 2 * l[index] + l[index + 1]);
        output.push_back(p);
    }
}
