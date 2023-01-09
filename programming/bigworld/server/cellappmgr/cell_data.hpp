#ifndef CELL_DATA_HPP
#define CELL_DATA_HPP

#include "bsp_node.hpp"

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class CellApp;
class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;


/**
 *	The CellAppMgr uses objects of this type to represent the cells that are
 *	currently running on the server.
 */
class CellData : public CM::BSPNode
{
public:
	CellData( CellApp & cellApp, Space & space );
	CellData( Space & space, BinaryIStream & data );
	~CellData();

	// BSPNode overrides
	virtual CM::BSPNode * addCell( CellData * pCell, bool isHorizontal );
	virtual CM::BSPNode * removeCell( CellData * pCell );

	virtual void visit( CM::CellVisitor & visitor ) const;
	virtual CellData * pCellAt( float x, float y ) const
		{ return const_cast<CellData*>( this ); }

	virtual bool hasBeenCreated() const		{ return hasBeenCreated_; }
	void hasBeenCreated( bool v )			{ hasBeenCreated_ = v; }

	virtual float updateLoad();
	virtual float avgLoad() const;
	virtual float avgSmoothedLoad() const;
	virtual float totalSmoothedLoad() const;
	float lastReceivedLoad() const;

	virtual float minLoad() const;
	virtual float maxLoad() const;

	virtual int cellCount() const { return 1; }

	virtual void balance( const BW::Rect & range,
			float loadSafetyBound, bool isShrinking );
	virtual void updateRanges( const BW::Rect & range );

	virtual void addToStream( BinaryOStream & stream, bool isForViewer );

	void setIsRetiring( bool val );
	virtual bool isRetiring() const		{ return numRetiring_ != 0; }

	virtual const BW::Rect & balanceChunkBounds() const;

	virtual void debugPrint( int indent );

	virtual Space * getSpace() const	{ return pSpace_; }

	virtual float calculateNewAreaNotLoaded(
			bool changeY, bool changeMax, bool moveLeft ) const;

	void setIsLeaf( bool val );
	bool isLeaf() const { return true; }

	virtual CM::BSPNode * addCellTo( CellData * pNewCell,
			CellData * pCellToSplit, bool isHorizontal );

	// ---- General ----

	enum State
	{
		// Note: This needs to be the same as cellapp/cell.hpp
		// Should really have a common header file.
		STATE_UNKNOWN,
		STATE_ADDING,
		STATE_ADDED,
		STATE_REMOVING
	};

	void updateBounds( BinaryIStream & data );

	void shouldOffload( bool enable );

	void setCellApp( CellApp & app );

	uint32 numEntities() const				{ return numEntities_; }

	Space & space()								{ return *pSpace_; }
	CellApp * pCellApp() const					{ return pCellApp_; }
	CellApp & cellApp() const					{ return *pCellApp_; }

	const Mercury::Address & addr() const;

	void removeSelf();
	void startRetiring();
	void cancelRetiring();

	static CM::BSPNode * readTree( Space * pSpace, BinaryIStream & data );
	static void skipDataInStream( BinaryIStream & data );

	virtual WatcherPtr getWatcher();
	static WatcherPtr pWatcher();

private:
	static void readRecoveryData( Mercury::Address& addr, BinaryIStream& data );
	void sendRetireCell( bool isRetiring ) const;

	void updateEntityBounds( BinaryIStream & data );

	float calculateAreaNotLoaded() const;

	float calculateAreaNotLoaded( const BW::Rect & range,
		   bool shouldDebugIfNonZero = false ) const;

	CellApp *			pCellApp_;
	Space *				pSpace_;

	/// Number of entities including ghosts
	uint32				numEntities_;

	bool				hasBeenCreated_;
	bool				isOverloaded_;
	bool				hasHadLoadedArea_;
	Mercury::Address	recoveringAddr_;
};


#ifdef CODE_INLINE
#include "cell_data.ipp"
#endif

BW_END_NAMESPACE

#endif // CELL_DATA_HPP
