#include "pch.hpp"
#include "clean_chunk_list.hpp"
#include "romp/progress.hpp"
#include "chunk/chunk_cache.hpp"
#include "chunk/geometry_mapping.hpp"

BW_BEGIN_NAMESPACE

namespace
{

/**
 *	This function returns the file information of the given file via parameter
 *	pFileInfo. If the given pathname is not a file, it will return false.
 */
bool getFileInfo( const BW::string& pathName, IFileSystem::FileInfo* pFileInfo )
{
	BW_GUARD;

	IFileSystem::FileType ft = BWResource::getFileType( pathName, pFileInfo );

	return ft == IFileSystem::FT_FILE ||
		ft == IFileSystem::FT_ARCHIVE;
}

}


/**
 *	Constructor
 */
CleanChunkList::CleanChunkList( GeometryMapping& dirMapping )
	: dirMapping_( dirMapping )
{
	BW_GUARD;

	load();
}


/**
 *	Saves out the clean chunk list
 */
void CleanChunkList::save() const
{
	BW_GUARD;

	DataSectionPtr dsDL = getDataSection();

	dsDL->deleteSections( "chunk" );

	for (BW::map<BW::string, ChunkInfo>::const_iterator iter = checkedChunks_.begin();
		iter != checkedChunks_.end(); ++iter)
	{
		DataSectionPtr ds = dsDL->newSection( "chunk" );

		ds->writeUInt64( "cdataCreationTime", iter->second.cdataCreationTime_ );
		ds->writeUInt64( "cdataModificationTime", iter->second.cdataModificationTime_ );
		ds->writeUInt64( "chunkCreationTime", iter->second.chunkCreationTime_ );
		ds->writeUInt64( "chunkModificationTime", iter->second.chunkModificationTime_ );
		ds->writeString( "name", iter->first );
	}

	for (BW::map<BW::string, ChunkInfo>::const_iterator iter = uncheckedChunks_.begin();
		iter != uncheckedChunks_.end(); ++iter)
	{
		DataSectionPtr ds = dsDL->newSection( "chunk" );

		ds->writeUInt64( "cdataCreationTime", iter->second.cdataCreationTime_ );
		ds->writeUInt64( "cdataModificationTime", iter->second.cdataModificationTime_ );
		ds->writeUInt64( "chunkCreationTime", iter->second.chunkCreationTime_ );
		ds->writeUInt64( "chunkModificationTime", iter->second.chunkModificationTime_ );
		ds->writeString( "name", iter->first );
	}

	dsDL->save();
}


/**
 *	Check every unchecked time stamp against the time stamp of chunk and cdata
 *	files to see if it has been modified outside WE
 */
void CleanChunkList::sync( Progress* task )
{
	BW_GUARD;

	BW::set<BW::string> chunks = dirMapping_.gatherChunks();
	int chunkProcessed = 0;

	if (task)
	{
		task->name( "Syncing chunks" );
		task->length( (float)chunks.size() );
	}

	for (BW::set<BW::string>::iterator iter = chunks.begin();
		iter != chunks.end(); ++iter)
	{
		if (uncheckedChunks_.find( *iter ) != uncheckedChunks_.end())
		{
			BW::string chunkName = dirMapping_.path() + *iter + ".chunk";
			BW::string cdataName = dirMapping_.path() + *iter + ".cdata";
			IFileSystem::FileInfo fiChunk;
			IFileSystem::FileInfo fiCdata;

			if (getFileInfo( chunkName, &fiChunk ) &&
				getFileInfo( cdataName, &fiCdata ))
			{
				if (uncheckedChunks_[ *iter ].cdataCreationTime_ == fiCdata.created &&
					uncheckedChunks_[ *iter ].cdataModificationTime_ == fiCdata.modified &&
					uncheckedChunks_[ *iter ].chunkCreationTime_ == fiChunk.created &&
					uncheckedChunks_[ *iter ].chunkModificationTime_ == fiChunk.modified)
				{
					checkedChunks_[ *iter ] = uncheckedChunks_[ *iter ];
				}
			}

			uncheckedChunks_.erase( *iter );
		}

		++chunkProcessed;

		if ( task )
		{
			if ( task->isCancelled() )
			{
				return;
			}
			else if ( chunkProcessed % 20 == 0 )
			{
				task->set( float( chunkProcessed ) );
			}
		}
	}
}


/**
 *	This function is called when the files of a chunk has been updated
 *	inside WE so its time stamp and clean status gets updated
 */
void CleanChunkList::update( Chunk* pChunk )
{
	BW_GUARD;

	MF_ASSERT( pChunk );

	ChunkInfo ci;
	IFileSystem::FileInfo fiChunk;
	IFileSystem::FileInfo fiCdata;

	if (uncheckedChunks_.find( pChunk->identifier() ) !=
		uncheckedChunks_.end())
	{
		uncheckedChunks_.erase( pChunk->identifier() );
	}

	bool clean = true;

	for (int i = 0; i < ChunkCache::cacheNum(); ++i)
	{
		if (ChunkCache* cc = pChunk->cache( i ))
		{
			if (cc->dirty())
			{
				clean = false;

				break;
			}
		}
	}

	if (clean)
	{
		if (getFileInfo( pChunk->resourceID(), &fiChunk ) &&
			getFileInfo( pChunk->binFileName(), &fiCdata ))
		{
			ci.cdataCreationTime_ = fiCdata.created;
			ci.cdataModificationTime_ = fiCdata.modified;
			ci.chunkCreationTime_ = fiChunk.created;
			ci.chunkModificationTime_ = fiChunk.modified;

			checkedChunks_[ pChunk->identifier() ] = ci;

			return;
		}
	}

	if (checkedChunks_.find( pChunk->identifier() ) !=
		checkedChunks_.end())
	{
		checkedChunks_.erase( pChunk->identifier() );
	}
}


/**
 *	This function marks the given chunk as clean. It ignores
 *	the chunk dirty status.
 */
void CleanChunkList::markAsClean( const BW::string& chunk )
{
	BW_GUARD;

	ChunkInfo ci;
	IFileSystem::FileInfo fiChunk;
	IFileSystem::FileInfo fiCdata;

	if (uncheckedChunks_.find( chunk ) != uncheckedChunks_.end())
	{
		uncheckedChunks_.erase( chunk );
	}

	BW::string chunkName = dirMapping_.path() + chunk + ".chunk";
	BW::string cdataName = dirMapping_.path() + chunk + ".cdata";

	if (getFileInfo( chunkName, &fiChunk ) &&
		getFileInfo( cdataName, &fiCdata ))
	{
		ci.cdataCreationTime_ = fiCdata.created;
		ci.cdataModificationTime_ = fiCdata.modified;
		ci.chunkCreationTime_ = fiChunk.created;
		ci.chunkModificationTime_ = fiChunk.modified;

		checkedChunks_[ chunk ] = ci;

		return;
	}

	if (checkedChunks_.find( chunk ) != checkedChunks_.end())
	{
		checkedChunks_.erase( chunk );
	}
}


/**
 *	This function returns whether the chunk is clean
 */
bool CleanChunkList::isClean( const BW::string& chunk ) const
{
	BW_GUARD;

	return checkedChunks_.find( chunk ) != checkedChunks_.end();
}


/**
 *	Load the clean list
 */
void CleanChunkList::load()
{
	BW_GUARD;

	DataSectionPtr dsDL = getDataSection();

	for (int i = 0; i < dsDL->countChildren(); ++i)
	{
		DataSectionPtr ds = dsDL->openChild( i );

		if (ds->sectionName() == "chunk")
		{
			ChunkInfo ci;

			ci.cdataCreationTime_ = ds->readUInt64( "cdataCreationTime" );
			ci.cdataModificationTime_ = ds->readUInt64( "cdataModificationTime" );
			ci.chunkCreationTime_ = ds->readUInt64( "chunkCreationTime" );
			ci.chunkModificationTime_ = ds->readUInt64( "chunkModificationTime" );

			uncheckedChunks_[ ds->readString( "name" ) ] = ci;
		}
	}
}


/**
 *	Return the clean list data section
 */
DataSectionPtr CleanChunkList::getDataSection() const
{
	BW_GUARD;

	return BWResource::openSection( dirMapping_.path() + "space.cleanlist", true );
}

BW_END_NAMESPACE

// clean_chunk_list.cpp
