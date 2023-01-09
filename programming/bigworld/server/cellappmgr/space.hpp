#ifndef SPACE_HPP
#define SPACE_HPP

#include "cells.hpp"

#include "math/math_extra.hpp" // For BW::Rect

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class CellApp;

namespace CM
{
class BSPNode;
}


/**
 *	This class is used to manage the different spaces.
 */
class Space
{
public:
	Space( SpaceID id = 0, bool isNewSpace = true,
		bool isFromDB = false, uint32 preferredIP = 0 );
	~Space();

	void shutDown();

	SpaceID id() const		{ return id_; }

	CellData * addCell( CellApp & cellApp, CellData * pCellToSplit = NULL );
	CellData * addCell();
	void addCell( CellData * pCell );
	CellData * addCellTo( CellData * pCellToSplit );

	void informCellAppsOfGeometry( bool shouldSend );

	void recoverCells( BinaryIStream & data );
	CellData * recoverCell( CellApp & cellApp,
			const Mercury::Address & addr );

	void loadBalance();
	void updateRanges();
	CellData * findCell( float x, float z ) const;
	CellData * findCell( const Mercury::Address & addr ) const;

	void addToStream( BinaryOStream & stream,
		   bool isForViewer = false ) const;
	void addGeometryMappingsToStream( BinaryOStream & stream ) const;

	void shouldOffload( bool value );

	bool isLarge() const;

	int numCells() const	{ return cells_.size(); }

	const Cells & cells() const	{ return cells_; }

	void eraseCell( CellData * pCell, bool notifyCellApps = false );

	void addData( const SpaceEntryID & entryID, uint16 key,
						const BW::string & data, bool possibleRepeat = false,
						bool setLastMappedGeometry = true );
	void delData( const SpaceEntryID & entryID );
	void sendSpaceDataUpdate( const Mercury::Address& srcAddr,
		const SpaceEntryID& entryID, uint16 key, const BW::string* pValue );

	void preferredIP( uint32 ip ) { preferredIP_ = ip; }
	uint32 preferredIP() const { return preferredIP_; }

	uint numUniqueIPs() const;

	int numCellAppsOnIP( uint32 ip ) const;

	void sendToDB( BinaryOStream & stream ) const;

	void updateBounds( BinaryIStream & data );

	bool cellsHaveLoadedMappedGeometry() const;
	void checkCellsHaveLoadedMappedGeometry();

	void setLastMappedGeometry( const BW::string& lastMappedGeometry );

	bool hasLoadedRequiredChunks() const;

	void spaceGrid( const float spaceGrid )	{ spaceGrid_ = spaceGrid; }
	float spaceGrid() const		{ return spaceGrid_; }

	const BW::Rect & spaceBounds() const		{ return spaceBounds_; }

	void hasHadEntities( bool value )	{ hasHadEntities_ = value; }
	bool hasHadEntities() const			{ return hasHadEntities_; }

	// Watcher stats
	float minLoad() const;
	float maxLoad() const;
	float avgLoad() const;

	float artificialMinLoad() const;
	float artificialMinLoadCellShare( float receivedCellLoad ) const;

	static void addCellToDelete( CellData * pCell );
	static void endRecovery();

	static WatcherPtr pWatcher();

	void debugPrint();

private:
	// Watcher stats
	float areaNotLoaded() const;
	int numRetiringCells() const;

	BW::string geometryMappingAsString() const;

	struct DataEntry
	{
		uint16 key;
		BW::string data;
	};
	typedef BW::map< SpaceEntryID, DataEntry > DataEntries;
	DataEntries dataEntries_;

	SpaceID	id_;
	Cells cells_;

	CM::BSPNode * pRoot_;

	bool isBalancing_;
	uint32 preferredIP_;

	bool isFirstCell_;
	bool isFromDB_;
	bool hasHadEntities_;

	BW::string lastMappedGeometry_;
	int waitForChunkBoundUpdateCount_;

	float spaceGrid_;
	BW::Rect spaceBounds_;

	float artificialMinLoad_;

	typedef BW::set< CellData * > CellsToDelete;
	static CellsToDelete cellsToDelete_;
};

#ifdef CODE_INLINE
#include "space.ipp"
#endif

BW_END_NAMESPACE

#endif // SPACE_HPP
