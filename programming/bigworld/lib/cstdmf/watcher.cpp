#include "pch.hpp"

#include "config.hpp"
#if ENABLE_WATCHERS

#include "watcher.hpp"

#include "fini_job.hpp"
#include "watcher_path_request.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Watcher Methods
// -----------------------------------------------------------------------------

// Cannot be a smart pointer because of static init ordering.
Watcher * g_pRootWatcher = NULL;
const char * g_pRootWatcherPath = NULL;
DEFINE_WATCH_DEBUG_MESSAGE_PRIORITY_FUNC

namespace
{

class WatcherFini : public FiniJob
{
public:
	virtual bool fini()
	{
		Watcher::fini();

		return true;
	}
};


// This object deletes itself
FiniJobPtr pFini = new WatcherFini();

} // anonymous namespace


/**
 *	This method returns whether a root watcher exists. It's to be used by
 *	methods that do not wish to cause the watcher hierarchy to come into
 *	existance.
 *
 *	@return	true if the root watcher exists, false otherwise.
 */
bool Watcher::hasRootWatcherInternal()
{
	return (g_pRootWatcher != NULL);
}


/**
 *	This method returns the root watcher.
 *
 *	@return	A reference to the root watcher.
 */
Watcher & Watcher::rootWatcherInternal()
{
	if (g_pRootWatcher == NULL)
	{
		g_pRootWatcher = new DirectoryWatcher();
		g_pRootWatcher->incRef();
	}

	return *g_pRootWatcher;
}

void Watcher::finiInternal() 
{
	if (g_pRootWatcher)
	{
		g_pRootWatcher->decRef();
		g_pRootWatcher = NULL;
	}
}


bool Watcher::visitChildren( const void * base, const char * path,
		WatcherVisitor & visitor )
{
	WatcherPathRequestWatcherVisitor wpr( visitor );
	return this->visitChildren( base, path, wpr );
}

BW_END_NAMESPACE

#include <assert.h>

#define SORTED_WATCHERS

/*
#ifndef CODE_INLINE
#include "debug_value.ipp"
#endif
*/

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: DirectoryWatcher Methods
// -----------------------------------------------------------------------------

/**
 *	Constructor for DirectoryWatcher.
 */
DirectoryWatcher::DirectoryWatcher():
container_(0)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
}


/*
 *	This method is an override from Watcher.
 */
bool DirectoryWatcher::getAsString( const void * base,
								const char * path,
								BW::string & result,
								BW::string & desc,
								Watcher::Mode & mode ) const
{
	if (isEmptyPath(path))
	{
		result = "<DIR>";
		mode = WT_DIRECTORY;
		desc = comment_;
		return true;
	}
	else
	{
		DirData * pChild = this->findChild( path );

		if (pChild != NULL)
		{
			const void * addedBase = (const void*)(
				((const uintptr)base) + ((const uintptr)pChild->base) );
			return pChild->watcher->getAsString( addedBase, this->tail( path ),
				result, desc, mode );
		}
		else
		{
			return false;
		}
	}
	// never gets here
}

bool DirectoryWatcher::getAsStream( const void * base,
								const char * path,
								WatcherPathRequestV2 & pathRequest ) const
{
	if (isEmptyPath(path))
	{
		Watcher::Mode mode = WT_DIRECTORY;
		watcherValueToStream( pathRequest.getResultStream(), "<DIR>", mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}
	else if (isDocPath( path ))
	{
		Watcher::Mode mode = Watcher::WT_READ_ONLY;
		watcherValueToStream( pathRequest.getResultStream(), comment_, mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}
	else
	{
		DirData * pChild = this->findChild( path );

		if (pChild != NULL)
		{
			const void * addedBase = (const void*)(
				((const uintptr)base) + ((const uintptr)pChild->base) );
			return pChild->watcher->getAsStream( addedBase, this->tail( path ),
				pathRequest );
		}
		else
		{
			return false;
		}
	}
	// never gets here
}



/*
 *	Override from Watcher.
 */
bool DirectoryWatcher::setFromString( void * base,
	const char * path, const char * value )
{
	DirData * pChild = this->findChild( path );

	if (pChild != NULL)
	{
		void * addedBase = (void*)( ((uintptr)base) + ((uintptr)pChild->base) );
		return pChild->watcher->setFromString( addedBase, this->tail( path ),
			value );
	}
	else
	{
		return false;
	}
}



/*
 *	Override from Watcher.
 */
bool DirectoryWatcher::setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest )
{
	DirData * pChild = this->findChild( path );

	if (pChild != NULL)
	{
		void * addedBase = (void*)( ((uintptr)base) + ((uintptr)pChild->base) );
		return pChild->watcher->setFromStream( addedBase, this->tail( path ),
			pathRequest );
	}
	else
	{
		return false;
	}
}



/*
 *	Override from Watcher
 */
bool DirectoryWatcher::visitChildren( const void * base, const char * path,
	WatcherPathRequest & pathRequest ) 
{
	bool handled = false;

	// Inform the path request of the number of children 
	MF_ASSERT( container_.size() <= INT_MAX );
	pathRequest.addWatcherCount( ( int )container_.size() );

	if (isEmptyPath(path))
	{
		Container::iterator iter = container_.begin();

		while (iter != container_.end())
		{
			const void * addedBase = (const void*)(
				((const uintptr)base) + ((const uintptr)(*iter).base) );

			if (!pathRequest.addWatcherPath( addedBase, NULL,
									(*iter).label, *(*iter).watcher ))
			{
				break;
			}

			iter++;
		}

		handled = true;
	}
	else
	{
		DirData * pChild = this->findChild( path );

		if (pChild != NULL)
		{
			const void * addedBase = (const void*)(
				((const uintptr)base) + ((const uintptr)pChild->base) );

			handled = pChild->watcher->visitChildren( addedBase,
				this->tail( path ), pathRequest );
		}
	}

	return handled;
}



/*
 * Override from Watcher.
 */
bool DirectoryWatcher::addChild( const char * path, WatcherPtr pChild,
	void * withBase )
{
	bool wasAdded = false;

	if (isEmptyPath( path ))						// Is it invalid?
	{
		ERROR_MSG( "DirectoryWatcher::addChild: "
			"tried to add unnamed child (no trailing slashes please)\n" );
	}
	else if (strchr(path,'/') == NULL)				// Is it for us?
	{
		// Make sure that we do not add it twice.
		if (this->findChild( path ) == NULL)
		{
			DirData		newDirData;
			newDirData.watcher = pChild;
			newDirData.base = withBase;
			newDirData.label = path;

#ifdef SORTED_WATCHERS
			Container::iterator iter = container_.begin();
			while ((iter != container_.end()) &&
					(iter->label < newDirData.label))
			{
				++iter;
			}
			container_.insert( iter, newDirData );
#else
			container_.push_back( newDirData );
#endif
			wasAdded = true;
		}
		else
		{
			ERROR_MSG( "DirectoryWatcher::addChild: "
				"tried to replace existing watcher %s\n", path );
		}
	}
	else											// Must be for a child
	{
		DirData * pFound = this->findChild( path );

		if (pFound == NULL)
		{
			char * pSeparator = strchr( (char*)path, WATCHER_SEPARATOR );

			size_t compareLength = (pSeparator == NULL) ?
				strlen( (char*)path ) : (pSeparator - path);

			Watcher * pChildWatcher = new DirectoryWatcher();

			DirData		newDirData;
			newDirData.watcher = pChildWatcher;
			newDirData.base = NULL;
			newDirData.label = BW::string( path, compareLength );

#ifdef SORTED_WATCHERS
			Container::iterator iter = container_.begin();
			while ((iter != container_.end()) &&
					(iter->label < newDirData.label))
			{
				++iter;
			}
			pFound = &*container_.insert( iter, newDirData );
#else
			container_.push_back( newDirData );
			pFound = &container_.back();
#endif
		}

		if (pFound != NULL)
		{
			wasAdded = pFound->watcher->addChild( this->tail( path ),
				pChild, withBase );
		}
	}

	return wasAdded;
}


/**
 *	This method removes a child identifier in the path string.
 *
 *	@param path		This string must start with the identifier that you are
 *					searching for. For example, "myDir/myValue" would match the
 *					child with the label "myDir".
 *
 *	@return	true if remove, false if not found
 */
bool DirectoryWatcher::removeChild( const char * path )
{
	if (path == NULL)
		return false;

	char * pSeparator = strchr( (char*)path, WATCHER_SEPARATOR );

	size_t compareLength =
		(pSeparator == NULL) ? strlen( path ) : (pSeparator - path);

	if (compareLength != 0)
	{
		Container::iterator iter = container_.begin();

		while (iter != container_.end())
        {
			if (compareLength == (*iter).label.length() &&
				strncmp(path, (*iter).label.data(), compareLength) == 0)
			{
				if( pSeparator == NULL )
				{
					container_.erase( iter );
					return true;
				}
				else
				{
					DirData* pChild = const_cast<DirData*>(&(const DirData &)*iter);
					return pChild->watcher->removeChild( tail( path ) );
				}
			}
			iter++;
		}
	}

	return false;
}


/*
 * Override from Watcher.
 */
WatcherPtr DirectoryWatcher::getChild( const char * path ) const
{
	if (path == NULL)
	{
		return NULL;
	}

	char * pSeparator = strchr( (char*)path, WATCHER_SEPARATOR );

	size_t compareLength =
		(pSeparator == NULL) ? strlen( path ) : (pSeparator - path);

	if (compareLength != 0)
	{
		Container::const_iterator iter = container_.begin();

		while (iter != container_.end())
        {
			if (compareLength == (*iter).label.length() &&
				strncmp(path, (*iter).label.data(), compareLength) == 0)
			{
				if( pSeparator == NULL )
				{
					return iter->watcher;
				}
				else
				{
					DirData* pChildData = const_cast<DirData*>(&(const DirData &)*iter);
					return pChildData->watcher->getChild( tail( path ) );
				}
			}
			iter++;
		}
	}

	return NULL;
}

/**
 *	This method finds the immediate child of this directory matching the first
 *	identifier in the path string.
 *
 *	@param path		This string must start with the identifier that you are
 *					searching for. For example, "myDir/myValue" would match the
 *					child with the label "myDir".
 *
 *	@return	The watcher matching the input path. NULL if one was not found.
 */
DirectoryWatcher::DirData * DirectoryWatcher::findChild( const char * path ) const
{
	if (path == NULL)
	{
		return NULL;
	}

	char * pSeparator = strchr( (char*)path, WATCHER_SEPARATOR );

	size_t compareLength =
		(pSeparator == NULL) ? strlen( path ) : (pSeparator - path);

	DirData * pChild = NULL;

	if (compareLength != 0)
	{
		Container::const_iterator iter = container_.begin();

		while (iter != container_.end() && pChild == NULL)
		{
			if (compareLength == (*iter).label.length() &&
				strncmp(path, (*iter).label.data(), compareLength) == 0)
			{
				pChild = const_cast<DirData*>(&(const DirData &)*iter);
			}
			iter++;
		}
	}
	else
	{
		ERROR_MSG( "DirectoryWatcher::findChild: Empty watcher path segment\n" );
	}

	return pChild;
}


/**
 *	This method is used to find the string representing the path without the
 *	initial identifier.
 *
 *	@param path		The path that you want to find the tail of.
 *
 *	@return	A string that represents the remainder of the path.
 */
const char * DirectoryWatcher::tail( const char * path )
{
	if (path == NULL)
		return NULL;

	char * pSeparator = strchr( (char*)path, WATCHER_SEPARATOR );

	if (pSeparator == NULL)
		return NULL;

	return pSeparator + 1;
}




// -----------------------------------------------------------------------------
// Section: Constructors/Destructor
// -----------------------------------------------------------------------------

/**
 *	Constructor for Watcher.
 */
Watcher::Watcher( const char * comment ) :
	comment_( comment )
{
}


/**
 *	The destructor deletes all of its children.
 */
Watcher::~Watcher()
{
}


// -----------------------------------------------------------------------------
// Section: Static methods of Watcher
// -----------------------------------------------------------------------------

/**
 *	This is a simple helper method used to partition a path string into the name
 *	and directory strings. For example, "section1/section2/hello" would be split
 *	into "hello" and "section1/section2/".
 *
 *	@param path				The string to be partitioned.
 *	@param resultingName	A reference to a string that is to receive the base
 *							name.
 *	@param resultingDirectory	A reference to a string that is to receive the
 *							directory string.
 */
void Watcher::partitionPath( const BW::string path,
		BW::string & resultingName,
		BW::string & resultingDirectory )
{
	BW::string::size_type pos = path.find_last_of( WATCHER_SEPARATOR );

	if (pos < path.size())
	{
		resultingDirectory	= path.substr( 0, pos + 1 );
		resultingName		= path.substr( pos + 1, path.length() - pos - 1 );
	}
	else
	{
		resultingName		= path;
		resultingDirectory	= "";
	}
}


/**
 *	Utility method to watch the count of instances of a type
 */
void watchTypeCount( const char * basePrefix, const char * typeName, int & var )
{
	BW::string str = basePrefix;
	str += typeName;
	addWatcher( str.c_str(), var );
}


// -----------------------------------------------------------------------------
// Section: CallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CallableWatcher::ArgDescription::ArgDescription(
		WatcherDataType type, const BW::string & description ) :
	type_( type ),
	description_( description )
{
}


/**
 *	Constructor.
 */
CallableWatcher::CallableWatcher( ExposeHint hint, const char * comment ) :
	Watcher( comment ),
	exposeHint_( hint )
{
}


/**
 *	This method adds an argument description to this callable object.
 *
 *	@param type	The type of the argument.
 *	@param description A brief description of the argument.
 */
void CallableWatcher::addArg( WatcherDataType type, const char * description )
{
	argList_.push_back( ArgDescription( type, description ) );
}


/*
 *	Override from Watcher.
 */
bool CallableWatcher::getAsString( const void * base, const char * path,
	BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
{
	if (isEmptyPath( path ))
	{
		result = "Callable function";
		mode = WT_CALLABLE;
		desc = comment_;

		return true;
	}

	return false;
}


/*
 *	Override from Watcher.
 */
bool CallableWatcher::setFromString( void * base, const char * path,
	const char * valueStr )
{
	ERROR_MSG( "PyFunctionWatcher::setFromString: Attempt to call python "
			"function watcher with old protocol (v1) not supported\n" );
	return false;
}


/**
 *	This method prepares the result stream for this CallableWatcher for the
 *	return value to be streamed onto it.
 *
 *	Do not forget to call endResultStream after streaming the return value onto
 *	the stream.
 *
 *	@param	pathRequest		The WatcherPathRequestV2 given to ::setFromStream
 *	@param	output			The stdio output generated by the call, no more than
 *							INT_MAX bytes.
 *	@param	returnValueSize	The size of the return value when streamed as a
 *							watcher value, no more than INT_MAX.
 *
 *	@return					A BinaryOStream to stream the return value onto
 */
BinaryOStream & CallableWatcher::startResultStream(
	WatcherPathRequestV2 & pathRequest, const BW::string & output,
	int returnValueSize ) const
{
	MF_ASSERT( output.length() <= INT_MAX );

	BinaryOStream & resultStream = pathRequest.getResultStream();

	// Callable Watchers always return a tuple of:
	// ( stdio output string, returnValue )
	const int TUPLE_ITEM_COUNT = 2;

	resultStream << (uint8)WATCHER_TYPE_TUPLE;
	resultStream << (uint8)WT_READ_ONLY;

	int resultSize = BinaryOStream::calculatePackedIntSize( TUPLE_ITEM_COUNT );
	resultSize += this->tupleElementStreamSize(
		static_cast< int >( output.length() ) );
	resultSize += this->tupleElementStreamSize( returnValueSize );

	resultStream.writePackedInt( resultSize );
	resultStream.writePackedInt( TUPLE_ITEM_COUNT );

	watcherValueToStream( resultStream, output, WT_READ_ONLY );

	return resultStream;
}


/**
 *	This method finalise the result stream from this CallableWatcher
 *
 *	@param	pathRequest	The WatcherPathRequestV2 given to ::setFromStream
 *	@param	base		The base pointer given to ::setFromStream
 */
void CallableWatcher::endResultStream( WatcherPathRequestV2 & pathRequest,
	const void * base )
{
	pathRequest.setResult( this->getComment(), WT_READ_ONLY, this, base );
}


/*
 *	This static method returns whether the path is the __args__ path.
 */
bool CallableWatcher::isArgsPath( const char * path )
{
	if (path == NULL)
		return false;

	return (strcmp(path, "__args__") == 0);
}


/**
 *	This static method returns whether the path is the __expose__ path.
 */
bool CallableWatcher::isExposePath( const char * path )
{
	if (path == NULL)
		return false;

	return (strcmp(path, "__expose__") == 0);
}


/*
 *	Override from Watcher.
 */
bool CallableWatcher::getAsStream( const void * base, const char * path,
	WatcherPathRequestV2 & pathRequest ) const
{
	if (isEmptyPath( path ))
	{
		Watcher::Mode mode = Watcher::WT_CALLABLE;
		BW::string value( "Callable function" );
		watcherValueToStream( pathRequest.getResultStream(),
			value, mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}
	else if (isDocPath( path ))
	{
		// <watcher>/__doc__ handling
		Watcher::Mode mode = Watcher::WT_READ_ONLY;
		watcherValueToStream( pathRequest.getResultStream(), comment_, mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}
	else if (this->isArgsPath( path ))
	{
		// <watcher>/__args__ handling
		size_t numArgs = argList_.size();
		Watcher::Mode mode = WT_CALLABLE;

		BinaryOStream & resultStream = pathRequest.getResultStream();

		resultStream << (uchar)WATCHER_TYPE_TUPLE;
		resultStream << (uchar)mode;

		// In order to make decoding of the stream re-useable, we throw the
		// size of the next segment (ie: the count and the tuple types) on
		// before the count of the number of tuple elements. It seems like
		// a tiny bit of bloat but makes the decoding code a lot easier.

		MemoryOStream tmpResult;

		// Add the number of arguments for the function
		MF_ASSERT( numArgs <= INT_MAX );
		tmpResult.writeStringLength( ( int )numArgs );

		// Now for each argument add the name / type
		for (size_t i = 0; i < numArgs; i++)
		{
			// tuple
			tmpResult << (uchar)WATCHER_TYPE_TUPLE;
			// mode
			tmpResult << (uchar)Watcher::WT_READ_ONLY;

			// Create the contents of the argument element tuple
			MemoryOStream argStream;
			// count = 2
			argStream.writeStringLength( 2 ); // Arg name + type
			watcherValueToStream( argStream, argList_[i].description(),
									Watcher::WT_READ_ONLY );
			watcherValueToStream( argStream, argList_[i].type(),
									Watcher::WT_READ_ONLY );

			// size
			tmpResult.writeStringLength( argStream.size() );
			tmpResult.addBlob( argStream.data(), argStream.size() );
		}

		// Prefix the entire argument description with the desc size.
		resultStream.writeStringLength( tmpResult.size() );
		resultStream.addBlob( tmpResult.data(), tmpResult.size() );
		pathRequest.setResult( comment_, mode, this, base );

		return true;
	}
	else if (this->isExposePath( path ))
	{
		Watcher::Mode mode = Watcher::WT_READ_ONLY;
		watcherValueToStream( pathRequest.getResultStream(),
				(int32)exposeHint_, mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
// Section: NoArgCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
NoArgCallableWatcher::NoArgCallableWatcher(
		ExposeHint hint, const char * comment ) :
	CallableWatcher( hint, comment )
{
}


/*
 *	Override from Watcher.
 */
bool NoArgCallableWatcher::setFromStream( void * base,
		const char * path,
		WatcherPathRequestV2 & pathRequest )
{
	BW::string value = "No value";
	BW::string output = "";

	BinaryIStream * pInput = pathRequest.getValueStream();

	if (!Watcher::isEmptyPath( path ) || !pInput || pInput->error())
	{
		ERROR_MSG( "NoArgCallableWatcher::setFromStream: "
				"Function Watcher called with bad path or stream.\n" );
		return false;
	}

	char typeChar;
	*pInput >> typeChar;

	const WatcherDataType type = (WatcherDataType) typeChar;

	if (type == WATCHER_TYPE_TUPLE)
	{
		/* int size = */ pInput->readPackedInt();
		int parameterCount = pInput->readPackedInt();
		if (pInput->error())
		{
			ERROR_MSG( "NoArgCallableWatcher::setFromStream: "
					"Function Watcher called with insufficient data.\n" );
			return false;
		}

		if (parameterCount != 0)
		{
			WARNING_MSG( "NoArgCallableWatcher::setFromStream: "
					"Function Watcher called with %d arguments, expecting 0\n.",
				parameterCount );
		}
	}
	else if (type == WATCHER_TYPE_STRING)
	{
		// Backwards compatibility - PyCommon before 2.6 sent
		// "" instead of an empty tuple for retiring apps.
		BW::string stringParam;
		if (!watcherStreamToValueType( *pInput, stringParam, type ))
		{
			ERROR_MSG( "NoArgCallableWatcher::setFromStream: "
					"Function Watcher called with insufficient data.\n" );
			return false;
		}

		if (!stringParam.empty())
		{
			WARNING_MSG( "NoArgCallableWatcher::setFromStream: "
					"Function Watcher called with non-tuple data as argument "
					"list: %s\n",
				stringParam.c_str() );
		}
	}
	else
	{
		WARNING_MSG( "NoArgCallableWatcher::setFromStream: "
				"Function Watcher called with non-tuple data as argument list. "
				"Received %d, wanted %d\n",
			type, WATCHER_TYPE_TUPLE );
	}

	pInput->finish();

	if (this->onCall( output, value ))
	{
		BinaryOStream & resultStream = this->startResultStream( pathRequest,
			output, static_cast< int >( value.size() ) );

		watcherValueToStream( resultStream, value, WT_READ_ONLY );

		this->endResultStream( pathRequest, base );

		return true;
	}
	else
	{
		WARNING_MSG( "NoArgCallableWatcher::setFromStream: "
				"Function call failed\n" );
		return false;
	}
}


// -----------------------------------------------------------------------------
// Section: NoArgFuncCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
NoArgFuncCallableWatcher::NoArgFuncCallableWatcher( Function func,
		ExposeHint hint, const char * comment ) :
	NoArgCallableWatcher( hint, comment ),
	func_( func )
{
}

bool NoArgFuncCallableWatcher::onCall(
		BW::string & output, BW::string & value )
{
	return (*func_)( output, value );
}


// -----------------------------------------------------------------------------
// Section: SimpleCallableWatcher
// -----------------------------------------------------------------------------
/**
 *	Constructor
 */
SimpleCallableWatcher::SimpleCallableWatcher(
		ExposeHint hint, const char * comment ) :
	CallableWatcher( hint, comment )
{
}


/**
 *	This method verifies that the watcher has been given a tuple and
 *	then provides the contents of that tuple to onCall.
 *
 *	Subclasses may override this if they need to intercept the stream
 *	(e.g., for backwards-compatiblity of a Set watcher being replaced
 *	with a single-argument Callable watcher) but must not disturb the
 *	valueStream before calling back into this version.
 */
bool SimpleCallableWatcher::setFromStream( void * base,
		const char * path,
		WatcherPathRequestV2 & pathRequest )
{
	BW::string output = "";
	BW::string value = "No value";

	BinaryIStream * pInput = pathRequest.getValueStream();

	if (!Watcher::isEmptyPath( path ) || !pInput || pInput->error())
	{
		ERROR_MSG( "SimpleCallableWatcher::setFromStream: "
				"Function Watcher called with bad path or stream.\n" );
		return false;
	}

	char type;
	*pInput >> type;

	int size = pInput->readPackedInt();
	int remaining = pInput->remainingLength();

	int parameterCount = pInput->readPackedInt();

	if (pInput->error() || remaining < size)
	{
		ERROR_MSG( "SimpleCallableWatcher::setFromStream: "
				"Function Watcher called with insufficient data.\n" );
		pInput->finish();
		return false;
	}

	if ((WatcherDataType)type != WATCHER_TYPE_TUPLE)
	{
		ERROR_MSG( "SimpleCallableWatcher::setFromStream: "
				"Function Watcher called with non-tuple data as argument list. "
				"Received %d, wanted %d\n",
			(WatcherDataType)type, WATCHER_TYPE_TUPLE );
		pInput->finish();
		return false;
	}

	if (this->onCall( output, value, parameterCount, *pInput ))
	{
		if (pInput->remainingLength() != 0)
		{
			WARNING_MSG( "SimpleCallableWatcher::setFromStream: "
					"Function call did not read all argument data\n" );
			pInput->finish();
		}

		BinaryOStream & resultStream = this->startResultStream( pathRequest,
			output, static_cast< int >( value.size() ) );

		watcherValueToStream( resultStream, value, WT_READ_ONLY );

		this->endResultStream( pathRequest, base );

		return true;
	}
	else
	{
		WARNING_MSG( "SimpleCallableWatcher::setFromStream: "
				"Function call failed\n" );
		pInput->finish();
		return false;
	}
}


// -----------------------------------------------------------------------------
// Section: SafeWatcher
// -----------------------------------------------------------------------------

SafeWatcher::SafeWatcher( WatcherPtr pWatcher, SimpleMutex & mutex ) :
	Watcher( pWatcher->getComment().c_str() ),
	pWatcher_( pWatcher ),
	mutex_( mutex ),
	grabCount_( 0 )
{ }


bool SafeWatcher::getAsString( const void * base, const char * path,
									  BW::string & result, BW::string & desc,
									  Watcher::Mode & mode ) const
{ 
	MutexHolder mh( *this );
	return pWatcher_->getAsString( base, path, result, desc, mode );
}

bool SafeWatcher::setFromString( void * base, const char * path,
										const char * valueStr )
{ 
	MutexHolder mh( *this );
	return pWatcher_->setFromString( base, path, valueStr );
}

bool SafeWatcher::getAsStream( const void * base, const char * path,
									  WatcherPathRequestV2 & pathRequest ) const
{ 
	MutexHolder mh( *this );
	return pWatcher_->getAsStream( base, path, pathRequest );
}

bool SafeWatcher::setFromStream( void * base, const char * path,
										WatcherPathRequestV2 & pathRequest )
{
	MutexHolder mh( *this );
	return pWatcher_->setFromStream( base, path, pathRequest );
}

bool SafeWatcher::visitChildren( const void * base, const char * path,
										WatcherPathRequest & pathRequest ) 
{
	MutexHolder mh( *this );
	return pWatcher_->visitChildren( base, path, pathRequest );
}


bool SafeWatcher::addChild( const char * path, WatcherPtr pChild,
								   void * withBase )
{ 
	MutexHolder mh( *this );
	return pWatcher_->addChild( path, pChild, withBase ); 
}


void SafeWatcher::grab() const
{
	// Make sure we are not reentrant
	if (grabCount_ == 0)
	{
		mutex_.grab();
	}
	++grabCount_;
}


void SafeWatcher::give() const
{
	MF_ASSERT( grabCount_ > 0 );

	--grabCount_;
	if (grabCount_ == 0)
	{
		mutex_.give();
	}
}

// -----------------------------------------------------------------------------
// Section: SafeWatcher
// -----------------------------------------------------------------------------

ReadWriteLockWatcher::ReadWriteLockWatcher( 
		WatcherPtr pWatcher, ReadWriteLock & readWriteLock ) :
	Watcher( pWatcher->getComment().c_str() ),
	pWatcher_( pWatcher ),
	readWriteLock_( readWriteLock )
{ }


bool ReadWriteLockWatcher::getAsString( const void * base, const char * path,
							  BW::string & result, BW::string & desc,
							  Watcher::Mode & mode ) const
{ 
	ReadWriteLock::ReadGuard rg( readWriteLock_ );
	return pWatcher_->getAsString( base, path, result, desc, mode );
}

bool ReadWriteLockWatcher::setFromString( void * base, const char * path,
								const char * valueStr )
{ 
	ReadWriteLock::WriteGuard wg( readWriteLock_ );
	return pWatcher_->setFromString( base, path, valueStr );
}

bool ReadWriteLockWatcher::getAsStream( const void * base, const char * path,
							  WatcherPathRequestV2 & pathRequest ) const
{ 
	ReadWriteLock::ReadGuard rg( readWriteLock_ );
	return pWatcher_->getAsStream( base, path, pathRequest );
}

bool ReadWriteLockWatcher::setFromStream( void * base, const char * path,
								WatcherPathRequestV2 & pathRequest )
{
	ReadWriteLock::WriteGuard wg( readWriteLock_ );
	return pWatcher_->setFromStream( base, path, pathRequest );
}

bool ReadWriteLockWatcher::visitChildren( const void * base, const char * path,
								WatcherPathRequest & pathRequest ) 
{
	ReadWriteLock::ReadGuard rg( readWriteLock_ );
	return pWatcher_->visitChildren( base, path, pathRequest );
}


bool ReadWriteLockWatcher::addChild( const char * path, WatcherPtr pChild,
						   void * withBase )
{ 
	ReadWriteLock::WriteGuard wg( readWriteLock_ );
	return pWatcher_->addChild( path, pChild, withBase ); 
}


// -----------------------------------------------------------------------------
// Section: DereferenceWatcher
// -----------------------------------------------------------------------------

DereferenceWatcher::DereferenceWatcher( WatcherPtr watcher, void * withBase ) :
	watcher_( watcher ),
	sb_( (uintptr)withBase )
{ }

bool DereferenceWatcher::getAsString( const void * base, const char * path,
									  BW::string & result, BW::string & desc,
									  Watcher::Mode & mode ) const
{ 
	uintptr baseAddress = base ? dereference( base ) : 0;
	return  baseAddress == 0 ? false :
			watcher_->getAsString( (void*)(sb_ + baseAddress), path, 
							   result, desc, mode ); 
}

bool DereferenceWatcher::setFromString( void * base, const char * path,
										const char * valueStr )
{ 
	uintptr baseAddress = base ? dereference( base ) : 0;
	return  baseAddress == 0 ? false :
		watcher_->setFromString( (void*)(sb_ + baseAddress), path, 
								 valueStr ); 
}

bool DereferenceWatcher::getAsStream( const void * base, const char * path,
									  WatcherPathRequestV2 & pathRequest ) const
{ 
	uintptr baseAddress = base ? dereference( base ) : 0;
	return  baseAddress == 0 ? false :
		watcher_->getAsStream( (void*)(sb_ + baseAddress), path, 
							   pathRequest ); 
}

bool DereferenceWatcher::setFromStream( void * base, const char * path,
										WatcherPathRequestV2 & pathRequest )
{
	uintptr baseAddress = base ? dereference( base ) : 0;
	return  baseAddress == 0 ? false :
		watcher_->setFromStream( (void*)(sb_ + baseAddress), path, 
								 pathRequest ); 
}

bool DereferenceWatcher::visitChildren( const void * base, const char * path,
										WatcherPathRequest & pathRequest ) 
{
	uintptr baseAddress = base ? dereference( base ) : 0;
	return  baseAddress == 0 ? false :
		watcher_->visitChildren( (void*)(sb_ + baseAddress), path, 
								 pathRequest ); 
}


bool DereferenceWatcher::addChild( const char * path, WatcherPtr pChild,
								   void * withBase )
{ 
	return watcher_->addChild( path, pChild, withBase ); 
}


// -----------------------------------------------------------------------------
// Section: AbsoluteWatcher
// -----------------------------------------------------------------------------


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
		/* override */
{
	return (base == NULL) ? false :
		pWatcher_->getAsString( (void*)(absoluteAddress_), path, result, desc,
			mode );
}


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::setFromString( void * base, const char * path,
		const char * valueStr ) /* override */
{
	return (base == NULL) ? false :
		pWatcher_->setFromString( (void*)(absoluteAddress_), path, valueStr );
}


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const /* override */
{
	return (base == NULL) ? false :
		pWatcher_->getAsStream( (void*)(absoluteAddress_), path, pathRequest );
}


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) /* override */
{
	return (base == NULL) ? false :
		pWatcher_->setFromStream( (void*)(absoluteAddress_), path,
			pathRequest );
}


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest ) /* override */
{
	return (base == NULL) ? false :
		pWatcher_->visitChildren( (void*)(absoluteAddress_), path, pathRequest );
}


/*
 *	Override from Watcher.
 */
bool AbsoluteWatcher::addChild( const char * path, WatcherPtr pChild,
		void * withBase ) /* override */
{
	return pWatcher_->addChild( path, pChild, withBase );
}



BW_END_NAMESPACE

#endif // ENABLE_WATCHERS


// watcher.cpp
