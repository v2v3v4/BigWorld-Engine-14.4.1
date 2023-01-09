#include "pch.hpp"

#include "appmgr/options.hpp"
#include "cstdmf/debug.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "common/space_editor.hpp"
#include "editor_chunk_cache_base.hpp"
#include "editor_chunk_model_base.hpp"
#include "gizmo/general_properties.hpp"
#include "moo/texture_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/string_provider.hpp"
#include "moo/geometrics.hpp"



DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
EditorChunkCacheBase::EditorChunkCacheBase( Chunk & chunk ) :
	chunk_( chunk ),
	pChunkSection_( NULL )
{
	BW_GUARD;
	invalidateFlag_ = InvalidateFlags::FLAG_NONE;
}


/**
 *	Load this chunk. We just save the data section pointer
 */
bool EditorChunkCacheBase::load( DataSectionPtr pSec, DataSectionPtr cdata )
{
	BW_GUARD;
	pChunkSection_ = pSec;
	return true;
}


/**
 *	Touch this chunk. We make sure there's one of us in every chunk.
 */
void EditorChunkCacheBase::touch( Chunk & chunk )
{
	BW_GUARD;

	EditorChunkCacheBase::instance( chunk );
}


/**
 *	Save this chunk and any items in it back to the XML file
 */
bool EditorChunkCacheBase::edSave()
{
	BW_GUARD;

	for (int i = 0; i < ChunkCache::cacheNum(); ++i)
	{
		if (ChunkCache* cc = chunk_.cache( i ))
		{
			cc->saveChunk( pChunkSection_ );
		}
	}

	pChunkSection_->save( chunk_.resourceID() );

	// save the binary data
	return this->edSaveCData();
}


/*
 *	Save the binary data, such as lighting to the .cdata file
 */
bool EditorChunkCacheBase::edSaveCData()
{
	BW_GUARD;

	// retrieve (and possibly create) our .cData file
	DataSectionPtr cData = this->pCDataSection();

	// delete lighting section, if any
	cData->deleteSections( "lighting" );
	cData->deleteSections( "staticLighting" );
	cData->deleteSections( "staticLightingCache" );

	bool savedOK = true;

	ChunkCleanFlags cf( cData );
	for (int i = 0; i < ChunkCache::cacheNum(); ++i)
	{
		if (ChunkCache* cc = chunk_.cache( i ))
		{
			cc->saveCData( cData );
			cc->saveCleanFlags( cf );
		}
	}

	cf.save();

	// check to see if need to save to disk (if already exists or there is data)
	if (cData->bytes() > 0)
	{
		// save to disk
		const BW::string fileName = chunk_.binFileName();

		if (!this->saveCDataInternal( cData, fileName ))
		{
			SpaceEditor::instance().addError( &chunk_, NULL,
				LocaliseUTF8(
				L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/CANNOT_OPEN_FILE",
				fileName ).c_str() );
			savedOK = false;
		}
	}

	ChunkProcessorManager::instance().onChunkDirtyStatusChanged( &chunk_ );

	return savedOK;
}


/**
 *  Default implementation of saving cdata into the DataSection.
 */
bool EditorChunkCacheBase::saveCDataInternal( DataSectionPtr ds,
										const BW::string& filename )
{
	BW_GUARD;
	return ds->save( filename );
}


/**
 *	Return the top level data section for this chunk
 */
DataSectionPtr EditorChunkCacheBase::pChunkSection()
{
	return pChunkSection_;
}


/**
 *	Return and possibly create the .cdata section for this chunk.
 *
 *	@return the binary cData section for the chunk
 */
DataSectionPtr	EditorChunkCacheBase::pCDataSection()
{
	BW_GUARD;

	// check to see if file already exists
	const BW::string fileName = chunk_.binFileName();
	DataSectionPtr cData = BWResource::openSection( fileName, false );

	if (!cData)
	{
		// create a section
		BW::string::size_type lastSep = fileName.find_last_of('/');
		BW::string parentName = fileName.substr(0, lastSep);
		DataSectionPtr parentSection = BWResource::openSection( parentName );
		MF_ASSERT(parentSection);

		BW::string tagName = fileName.substr( lastSep + 1 );

		// make it
		cData = new BinSection( tagName,
			new BinaryBlock( NULL, 0, "BinaryBlock/EditorChunk" ) );
		cData->setParent( parentSection );
		cData = cData->convertToZip();
		cData = DataSectionCensus::add( fileName, cData );
	}
	return cData;
}

/**
 *	Ensure that EditorChunkCacheBase::instance() method is
 *	only called for loaded or loading chunks
 */
// This is needed due to the fragility of the interaction between
// EditorChunkCacheBase and EditorChunkCache (in WorldEditor)
// See bug 32433: Reproducible random memory crash in WE using stress tester
// for details.
// `__declspec(noinline)` is to prevent MSVC 2017 from overly-aggressively inlining this funciton specialisation;
//  there are calls to this API from other libraries, which will fail to link.
template <> __declspec(noinline)
EditorChunkCacheBase & ChunkCache::Instance<EditorChunkCacheBase>::operator()( Chunk & chunk ) const
{
	MF_ASSERT( chunk.loading() || chunk.loaded() );
	ChunkCache * & cc = chunk.cache( index_ );
	if (cc == NULL) cc = new EditorChunkCacheBase( chunk );
	return static_cast<EditorChunkCacheBase &>(*cc);
}


/// Static instance accessor initialiser
ChunkCache::Instance<EditorChunkCacheBase> EditorChunkCacheBase::instance;
BW_END_NAMESPACE

