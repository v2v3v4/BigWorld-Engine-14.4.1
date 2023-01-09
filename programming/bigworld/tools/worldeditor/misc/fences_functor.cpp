#include "pch.hpp"

#include "fences_functor.hpp"
#include "fences_tool_view.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"

#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"
#include "gizmo/item_view.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT(FencesFunctor)

PY_BEGIN_METHODS(FencesFunctor)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(FencesFunctor)
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED(FencesFunctor, "FencesFunctor", Functor)

FUNCTOR_FACTORY(FencesFunctor)


FencesFunctor::FencesFunctor(PyTypePlus * pType):
	ToolFunctor(false, pType),
	isActive_(false)
{
}

void FencesFunctor::update(float dTime, Tool &tool)
{
	BW_GUARD;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView == NULL)
		return;
	
	toolView->setCursorPos( tool.locator()->transform().applyToOrigin() );

	isActive_ = InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE );

	if (isActive_ && toolView->checkCanPlaceNextSection())
		addFenceSection();
}

void FencesFunctor::addFenceSection()
{
	BW_GUARD;

	FencesToolView *toolView = WorldManager::instance().getFenceToolView();
	if (toolView == NULL)
		return;

	WEPythonAdapter *pAdapter = WorldEditorApp::instance().pythonAdapter();
	if (!pAdapter)
		return;

	Matrix m = toolView->addSectionToSequence();

	// see page_objects.cpp for details
	UndoRedo::instance().disableAdd( true );
	pAdapter->ActionScriptExecute( "actXZSnap" );
	bool snapsWasEnabled = !!OptionsSnaps::snapsEnabled();
	if (snapsWasEnabled)
		pAdapter->ActionScriptExecute( "actToggleSnaps" );
	UndoRedo::instance().disableAdd( false );
	OptionsHelper::tick();

	UndoRedo::instance().disableAdd( true );
	PyObjectPtr pyGroup = pAdapter->addChunkItem( toolView->modelName() );
	UndoRedo::instance().disableAdd( false );
	if (pyGroup != NULL 
		&& PyObject_IsInstance( pyGroup.get(), (PyObject*)&ChunkItemGroup::s_type_ ))
	{
		ChunkItemGroup *pGroup = static_cast<ChunkItemGroup*>( pyGroup.get() );
		BW::vector<ChunkItemPtr> &items = pGroup->get();

		if (items.size() == 1)
		{
			ChunkItemPtr pItem = items[0];
			Matrix chunkMatrInv = pItem->chunk()->transform();
			chunkMatrInv.invert();
			m.postMultiply( chunkMatrInv );

			pItem->edTransform( m, false );

			const type_info& ChunkModelTypeId = typeid( EditorChunkModel );
			const type_info& itemTypeId = typeid( *pItem );
			if (itemTypeId == ChunkModelTypeId)
			{
				EditorChunkModel *pModel = (EditorChunkModel*)( pItem.get() );
				FenceInfo *pFenceInfo = pModel->getFenceInfo( true, toolView->fenceId() );
				toolView->fenceId( pFenceInfo->fenceId );
			}

			UndoRedo::instance().add(
				new ChunkItemExistenceOperation( pItem, NULL ) );

			UndoRedo::instance().add(
				new FencesToolStateOperation() );

			UndoRedo::instance().barrier( LocaliseUTF8(
				L"WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CREATE_ASSET",
				pItem->edDescription().c_str() ), false );
		}
	}

	UndoRedo::instance().disableAdd( true );
	if (snapsWasEnabled)
		pAdapter->ActionScriptExecute( "actToggleSnaps" );
	pAdapter->ActionScriptExecute( "actTerrainSnaps" );
	UndoRedo::instance().disableAdd( false );
	OptionsHelper::tick();

	UndoRedo::instance().disableAdd( true );
	pAdapter->setSelection( new ChunkItemGroup() );
	UndoRedo::instance().disableAdd( false );
}

PyObject *FencesFunctor::pyNew( PyObject * )
{
	BW_GUARD;

	return new FencesFunctor();
}

BW_END_NAMESPACE

