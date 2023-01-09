#ifndef FRAME_RATE_GRAPH_HPP
#define FRAME_RATE_GRAPH_HPP


#include "moo/device_callback.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This helper class displays a graph of the framerate.
 *	It is drawn even when the engine statistics console is not up.
 */
class FrameRateGraph : public Moo::DeviceCallback
{
public:
	FrameRateGraph();
	~FrameRateGraph();

	void graph( float t );

private:
	void createUnmanagedObjects();

    uint valueIndex_;
	float values_[100];
	Moo::VertexTL verts_[100];
	Moo::Material mat_;
	Moo::VertexTL measuringLines_[6];
	static bool s_display_;
};

BW_END_NAMESPACE

#endif // FRAME_RATE_GRAPH_HPP

// frame_rate_graph.hpp
