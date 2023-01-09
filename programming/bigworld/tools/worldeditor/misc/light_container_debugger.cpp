
#include "pch.hpp"
#include "light_container_debugger.hpp"
#include "../world/world_manager.hpp"

#include "chunk/chunk.hpp"
#include "moo/line_helper.hpp"

BW_BEGIN_NAMESPACE

const uint32 ACTIVE_LIGHT_COLOUR	= 0xffffffaa;
const uint32 INACTIVE_LIGHT_COLOUR	= 0xff888888;


SmartPointer<LightContainerDebugger> LightContainerDebugger::s_instance_ = NULL;

LightContainerDebugger* LightContainerDebugger::instance()
{
	if ( s_instance_ == NULL )
	{
		BW_GUARD;
		s_instance_ = new LightContainerDebugger();
	}
	return s_instance_.getObject();
}


void LightContainerDebugger::fini()
{
	if ( s_instance_ != NULL )
	{
		WorldManager::instance().removeRenderable( s_instance_ );
		s_instance_->clearItems();
		s_instance_ = NULL;
	}
}


LightContainerDebugger::LightContainerDebugger()
{
	BW_GUARD;
	WorldManager::instance().addRenderable( this );
}


void LightContainerDebugger::render()
{
	BW_GUARD;

	if (items_.empty())
		return;

	for (uint32 i=0; i<items_.size(); i++)
	{
		Moo::LightContainerPtr pContainer = items_[i]->edVisualiseLightContainer();
		if (!pContainer || !items_[i]->chunk())
		{
			continue;
		}

		Matrix chunkWorld = items_[i]->chunk()->transform();
		const Vector3& objectCentre = chunkWorld.applyPoint( items_[i]->edTransform().applyToOrigin() );
		
		for (uint32 i = 0; i < pContainer->nOmnis(); ++i)
		{
			uint32 colour = i < 4 ? ACTIVE_LIGHT_COLOUR : INACTIVE_LIGHT_COLOUR;

			Vector3 oPos = pContainer->omni(i)->worldPosition();
			LineHelper::instance().drawLine( oPos, objectCentre, colour );
		}
		for (uint32 i = 0; i < pContainer->nSpots(); ++i)
		{
			uint32 colour = i < 2 ? ACTIVE_LIGHT_COLOUR : INACTIVE_LIGHT_COLOUR;

			Vector3 sPos = pContainer->spot(i)->worldPosition();
			LineHelper::instance().drawLine( sPos, objectCentre, colour );
		}
	}

	// save states
	DWORD oldLighting;
	DWORD oldZEnable;
	Moo::rc().device()->GetRenderState( D3DRS_LIGHTING, &oldLighting );
	Moo::rc().device()->GetRenderState( D3DRS_ZENABLE, &oldZEnable );
	// change states
	Moo::rc().device()->SetRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().device()->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	// draw
	LineHelper::instance().purge();
	// restore states
	Moo::rc().device()->SetRenderState( D3DRS_LIGHTING, oldLighting );
	Moo::rc().device()->SetRenderState( D3DRS_ZENABLE, oldZEnable );
}

BW_END_NAMESPACE

