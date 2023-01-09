#ifndef CELLAPP_HPP
#define CELLAPP_HPP

#include "cells.hpp"
#include "cellappmgr_interface.hpp"

#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

class CellApp;
class CellAppGroup;
class CellApps;
class CellData;

/**
 *	This class is used to represent a cell application.
 */
class CellApp : public Mercury::ChannelOwner
{
public:
	CellApp( const Mercury::Address & addr, uint16 viewerPort,
		  CellAppID id );
	~CellApp();

	void addCell( CellData * pCellData );
	void eraseCell( CellData * pCellData );

	CellData * findCell( SpaceID id ) const;

	uint16 viewerPort() const			{ return viewerPort_; }

	void startRetiring();
	void shutDown();

	void controlledShutDown( ShutDownStage stage, GameTime shutDownTime );

	void shutDownSpace( SpaceID spaceID );

	void informOfLoad( const CellAppMgrInterface::informOfLoadArgs & args );
	void updateBounds( BinaryIStream & data );
	void retireApp();
	void ackCellAppShutDown(
			const CellAppMgrInterface::ackCellAppShutDownArgs & args );

	void handleUnexpectedDeath( BinaryOStream & baseAppData );

	float currLoad() const		{ return currLoad_; }
	float lastReceivedLoad() const	{ return lastReceivedLoad_; }
	float smoothedLoad() const	{ return smoothedLoad_; }
	float estimatedLoad() const	{ return estimatedLoad_; }
	int   numEntities() const	{ return numEntities_; }

	CellData * chooseCellToOffload() const;

	int numCells() const	{ return cells_.size(); }
	bool isEmpty() const	{ return cells_.empty(); }

	void incCellsRetiring();
	void decCellsRetiring();

	bool hasMulticellSpace() const;

	bool hasTimedOut( uint64 currTime, uint64 timeoutPeriod ) const;

	CellAppID id() const					{ return id_; }
	bool isRetiring() const				{ return isRetiring_; }
	void isRetiring( bool value );

	void retireAllCells();
	void cancelRetiringCells();

	bool hasOnlyRetiringCells() const;
	bool hasAnyRetiringCells() const;

	ShutDownStage shutDownStage() const		{ return shutDownStage_; }

	const Cells & cells() const				{ return cells_; }

	CellAppGroup * pGroup() const			{ return pGroup_; }
	void markGroup( CellAppGroup * pGroup );

	static WatcherPtr pWatcher();

private:
	void updateEstimatedLoad( float loadIncrease );

	float	currLoad_;
	float	lastReceivedLoad_;
	float	smoothedLoad_;
	float	estimatedLoad_;
	int		numEntities_;

	uint16 viewerPort_;

	Cells cells_;
	CellAppID id_;

	bool	isRetiring_;
	int		numCellsRetiring_;

	ShutDownStage shutDownStage_;

	CellAppGroup *		pGroup_;

	friend class CellAppGroup;
};


#ifdef CODE_INLINE
#include "cellapp.ipp"
#endif

BW_END_NAMESPACE

#endif // CELLAPP_HPP
