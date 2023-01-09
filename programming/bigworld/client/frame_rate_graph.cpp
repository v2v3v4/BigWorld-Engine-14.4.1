#include "pch.hpp"
#include "frame_rate_graph.hpp"


BW_BEGIN_NAMESPACE

/*static*/ bool FrameRateGraph::s_display_ = false;


FrameRateGraph::FrameRateGraph() :
	valueIndex_(0)
{
	BW_GUARD;
	mat_.fogged( false );
	mat_.zBufferRead( false );
	mat_.zBufferWrite( false );
	Moo::TextureStage ts;
	ts.colourOperation( Moo::TextureStage::SELECTARG2 );
	mat_.addTextureStage( ts );
	mat_.addTextureStage( Moo::TextureStage() );

	createUnmanagedObjects();

	MF_WATCH( "Render/DisplayFramerateGraph",
		s_display_,
		Watcher::WT_READ_WRITE,
		"Enable frame rate graph, including 30 and 60 fps indicators." );
}


FrameRateGraph::~FrameRateGraph()
{
}


void FrameRateGraph::graph( float t )
{
	BW_GUARD;
	if (s_display_)
	{

		float sHeight = Moo::rc().screenHeight();
		// The dHeight makes 60hz be halfway up the screen;
		float dHeight = sHeight / 120.f;
		float fps = sHeight - (dHeight / t);
		values_[valueIndex_++] = fps;
		valueIndex_ = valueIndex_ % 100;
		uint vi = valueIndex_;
		for (int i = 0; i < 100; i++)
		{
			verts_[ i ].pos_.y = values_[ vi % 100 ];
			vi++;
		}
		DX::Device* pDev = Moo::rc().device();
		mat_.set();
		Moo::rc().setFVF( Moo::VertexTL::fvf() );
		pDev->DrawPrimitiveUP( D3DPT_LINELIST, 3, measuringLines_, sizeof( Moo::VertexTL ) );
		pDev->DrawPrimitiveUP( D3DPT_LINESTRIP, 99, verts_, sizeof( Moo::VertexTL ) );
	}
}


void FrameRateGraph::createUnmanagedObjects()
{
	BW_GUARD;
	float sWidth = Moo::rc().screenWidth();
	float sHeight = Moo::rc().screenHeight();
	// The dHeight makes 60hz be halfway up the screen;
	float dHeight = sHeight / 120.f;

    for (int i = 0; i < 100; i++)
	{
		values_[i] = sHeight;
		verts_[i].pos_.set( (sWidth / 100.f) * float(i), 0, 0, 1 );
		verts_[i].colour_ = 0x00ff0000;

	}

	measuringLines_[0].pos_.set( 0, sHeight - float(30 * dHeight), 0, 1 );
	measuringLines_[1].pos_.set( sWidth, sHeight - float(30 * dHeight), 0, 1 );
	measuringLines_[2].pos_.set( 0, sHeight - float(60 * dHeight), 0, 1 );
	measuringLines_[3].pos_.set( sWidth, sHeight - float(60 * dHeight), 0, 1 );
	measuringLines_[4].pos_.set( 0, sHeight - float(55 * dHeight), 0, 1 );
	measuringLines_[5].pos_.set( sWidth, sHeight - float(55 * dHeight), 0, 1 );
	measuringLines_[0].colour_ = 0x000000ff;
	measuringLines_[1].colour_ = 0x000000ff;
	measuringLines_[2].colour_ = 0x000000ff;
	measuringLines_[3].colour_ = 0x000000ff;
	measuringLines_[4].colour_ = 0x0000ff00;
	measuringLines_[5].colour_ = 0x0000ff00;

}

BW_END_NAMESPACE

// frame_rate_graph.cpp
