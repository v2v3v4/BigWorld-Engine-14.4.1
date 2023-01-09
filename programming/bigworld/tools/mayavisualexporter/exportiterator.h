#ifndef _53E8E491_6A4A_4879_A603_6BC63A4DB3FA__
#define _53E8E491_6A4A_4879_A603_6BC63A4DB3FA__

BW_BEGIN_NAMESPACE

class ExportIteratorBase
{
	ExportIteratorBase( const ExportIteratorBase& );
	ExportIteratorBase& operator=( const ExportIteratorBase& );
public:
	ExportIteratorBase(){}
	virtual bool isDone( MStatus* status = NULL ) = 0;
	virtual MStatus next() = 0;
	virtual MStatus reset() = 0;
	virtual MObject item( MStatus* status = NULL ) = 0;
	virtual ~ExportIteratorBase(){}
};

class DependencyExportIterator : public ExportIteratorBase
{
	MItDependencyNodes mItDependencyNodes;
	MObject mHolder;
public:
	DependencyExportIterator( MFn::Type filter = MFn::kInvalid, MStatus* status = NULL )
		: mItDependencyNodes( filter, status )
	{}
	virtual bool isDone( MStatus* status = NULL )
	{
		return mItDependencyNodes.isDone( status );
	}
	virtual MStatus next()
	{
		return mItDependencyNodes.next();
	}
	virtual MStatus reset()
	{
		return mItDependencyNodes.reset();
	}
	virtual MObject item( MStatus* status = NULL )
	{
		return mItDependencyNodes.item( status );
	}
};

class SelectionExportIterator : public ExportIteratorBase
{
	MSelectionList mSelectionList;
	MItSelectionList* mItSelectionList;
public:
	SelectionExportIterator( MFn::Type filter = MFn::kInvalid, MStatus* status = NULL )
	{
		// I ignore the error code of the following line
		// Since if there is an error, it will be reported in the next line
		// We must keep mItSelectionList valid
		MStatus s = MGlobal::getActiveSelectionList ( mSelectionList );
		if( status )
			*status = s;
		mItSelectionList = new MItSelectionList( mSelectionList, filter, status );
	}
	~SelectionExportIterator()
	{
		delete mItSelectionList;
	}
	virtual bool isDone( MStatus* status = NULL )
	{
		return mItSelectionList->isDone( status );
	}
	virtual MStatus next()
	{
		return mItSelectionList->next();
	}
	virtual MStatus reset()
	{
		return mItSelectionList->reset();
	}
	virtual MObject item( MStatus* status = NULL )
	{
		MObject result;
		MStatus s = mItSelectionList->getDependNode( result );
		if( status )
			*status = s;
		return result;
	}
};

class ExportIterator : public ExportIteratorBase
{
	ExportIteratorBase* mExportIterator;
public:
	ExportIterator( MFn::Type filter = MFn::kInvalid, MStatus* status = NULL )
	{
		bool isExportingSelection = ExportSettings::instance().nodeFilter()
			== ExportSettings::SELECTED;
		if( isExportingSelection )
			mExportIterator = new SelectionExportIterator( filter, status );
		else
			mExportIterator = new DependencyExportIterator( filter, status );
	}
	~ExportIterator()
	{
		delete mExportIterator;
	}
	virtual bool isDone( MStatus* status = NULL )
	{
		return mExportIterator->isDone( status );
	}
	virtual MStatus next()
	{
		return mExportIterator->next();
	}
	virtual MStatus reset()
	{
		return mExportIterator->reset();
	}
	virtual MObject item( MStatus* status = NULL )
	{
		return mExportIterator->item( status );
	}
};

BW_END_NAMESPACE

#endif//_53E8E491_6A4A_4879_A603_6BC63A4DB3FA__
