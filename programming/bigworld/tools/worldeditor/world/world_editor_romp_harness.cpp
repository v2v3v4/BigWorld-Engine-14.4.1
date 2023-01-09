#include "pch.hpp"
#include "gizmo/tool_manager.hpp"
#include "world_editor_romp_harness.hpp" 
#include "worldeditor/world/items/editor_chunk_water.hpp"

BW_BEGIN_NAMESPACE

void WorldEditorRompHarness::disturbWater()
{
	BW_GUARD;

	if ( ToolManager::instance().tool() && ToolManager::instance().tool()->locator() )
	{
		waterMovement_[0] = waterMovement_[1];
		waterMovement_[1] = ToolManager::instance().tool()->locator()->
			transform().applyToOrigin();

		SimpleMutexHolder holder(EditorChunkWater::instancesMutex());
		const BW::vector<EditorChunkWater*>& waterItems = EditorChunkWater::instances();
		BW::vector<EditorChunkWater*>::const_iterator i = waterItems.begin();
		for (; i != waterItems.end(); ++i)
		{
			(*i)->sway( waterMovement_[0], waterMovement_[1], 1.f );
		}

		//s_phloem.addMovement( waterMovement_[0], waterMovement_[1] );
	}
}

BW_END_NAMESPACE

