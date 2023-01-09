
BW_BEGIN_NAMESPACE

const float PI = 3.1415926535f;

struct poss3
{
	Vector3 point;
	float angle;
	poss3(Vector3 point_,float angle_) : point(point_), angle(angle_) {}
};

bool comparePoss3 ( poss3& elem1, poss3& elem2 )
{
	return elem1.angle > elem2.angle;
}

void sort3DPoints(BW::vector<Vector3> &points)
{
	// find center
	Vector3 center(0,0,0);
	for(size_t i = 0; i < points.size(); ++i)
		center += points[i];
	center.x = center.x/points.size();
	center.y = center.y/points.size();
	center.z = center.z/points.size();
	// find angles to points from center
	BW::vector<poss3> angles;
	for(size_t i = 0; i < points.size(); ++i)
	{
		Vector3 relP(points[i]-center);
		float len2D = sqrtf(relP.x*relP.x+relP.y*relP.y);
		float angle = relP.x / len2D;
		angle = acosf(angle);
		if(relP.y < 0) angle = PI*2-angle+0.1f;
		angles.push_back(poss3(points[i],angle));
	}
	std::sort(angles.begin(),angles.end(),comparePoss3);
	points.clear();
	for(size_t i = 0; i < angles.size(); ++i)
		points.push_back(angles[i].point);
}

BoundingBox makeBoundingBoxFromLocalVertices(Vector3 *localBoxVertices)
{
	Vector3* locV = localBoxVertices;
	Vector3 minV(1e10,1e10,1e10),maxV(-1e10,-1e10,-1e10);
	for(int i = 0; i < 8; ++i)
	{
		if(locV[i].x < minV.x) minV.x = locV[i].x;
		if(locV[i].y < minV.y) minV.y = locV[i].y;
		if(locV[i].z < minV.z) minV.z = locV[i].z;
		if(locV[i].x > maxV.x) maxV.x = locV[i].x;
		if(locV[i].y > maxV.y) maxV.y = locV[i].y;
		if(locV[i].z > maxV.z) maxV.z = locV[i].z;
	}
	BoundingBox b;
	b.setBounds(minV,maxV);
	return b;
}

Vector2* distanceFromPointToWorldBoxNormalized(Vector3* worldBox,Vector3* point,Vector2* res,const Vector4* texCoord)
{
	// u coord
	{
		Vector3 p = worldBox[0];
		Vector3 n = worldBox[1]-worldBox[0];
		float size = n.length();
		n.normalise();
		float D = n.dotProduct(p);
		float dist = point->dotProduct(n)-D;
		// normalize x coord to requested texture diapason
		res->x = max(texCoord->x, min(texCoord->x + dist * texCoord->z / size, texCoord->z));
	}
	// v coord
	{
		Vector3 p = worldBox[0];
		Vector3 n = worldBox[3]-worldBox[0];
		float size = n.length();
		n.normalise();
		float D = n.dotProduct(p);
		float dist = point->dotProduct(n)-D;
		// normalize v coord to requested texture diapason
		res->y = max(texCoord->y, min(texCoord->y + dist * texCoord->w / size, texCoord->w));
	}
	return res;
}

void makeBoxFromStartEndAndSize(const Vector3 &start, const Vector3 &end, 
								const Vector3 &size, const Vector3 &up, 
								Vector3 *worldBox,Vector3 *locBox,
								Matrix *toWorld,Matrix *toLoc)
{
	Vector3 zAxis(end-start);
	zAxis.normalise();
	Vector3 yAxis(up);
	Vector3 xAxis(yAxis.crossProduct(zAxis));
	xAxis.normalise();
	yAxis.crossProduct(zAxis,xAxis);
	Vector3 hS = size/2;
	Vector3 extent = end-start;
	if(worldBox)
	{
		worldBox[0]	= start-xAxis*hS.x+yAxis*hS.y;
		worldBox[1]	= start+xAxis*hS.x+yAxis*hS.y;
		worldBox[2]	= start+xAxis*hS.x-yAxis*hS.y;
		worldBox[3] = start-xAxis*hS.x-yAxis*hS.y;
		worldBox[4] = worldBox[0]+extent;
		worldBox[5] = worldBox[1]+extent;
		worldBox[6] = worldBox[2]+extent;
		worldBox[7] = worldBox[3]+extent;
	}
	if(locBox)
	{
		Matrix m(Vector4(xAxis.x,xAxis.y,xAxis.z,0),
			Vector4(yAxis.x,yAxis.y,yAxis.z,0),
			Vector4(zAxis.x,zAxis.y,zAxis.z,0),
			Vector4(start.x,start.y,start.z,1));
		*toWorld = m;
		m.invert();
		*toLoc = m;
		locBox[0] = m.applyPoint(worldBox[0]);
		locBox[1] = m.applyPoint(worldBox[1]);
		locBox[2] = m.applyPoint(worldBox[2]);
		locBox[3] = m.applyPoint(worldBox[3]);
		locBox[4] = m.applyPoint(worldBox[4]);
		locBox[5] = m.applyPoint(worldBox[5]);
		locBox[6] = m.applyPoint(worldBox[6]);
		locBox[7] = m.applyPoint(worldBox[7]);
	}
}

//Rotates segment start-end in plane formed by vectors start-end and v by the angle 
void rotateSegmentInPlane(Vector3& start, Vector3& end, const Vector3& v, float angle)
{
	// center point of rotation
	Vector3 center = (end + start) * 0.5f;

	Vector3 v0 = end - center;
	float length = v0.length();
	v0.normalise();

	if(length == 0.0f)
		return;

	// vector perpendicular to v0 and laying in the plane v0-v
	Vector3 p = v0 * v0.dotProduct(v) - v;
	p.normalise();

	// rotate v0 in plane by angle
	end = v0 * cosf(angle) + p * sinf(angle);
	// restore length
	end *= length;
	// restore center
	start = center - end;
	end = center + end;
}

BW_END_NAMESPACE
