#include "pch.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "common/material_utility.hpp"

#include "moo/custom_mesh.hpp"

BW_BEGIN_NAMESPACE

SpaceMapDebug* SpaceMapDebug::s_instance_ = NULL;


SpaceMapDebug& SpaceMapDebug::instance()
{
	if (s_instance_ == NULL)
	{
		BW_GUARD;

		s_instance_ = new SpaceMapDebug();
	}
	return *s_instance_;
}


/*static*/ void SpaceMapDebug::deleteInstance()
{
	BW_GUARD;

	delete s_instance_;
	s_instance_ = NULL;
}


SpaceMapDebug::SpaceMapDebug():
	visible_(false)
{
	BW_GUARD;

	MF_WATCH( "Render/Space Map/draw debug overlay", visible_ );
	material_ = new Moo::EffectMaterial();
	material_->load( BWResource::openSection( "resources/materials/space_map.mfm" ));
	MaterialUtility::viewTechnique( material_, "spaceMap" );
}

SpaceMapDebug::~SpaceMapDebug()
{
	BW_GUARD;

	material_ = NULL;
}


void SpaceMapDebug::spaceInformation( const SpaceInformation& info )
{
	BW_GUARD;

	if (info_ != info)
	{
		info_ = info;
		debugInfo_.clear();
	}
}


void SpaceMapDebug::onDraw( int16 gridX, int16 gridZ, uint32 colour )
{
	BW_GUARD;

	if (visible_)
		debugInfo_.push_back( DebugInfo( gridX, gridZ, colour ) );
}


void SpaceMapDebug::onConsidered( int16 gridX, int16 gridZ, uint32 colour )
{
	BW_GUARD;

	if (visible_)
		debugInfo_.push_back( DebugInfo( gridX, gridZ, colour, 0.5f ) );
}


void SpaceMapDebug::draw()
{
	BW_GUARD;

	if (!visible_)
		return;

	MaterialUtility::viewTechnique( material_, "spaceMapDebug" );

	if (material_->begin())
	{
		for ( uint32 i=0; i<material_->numPasses(); i++ )
		{
			material_->beginPass(i);			
			Moo::rc().setTexture(0,NULL);

			BW::deque<DebugInfo>::iterator it = debugInfo_.begin();
			BW::deque<DebugInfo>::iterator end = debugInfo_.end();
			while (it != end )
			{
				DebugInfo& di = *it++;				
				uint32 colour = di.colour_ & 0x00ffffff;
				uint32 tFactor = (uint8)(di.age_ * 255.f);
				colour = colour | (tFactor << 24);
				di.age_ = max( di.age_ - 0.033f, 0.f );

				uint16 biasedX,biasedZ;
				biasGrid(info_.localToWorld_,di.gridX_,di.gridZ_,biasedX,biasedZ);							

				Vector2 offSet( 1.f / Moo::rc().screenWidth(), 1.f / Moo::rc().screenHeight() );

				float x = float(biasedX) - offSet.x;
				float y = float(biasedZ) - offSet.y;
				float dx = 1.f;
				float dy = 1.f;

				static CustomMesh< Moo::VertexXYZL > s_gridSquare( D3DPT_TRIANGLEFAN );				

				s_gridSquare.clear();
				Moo::VertexXYZL v;
				v.pos_.set(x,y,0.f);
				v.colour_ = colour;
				s_gridSquare.push_back(v);

				v.pos_.set(x+dx,y,0.f);
				v.colour_ = colour;
				s_gridSquare.push_back(v);

				v.pos_.set(x+dx,y+dy,0.f);
				v.colour_ = colour;
				s_gridSquare.push_back(v);

				v.pos_.set(x,y+dy,0.f);
				v.colour_ = colour;
				s_gridSquare.push_back(v);

				s_gridSquare.drawEffect();	
			}
			material_->endPass();
		}
		material_->end();
	}

	MaterialUtility::viewTechnique( material_, "spaceMap" );

	BW::deque<DebugInfo>::iterator it = debugInfo_.begin();
	BW::deque<DebugInfo>::iterator end = debugInfo_.end();
	if (it != end )
	{
		DebugInfo& di = *it;
		if (di.age_ <= 0.f)
		{
			debugInfo_.pop_front();		
		}		
	}
}

BW_END_NAMESPACE
