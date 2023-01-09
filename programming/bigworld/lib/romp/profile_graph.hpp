#ifndef PROFILE_GRAPH_HPP
#define PROFILE_GRAPH_HPP

#if ENABLE_CONSOLES
//#include "math\vector2.hpp"
#include "moo\moo_math.hpp"


BW_BEGIN_NAMESPACE

class XConsole;

static const Moo::Colour s_colours[] = 
{
// 	0xffffff00,	// yellow  // not used as these are used as default colours already
// 	0xffffffff,	// white

	0xffff0000,	// red
	0xffFF7F00,	// orange
	0xff93DB70,	// yellow green
	0xff00ff00,	// green
	0xff007fff,	// slate blue
	0xff00ffff,	// cyan
	0xffff00ff,	// magenta
	0xffeaadea,	// plum
	0xffeaeaae,	// medium goldenrod
	0xff8fbc8f,	// pale green
};

static const int s_numColourEntries = sizeof(s_colours)/sizeof(s_colours[0]);

class ProfileGraph
{
public:
	ProfileGraph(Vector2 topLeft, Vector2 bottomRight);
	~ProfileGraph();

	void setNumPoints(int numPoints);
	void drawGraph(const char* name, float* pointData, int offset, XConsole & console);
	void drawBorder( XConsole & console);

	static Moo::Colour getColour(int i) { return s_colours[ i % s_numColourEntries];}

private:
	Vector2 tl_, br_;
	Vector2 * points_;
	int maxNumPoints_;
	float upperLimit_;
	float maxVal_;
	int numGraphs_;
};

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
#endif // PROFILE_GRAPH_HPP
