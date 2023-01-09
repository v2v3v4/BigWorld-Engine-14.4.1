#include "pch.hpp"
#include "dirty_chunk_list.hpp"

#include "chunk/chunk.hpp"


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	Section: DirtyChunkList  (1 list)
///////////////////////////////////////////////////////////////////////////////

DirtyChunkList::DirtyChunkList( const BW::string& type )
	: type_( type ), requireProcessingInBackground_( false ),
	requireProcessingInMainThread_( false )
{
}


bool DirtyChunkList::isDirty( const BW::string& chunkName ) const
{
	BW_GUARD;

	return chunks_.find( chunkName ) != chunks_.end();
}


void DirtyChunkList::dirty( Chunk* pChunk )
{
	BW_GUARD;

	chunks_[ pChunk->identifier() ] = pChunk;
}


void DirtyChunkList::clean( const BW::string& chunkName )
{
	BW_GUARD;

	chunks_.erase( chunkName );
}


bool DirtyChunkList::any() const
{
	BW_GUARD;

	for (ChunkMap::const_iterator iter = chunks_.begin();
		iter != chunks_.end(); ++iter)
	{
		return true;
	}

	return false;
}


int DirtyChunkList::num() const
{
	return (int)chunks_.size();
}


DirtyChunkList::iterator DirtyChunkList::begin()
{
	return chunks_.begin();
}


DirtyChunkList::iterator DirtyChunkList::end()
{
	return chunks_.end();
}


DirtyChunkList::iterator DirtyChunkList::erase( iterator iter )
{
	return chunks_.erase( iter );
}


DirtyChunkList::const_iterator DirtyChunkList::begin() const
{
	return chunks_.begin();
}


DirtyChunkList::const_iterator DirtyChunkList::end() const
{
	return chunks_.end();
}


bool DirtyChunkList::empty() const
{
	return chunks_.empty();
}


void DirtyChunkList::clear()
{
	BW_GUARD;

	chunks_.clear();
}


///////////////////////////////////////////////////////////////////////////////
//	Section: DirtyChunkLists  (all the  lists)
///////////////////////////////////////////////////////////////////////////////

DirtyChunkLists::DirtyChunkLists() :
	needSync_()
{
}


DirtyChunkList& DirtyChunkLists::operator[]( const BW::string& type )
{
	BW_GUARD;

	DirtyLists::iterator iter = dirtyLists_.find( type );

	if (iter != dirtyLists_.end())
	{
		return iter->second;
	}

	return dirtyLists_.insert(
		DirtyLists::value_type( type, DirtyChunkList( type ) ) ).first->second;

}


const DirtyChunkList& DirtyChunkLists::operator[]( const BW::string& type ) const
{
	BW_GUARD;

	DirtyLists::const_iterator iter = dirtyLists_.find( type );

	if (iter != dirtyLists_.end())
	{
		return iter->second;
	}

	static DirtyChunkList dummy( "dummy" );
	return dummy;
}


size_t DirtyChunkLists::size() const
{
	return dirtyLists_.size();
}


DirtyChunkList& DirtyChunkLists::operator[]( size_t index )
{
	BW_GUARD;

	DirtyLists::iterator iter = dirtyLists_.begin();

	std::advance( iter, index );

	return iter->second;
}


const DirtyChunkList& DirtyChunkLists::operator[]( size_t index ) const
{
	BW_GUARD;

	DirtyLists::const_iterator iter = dirtyLists_.begin();

	std::advance( iter, index );

	return iter->second;
}


const BW::string& DirtyChunkLists::name( size_t index ) const
{
	BW_GUARD;

	DirtyLists::const_iterator iter = dirtyLists_.begin();

	std::advance( iter, index );

	return iter->first;
}


bool DirtyChunkLists::empty() const
{
	BW_GUARD;

	bool result = true;

	for (DirtyLists::const_iterator iter = dirtyLists_.begin();
		iter != dirtyLists_.end(); ++iter)
	{
		if (!iter->second.empty())
		{
			result = false;
			break;
		}
	}

	return result;
}


bool DirtyChunkLists::any() const
{
	BW_GUARD;

	bool result = false;

	for (DirtyLists::const_iterator iter = dirtyLists_.begin();
		iter != dirtyLists_.end(); ++iter)
	{
		if (iter->second.any())
		{
			result = true;
			break;
		}
	}

	return result;
}


int DirtyChunkLists::num() const
{
	BW_GUARD;

	BW::set<Chunk*> chunks;

	for (DirtyLists::const_iterator iter = dirtyLists_.begin();
		iter != dirtyLists_.end(); ++iter)
	{
		for (DirtyChunkList::const_iterator cit = iter->second.begin();
			cit != iter->second.end(); ++cit)
		{
			chunks.insert( cit->second );
		}
	}

	return static_cast<int>(chunks.size());
}


bool DirtyChunkLists::isDirty( const BW::string& chunkName ) const
{
	BW_GUARD;

	bool result = false;

	for (DirtyLists::const_iterator iter = dirtyLists_.begin();
		iter != dirtyLists_.end(); ++iter)
	{
		if (iter->second.isDirty( chunkName ))
		{
			result = true;
			break;
		}
	}

	return result;
}


void DirtyChunkLists::clean( const BW::string& chunkName )
{
	BW_GUARD;

	for (DirtyLists::iterator iter = dirtyLists_.begin();
		iter != dirtyLists_.end(); ++iter)
	{
		iter->second.clean( chunkName );
	}
}


void DirtyChunkLists::clear()
{
	BW_GUARD;

	dirtyLists_.clear();
}

BW_END_NAMESPACE

// dirty_chunk_list.cpp
