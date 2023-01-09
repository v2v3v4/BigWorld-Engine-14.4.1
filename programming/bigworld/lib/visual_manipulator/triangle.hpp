#ifndef VISUAL_MANIPULATOR_TRIANGLE_HPP
#define VISUAL_MANIPULATOR_TRIANGLE_HPP


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{

struct Triangle
{
	uint32 a;
	uint32 b;
	uint32 c;
	uint32 materialIndex;

/*	bool operator == (const Triangle& triangle);
	bool operator < (const Triangle& triangle);*/
};

}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_TRIANGLE_HPP
