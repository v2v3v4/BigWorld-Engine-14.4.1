#include "cell_info.hpp"

#include "cstdmf/watcher.hpp"
#include "math/math_extra.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellInfo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CellInfo::CellInfo( SpaceID spaceID,
							const BW::Rect & rect,
							const Mercury::Address & addr,
							BinaryIStream & stream ) :
						spaceID_( spaceID ),
						addr_( addr ),
						load_( 0.f ),
						rect_( rect ),
						shouldDelete_( false ),
						isDeletePending_( false ),
						hasBeenCreated_( false )
{
	this->updateFromStream( stream );
}


/**
 *	This method updates this objects data based on the input stream.
 */
void CellInfo::updateFromStream( BinaryIStream & stream )
{
	if (isDeletePending_)
	{
		// We received a geometry update, so we must not be pending deletion
		// any more.
		INFO_MSG( "CellInfo::updateFromStream: "
				"received update for re-added cell %s in space %u previously "
				"pending deletion\n",
			addr_.c_str(), spaceID_ );

		isDeletePending_ = false;
	}

	stream >> load_ >> hasBeenCreated_;
}


/**
 *	Destructor.
 */
CellInfo::~CellInfo()
{
}


/**
 *	This method return the CellInfo at the input point in this subtree.
 */
const CellInfo * CellInfo::pCellAt( float x, float z ) const
{
	if (!this->hasBeenCreated())
	{
		// We are unsuitable for offloading.
		return NULL;
	}

	return this;
}


/**
 *	This method visits all of the rectangles in this subtree.
 */
void CellInfo::visitRect( const BW::Rect & rect, CellInfoVisitor & visitor )
{
	if (this->hasBeenCreated())
	{
		visitor.visit( *this );
	}
}


/**
 *	This method adds this leaf node to the input stream.
 */
void CellInfo::addToStream( BinaryOStream & stream ) const
{
	stream << true; // It's a leaf
	stream << addr_;
}


/**
 *	This static method returns a generic watcher for the class Space::CellInfo.
 */
WatcherPtr CellInfo::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (!pWatcher)
	{
		pWatcher = new DirectoryWatcher;

		pWatcher->addChild( "rect", makeWatcher( &CellInfo::rect ) );
		pWatcher->addChild( "load", makeWatcher( &CellInfo::getLoad ) );
		pWatcher->addChild( "shouldDelete", 
			makeWatcher( &CellInfo::shouldDelete ) );
		pWatcher->addChild( "isDeletePending", 
			makeWatcher( &CellInfo::isDeletePending ) );
		pWatcher->addChild( "hasBeenCreated", 
			makeWatcher( &CellInfo::hasBeenCreated ) );
	}

	return pWatcher;
}

BW_END_NAMESPACE

// cell_info.cpp
