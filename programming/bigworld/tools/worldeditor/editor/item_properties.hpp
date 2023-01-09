#ifndef ITEM_PROPERTIES_HPP
#define ITEM_PROPERTIES_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/general_properties.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a MatrixProxy for chunk item pointers.
 */
class ChunkItemMatrix : public MatrixProxy
{
public:
	ChunkItemMatrix( ChunkItemPtr pItem );
	~ChunkItemMatrix();

	void getMatrix( Matrix & m, bool world );
	void getMatrixContext( Matrix & m );
	void getMatrixContextInverse( Matrix & m );
	bool setMatrix( const Matrix & m );

	void recordState();
	bool commitState( bool revertToRecord, bool addUndoBarrier );

	virtual bool hasChanged();

	Vector3 movementSnaps() const { return movementSnaps_; }
	void movementSnaps( const Vector3& v ) { movementSnaps_ = v; }

protected:
	ChunkItemPtr	pItem_;
	bool			haveRecorded_;
private:
	Chunk *			origChunk_;
	Matrix			origPose_;

	Vector3			movementSnaps_;
	bool			warned_;
	bool			speedTree_;
	bool			shownError_;
	Matrix			lastGoodPose_;

	friend class MyChunkItemMatrix;
};


/**
 *	This static method in the MatrixProxy base class gets the default
 *	matrix proxy for a given chunk item. It may return NULL.
 */
class WEMatrixProxyCreator : public MatrixProxyCreator
{
public:
	MatrixProxyPtr operator()( ChunkItemPtr pItem );
};


class ChunkItemPositionProperty : public GenPositionProperty
{
	float length( ChunkItemPtr item );
public:
	ChunkItemPositionProperty( const Name& name, MatrixProxyPtr pMatrix, ChunkItemPtr item )
		: GenPositionProperty( name, pMatrix, length( item ) )
	{}
};

/**
 *	Wrap some property that has an accessor to get and set it. It always
 *	uses an undoable data proxy. The set accessor should return a bool
 *	indicating whether or not the value set to was legal.
 *
 *	Template arguments are class to be overriden, and proxy to use.
 */
template <class CL, class DT> class AccessorDataProxy :
	public UndoableDataProxy<DT>
{
public:
	typedef typename DT::Data (CL::*GetFn)() const;
	typedef bool (CL::*SetFn)( const typename DT::Data & v );

	AccessorDataProxy( SmartPointer<CL> pItem,
			const BW::string & propDesc, GetFn getFn, SetFn setFn ) :
		pItem_( pItem ),
		propDesc_( propDesc ),
		getFn_( getFn ),
		setFn_( setFn )
	{
	}

	virtual typename DT::Data get() const
	{
		BW_GUARD;

		return ((*pItem_).*getFn_)();
	}

	virtual void setTransient( typename DT::Data v )
	{
		BW_GUARD;

		((*pItem_).*setFn_)( v );
	}

	virtual bool setPermanent( typename DT::Data v )
	{
		BW_GUARD;

		// set it
		if (!((*pItem_).*setFn_)( v )) return false;

		// ok, flag the chunk as having changed then
		WorldManager::instance().changedChunk( pItem_->chunk() );
		pItem_->edPostModify();

		// update its data section
		pItem_->edSave( pItem_->pOwnSect() );

		return true;
	}

	virtual BW::string opName()
	{
		BW_GUARD;

		return "Set " + pItem_->edDescription().str() + ' ' + propDesc_;
	}

private:
	SmartPointer<CL>	pItem_;
	BW::string			propDesc_;
	GetFn				getFn_;
	SetFn				setFn_;
};

/**
 *	Wrap some property that has an accessor to get and set it. It always
 *	uses an undoable data proxy. The set accessor should return a bool
 *	indicating whether or not the value set to was legal.
 *
 *	Template arguments are class to be overriden, and proxy to use.
 */
template <class CL, class DT> class AccessorDataProxyWithName :
	public UndoableDataProxy<DT>
{
public:
	typedef typename DT::Data (CL::*GetFn)( const BW::string& name ) const;
	typedef bool (CL::*SetFn)( const BW::string& name, const typename DT::Data & v );
	typedef bool (CL::*RangeFn)( const BW::string& name, typename DT::Data & min, typename DT::Data & max,
		int& digits );
	typedef bool (CL::*GetDefaultFn)( const BW::string& name, typename DT::Data & def );
	typedef void (CL::*SetToDefaultFn)( const BW::string& name );
	typedef bool (CL::*IsDefaultFn)( const BW::string& name );

	AccessorDataProxyWithName( SmartPointer<CL> pItem,
			const BW::string & propDesc, GetFn getFn, SetFn setFn, RangeFn rangeFn = NULL,
			GetDefaultFn getDefaultFn = NULL, SetToDefaultFn setToDefaultFn = NULL, IsDefaultFn isDefaultFn = NULL ) :
		pItem_( pItem ),
		propDesc_( propDesc ),
		getFn_( getFn ),
		setFn_( setFn ),
		rangeFn_( rangeFn ),
		getDefaultFn_( getDefaultFn ),
		setToDefaultFn_( setToDefaultFn ),
		isDefaultFn_( isDefaultFn )
	{
	}

	virtual typename DT::Data get() const
	{
		BW_GUARD;

		return ((*pItem_).*getFn_)( propDesc_ );
	}

	virtual void setTransient( typename DT::Data v )
	{
		BW_GUARD;

		((*pItem_).*setFn_)( propDesc_, v );
	}

	// for matrix proxy
	virtual void getMatrix( typename DT::Data & m, bool world = true )
	{
		BW_GUARD;

		m = ((*pItem_).*getFn_)( propDesc_ );
	}
	virtual void getMatrixContext( Matrix & m ){}
	virtual void getMatrixContextInverse( Matrix & m ){}
	virtual bool setMatrix( const typename DT::Data & m )
	{
		BW_GUARD;

		((*pItem_).*setFn_)( propDesc_, m );
		return true;
	}
	virtual void recordState(){}
	virtual bool commitState( bool revertToRecord = false, bool addUndoBarrier = true ){ return false; }

	/** If the state has changed since the last call to recordState() */
	virtual bool hasChanged(){	return false; }

	virtual bool setPermanent( typename DT::Data v )
	{
		BW_GUARD;

		// set it
		if (!((*pItem_).*setFn_)( propDesc_, v )) return false;

		// ok, flag the chunk as having changed then
		WorldManager::instance().changedChunk( pItem_->chunk() );
		pItem_->edPostModify();

		// update its data section
		pItem_->edSave( pItem_->pOwnSect() );

		return true;
	}

	virtual BW::string opName()
	{
		BW_GUARD;

		return "Set " + pItem_->edDescription().str() + ' ' + propDesc_;
	}

	bool getRange( int& min, int& max ) const
	{
		BW_GUARD;

		DT::Data mind, maxd;
		int digits;
		if( rangeFn_ )
		{
			bool result =  ((*pItem_).*rangeFn_)( propDesc_, mind, maxd, digits );
			min = mind;
			max = maxd;
			return result;
		}
		return false;
	}
	bool getRange( float& min, float& max, int& digits ) const
	{
		BW_GUARD;

		DT::Data mind, maxd;
		if( rangeFn_ )
		{
			bool result =  ((*pItem_).*rangeFn_)( propDesc_, mind, maxd, digits );
			min = mind;
			max = maxd;
			return result;
		}
		return false;
	}
	bool getDefault( float& def ) const
	{
		BW_GUARD;

		if( getDefaultFn_ )
			return ((*pItem_).*getDefaultFn_)( propDesc_, def );
		return false;
	}
	void setToDefault()
	{
		BW_GUARD;

		if( setToDefaultFn_ )
			((*pItem_).*setToDefaultFn_)( propDesc_ );
	}
	bool isDefault() const
	{
		BW_GUARD;

		if( isDefaultFn_ )
			return ((*pItem_).*isDefaultFn_)( propDesc_ );
		return false;
	}
private:
	SmartPointer<CL>	pItem_;
	BW::string			propDesc_;
	GetFn				getFn_;
	SetFn				setFn_;
	RangeFn				rangeFn_;
	GetDefaultFn		getDefaultFn_;
	SetToDefaultFn		setToDefaultFn_;
	IsDefaultFn			isDefaultFn_;
};


/**
 *	Wrap some property that has an accessor to get and set it transiently,
 *	but must be edSave and reloaded to take permanent effect. It always
 *	uses an undoable data proxy.
 *
 *	Template arguments are class to be overriden, and proxy to use.
 */
template <class CL, class DT> class SlowPropReloadingProxy :
	public UndoableDataProxy<DT>
{
public:
	typedef typename DT::Data (CL::*GetFn)() const;
	typedef void (CL::*SetFn)( const typename DT::Data & v );

	SlowPropReloadingProxy( SmartPointer<CL> pItem,
			const BW::string & propDesc, GetFn getFn, SetFn setFn ) :
		pItem_( pItem ),
		propDesc_( propDesc ),
		getFn_( getFn ),
		setFn_( setFn )
	{
	}

	virtual typename DT::Data get() const
	{
		BW_GUARD;

		return ((*pItem_).*getFn_)();
	}

	virtual void setTransient( typename DT::Data v )
	{
		BW_GUARD;

		((*pItem_).*setFn_)( v );
	}

	virtual bool setPermanent( typename DT::Data v )
	{
		BW_GUARD;

		// set it
		this->setTransient( v );

		// flag the chunk as having changed
		WorldManager::instance().changedChunk( pItem_->chunk() );
		pItem_->edPostModify();

		// update its data section
		pItem_->edSave( pItem_->pOwnSect() );

		// see if we can load it (no harm in calling overriden method)
		if (!pItem_->reload()) return false;

		// if we failed then our caller will call setPermanent back to the
		//  old value so it's ok to return here with a damaged item.
		return true;
	}

	virtual BW::string opName()
	{
		BW_GUARD;

		return "Set " + pItem_->edDescription().str() + ' ' + propDesc_;
	}

private:
	SmartPointer<CL>	pItem_;
	BW::string			propDesc_;
	GetFn				getFn_;
	SetFn				setFn_;
};


// helper method for representing chunk pointers
extern BW::string chunkPtrToString( Chunk * pChunk );

/**
 *	Wrap a constant (for the user) chunk pointer and present it as name.
 */
template <class CL> class ConstantChunkNameProxy : public StringProxy
{
public:
	explicit ConstantChunkNameProxy( SmartPointer<CL> pItem,
									Chunk * (CL::*getFn)() const = NULL ) :
		pItem_( pItem ),
		pLastChunk_( NULL ),
		getFn_( getFn )
	{
	}

	virtual BW::string get() const
	{
		BW_GUARD;

		Chunk * pNewChunk = NULL;

		if (getFn_ == NULL)
		{
			pNewChunk = pItem_->chunk();
		}
		else
		{
			pNewChunk = ((*pItem_).*getFn_)();
		}

		if (pNewChunk != pLastChunk_)
		{
			pLastChunk_ = pNewChunk;
			cachedString_ = chunkPtrToString( pNewChunk );
		}
		return cachedString_;
	}

	virtual void set( BW::string, bool, bool ) { }

private:
	SmartPointer<CL> pItem_;
	mutable Chunk * pLastChunk_;
	mutable BW::string cachedString_;
	Chunk * (CL::*getFn_)() const;
};

BW_END_NAMESPACE

#endif // ITEM_PROPERTIES_HPP
