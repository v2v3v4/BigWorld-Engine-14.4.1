#include "spaces.hpp"

#include "profile.hpp"
#include "space.hpp"
#include "cell_app_channel.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Destructor.
 */
Spaces::~Spaces()
{
	INFO_MSG( "Spaces::~Spaces: going to free %zu spaces\n", container_.size() );	
	
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		delete iter->second;
		++iter;
	}
}


Space * Spaces::find( SpaceID id ) const
{

	Container::const_iterator iter = container_.find( id );

	return (iter != container_.end()) ? iter->second : NULL;
}


Space * Spaces::create( SpaceID id )
{
	MF_ASSERT( this->find( id ) == NULL );

	Space * pSpace = new Space( id );
	container_[ id ] = pSpace;
	return pSpace;
}


/**
 *	This method prepares all chunks that are in the process of fully loading
 *	for deletion.
 */
void Spaces::prepareNewlyLoadedChunksForDelete()
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->prepareNewlyLoadedChunksForDelete();

		++iter;
	}
}


/**
 *	This method checks whether there are any chunks that need to be loaded or
 *	unloaded.
 */
void Spaces::tickChunks()
{
	AUTO_SCOPED_PROFILE( "chunksMainThread" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );

	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->chunkTick();

		++iter;
	}
}


/**
 *	This method deletes any spaces that have fully unloaded.
 */
void Spaces::deleteOldSpaces()
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		Container::iterator currIter = iter;
		++iter;

		Space * pSpace = currIter->second;

		if (pSpace->pCell() == NULL && pSpace->isFullyUnloaded())
		{
			// spaces only erased from container_ here
			// (no reentrancy to invalidate iterators)
			container_.erase( currIter );
			delete pSpace;
		}
	}
}


void Spaces::writeRecoveryData( BinaryOStream & stream )
{
	stream << uint32( container_.size() );

	Container::iterator spaceIter = container_.begin();

	while (spaceIter != container_.end())
	{
		Space * pSpace = spaceIter->second;
		pSpace->writeRecoveryData( stream );

		++spaceIter;
	}
}

WatcherPtr Spaces::pWatcher()
{
	WatcherPtr pWatcher = new MapWatcher<Container>( container_ );
	pWatcher->addChild( "*", new BaseDereferenceWatcher( Space::pWatcher() ) );

	return pWatcher;
}


/**
 *	This method checks whether there are other cells in the spaces that 
 *	we currently have.
 *
 *	@param remoteChannel	The remote CellAppChannel.
 *
 *	@return		true if other cells existing in our spaces on other CellApps,
 *				false otherwise.
 */
bool Spaces::haveOtherCellsIn( const CellAppChannel & remoteChannel ) const
{
	Container::const_iterator iter = container_.begin();

	while (iter != container_.end())
	{
		if (iter->second->findCell( remoteChannel.addr() ) != NULL)
		{
			return true;
		}
		++iter;
	}

	return false;
}


BW_END_NAMESPACE

// spaces.cpp

