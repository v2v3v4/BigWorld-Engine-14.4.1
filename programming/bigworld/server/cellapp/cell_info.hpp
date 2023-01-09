#ifndef CELL_INFO_HPP
#define CELL_INFO_HPP

#include "space_node.hpp"

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer<Watcher> WatcherPtr;


/**
 *	This interface is implemented by classes that are used to visit all
 *	cells in a space.
 */
class CellInfoVisitor
{
public:
	virtual ~CellInfoVisitor() {};
	virtual void visit( CellInfo & cell ) {};
};


/**
 *	This class is used to represent a leaf node of the BSP. It corresponds
 *	to a cell in the space.
 */
class CellInfo : public SpaceNode, public ReferenceCount
{
public:
	CellInfo( SpaceID spaceID, const BW::Rect & rect,
			const Mercury::Address & addr, BinaryIStream & stream );
	~CellInfo();

	static WatcherPtr pWatcher();

	void updateFromStream( BinaryIStream & stream );

	virtual void deleteTree() {};
	virtual const CellInfo * pCellAt( float x, float z ) const;
	virtual void visitRect( const BW::Rect & rect, CellInfoVisitor & visitor );
	virtual void addToStream( BinaryOStream & stream ) const;

	const Mercury::Address & addr() const	{ return addr_; }
	float getLoad() const					{ return load_; }

	bool shouldDelete() const			{ return shouldDelete_; }
	void shouldDelete( bool v )			{ shouldDelete_ = v; }

	const BW::Rect & rect() const		{ return rect_; }
	void rect( const BW::Rect & rect )	{ rect_ = rect; }

	bool contains( const Vector3 & pos ) const
		{ return rect_.contains( pos.x, pos.z ); }

	void setPendingDelete()			{ isDeletePending_ = true; }
	bool isDeletePending() const	{ return isDeletePending_; }
	bool hasBeenCreated() const 	{ return hasBeenCreated_; }

private:
	SpaceID				spaceID_;
	Mercury::Address	addr_;
	float				load_;
	BW::Rect			rect_;

	bool				shouldDelete_;
	bool				isDeletePending_;
	bool				hasBeenCreated_;
};

typedef SmartPointer< CellInfo > CellInfoPtr;
typedef ConstSmartPointer< CellInfo > ConstCellInfoPtr;

BW_END_NAMESPACE

#endif // CELL_INFO_HPP
