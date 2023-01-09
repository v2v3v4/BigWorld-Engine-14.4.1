#include "pch.hpp"
#include "editor_chunk_model_base.hpp"
#include "chunk/chunk.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/bwresource.hpp"
#include "common/space_editor.hpp"
#include "model/super_model.hpp"
#include "moo/visual_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EditorChunkModelBase::EditorChunkModelBase()
	: firstToss_( true )
{
}


/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkModelBase::load( DataSectionPtr pSection, Chunk * pChunk,
								BW::string* pErrorString )
{
	BW_GUARD;

	firstToss_ = true;
	pOwnSect_ = pSection;

	return this->ChunkModel::load( pSection, pChunk, pErrorString );
}


BW::vector<BW::string> EditorChunkModelBase::extractVisualNames() const
{
	BW_GUARD;

	BW::vector<BW::string> models;
	pOwnSect_->readStrings( "resource", models );

	BW::vector<BW::string> v;
	v.reserve( models.size() );

	for (uint i = 0; i < models.size(); i++)
	{
		DataSectionPtr modelSection = BWResource::openSection( models[i] );
		if (!modelSection)
		{
			WARNING_MSG( "Couldn't read model %s for ChunkModel\n", models.front().c_str() );
			continue;
		}

		BW::string visualName = modelSection->readString( "nodelessVisual" );

		if (visualName.empty())
		{
			visualName = modelSection->readString( "nodefullVisual" );
			if (visualName.empty())
			{
				WARNING_MSG( "ChunkModel %s has a model that has no visual\n", models[i].c_str() );
			}
		}

		BW::string fullVisualName = visualName + ".static.visual";

		Moo::VisualPtr visual = Moo::VisualManager::instance()->get(
			fullVisualName
			);

		if (!visual)
		{
			fullVisualName = visualName + ".visual";
			visual = Moo::VisualManager::instance()->get( fullVisualName );
		}

		if (visual)
			v.push_back( fullVisualName );
		else
			v.push_back( "" );
	}

	return v;
}


bool EditorChunkModelBase::isVisualFileNewer() const
{
	BW_GUARD;

	MF_ASSERT(chunk());
	BW::vector<BW::string> visualNames = extractVisualNames();
	BW::string cdataName = chunk()->binFileName();

	for (BW::vector<BW::string>::iterator it = visualNames.begin();
		it != visualNames.end(); ++it)
	{
		if (BWResource::isFileOlder( cdataName, *it ))
		{
			// ok, the lighting data, navMesh, shadowMap is out of date, bail
			return true;
		}
	}

	return false;
}


/**
 *	This method does extra stuff when this item is tossed between chunks.
 *
 *	It updates its datasection in that chunk.
 */
void EditorChunkModelBase::tossWithExtra( Chunk * pChunk, SuperModel* extraModel )
{
	BW_GUARD;

	this->ChunkModel::tossWithExtra( pChunk, extraModel );

	if (firstToss_)
	{
		// If the .visual file is newer than the .cdata file, then we need update
		// the navMesh, shadow map, static lighting and thumbnail of that chunk
		if (pChunk_ && isVisualFileNewer())
		{
			SpaceEditor::instance().changedChunk( pChunk_, *this );
		}

		firstToss_ = false;
	}
}


/// Write the factory statics stuff
/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)
IMPLEMENT_CHUNK_ITEM( EditorChunkModelBase, model, 1 )
IMPLEMENT_CHUNK_ITEM_ALIAS( EditorChunkModelBase, shell, 1 )

BW_END_NAMESPACE
// editor_chunk_model.cpp
