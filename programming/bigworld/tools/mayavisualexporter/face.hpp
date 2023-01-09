#ifndef __face_hpp__
#define __face_hpp__

BW_BEGIN_NAMESPACE

struct Face
{
	Face();
	
	int positionIndex[3];
	int normalIndex[3];
	int tangentIndex[3];
	int uvIndex[3];
	int uvIndex2[3];
	int colourIndex[3];
	int materialIndex;
};

BW_END_NAMESPACE

#endif // __face_hpp__