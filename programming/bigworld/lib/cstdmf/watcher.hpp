#ifndef WATCHER_HPP
#define WATCHER_HPP

#include "config.hpp"
#include "smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Watcher;
class WatcherPathRequest;
class WatcherPathRequestV2;
class WatcherPathRequestV1;
class DirectoryWatcher;
typedef SmartPointer<Watcher> WatcherPtr;
typedef SmartPointer<DirectoryWatcher> DirectoryWatcherPtr;

BW_END_NAMESPACE

#if ENABLE_WATCHERS

#include "binary_stream.hpp"
#include "bw_string.hpp"
#include "binary_stream.hpp"

#include <climits>

/// Declare watcher variables when CSTDMF is compiled as a DLL
#define DECLARE_WATCHER_DATA( name )				\
	Watcher * g_pRootWatcher = NULL;				\
	const char * g_pRootWatcherPath = name;			\
	DEFINE_WATCH_DEBUG_MESSAGE_PRIORITY_FUNC

BW_BEGIN_NAMESPACE

/**
 * @internal
 * Enumeration listing all valid data types that can be
 * sent and received via the watcher protocol.
 */
enum WatcherDataType {
	WATCHER_TYPE_UNKNOWN = 0,
	WATCHER_TYPE_INT,
	WATCHER_TYPE_UINT,
	WATCHER_TYPE_FLOAT,
	WATCHER_TYPE_BOOL,
	WATCHER_TYPE_STRING,	// 5
	WATCHER_TYPE_TUPLE,
	WATCHER_TYPE_TYPE

	// Potential type additions
	//WATCHER_TYPE_VECTOR2,
	//WATCHER_TYPE_VECTOR3,
	//WATCHER_TYPE_VECTOR4
};


BW_END_NAMESPACE

#include <stdio.h>
#include "cstdmf/bw_vector.hpp"
#include <sstream>

#include "cstdmf/stdmf.hpp"

#include "binary_stream.hpp"

BW_BEGIN_NAMESPACE


/**
 *	The character that separates values in a watcher path.
 *
 *	@ingroup WatcherModule
 */
const char WATCHER_SEPARATOR = '/';

class WatcherVisitor;

template <class VALUE_TYPE>
bool watcherStringToValue( const char * valueStr, VALUE_TYPE &value )
{
	BW::stringstream stream;
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1800)
	VALUE_TYPE tmp{};
#else
	VALUE_TYPE tmp = VALUE_TYPE();
#endif

	stream.write( valueStr, std::streamsize(strlen( valueStr )) );
	stream.seekg( 0, std::ios::beg );
	stream >> tmp;

	if (!stream.fail())
	{
		value = tmp;
	}

	return !stream.fail();
}

inline
bool watcherStringToValue( const char * valueStr, const char *& value )
{
	return false;
}

inline
bool watcherStringToValue( const char * valueStr, BW::string & value )
{
	value = valueStr;

	return true;
}

inline
bool watcherStringToValue( const char * valueStr, bool & value )
{
	if (bw_stricmp( valueStr, "true" ) == 0)
	{
		value = true;
	}
	else if (bw_stricmp( valueStr, "false" ) == 0)
	{
		value = false;
	}
	else
	{
		BW::stringstream stream;
		bool tmp = false;

		stream.write( valueStr, std::streamsize( strlen( valueStr )));
		stream.seekg( 0, std::ios::beg );
		stream >> tmp;

		if (!stream.fail())
		{
			value = tmp;
		}

		return !stream.fail();
	}

	return true;
}


/* Watcher Stream -> Value (for SET operations: protocol 2) */
bool watcherStreamToValueType( BinaryIStream & stream, BW::string & value,
								WatcherDataType type );

/* Fallback stream handler when other type handlers haven't worked */
template <class VALUE_TYPE>
bool watcherStreamToStringToValue( BinaryIStream & stream, VALUE_TYPE &value,
								   const WatcherDataType type )
{
	// If the type on stream isn't a string, pull off the blob and then bail
	if (type != WATCHER_TYPE_STRING)
	{
		int size;
		size = stream.readStringLength();

		if (stream.error())
			return false;

		// Read off size bytes and then return failure
		stream.retrieve( size );
		return false;
	}

	BW::string extractedValue;
	if (!watcherStreamToValueType(stream, extractedValue, WATCHER_TYPE_STRING))
	{
		// Not much we can do if the string wasn't successfully read
		return false;
	}

	// Now pass it off
	return watcherStringToValue( extractedValue.c_str(), value );
}

template <class VALUE_TYPE>
bool watcherStreamToValue( BinaryIStream & stream, VALUE_TYPE &value )
{
#if !defined( _XBOX360 )
	char type;
	stream >> type;

	// If it's a string, we can push off the handling to the old protocol
	if (type == WATCHER_TYPE_STRING)
	{
		return watcherStreamToStringToValue( stream, value,
											(WatcherDataType)type );
	}

	int size;
	size = stream.readStringLength();
	if (stream.error())
	{
		return false;
	}

	int sizeDiff = stream.remainingLength();
	stream >> value;
	sizeDiff -= stream.remainingLength();

	if ((stream.error()) || (sizeDiff != size))
	{
		return false;
	}
	else
	{
		return true;
	}
#else // _XBOX360
	return false;
#endif
}

/* stream extractor function for a bool */
inline
bool watcherStreamToValueType( BinaryIStream & stream, bool & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_BOOL)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	if (size != sizeof(bool))
	{
		// Read off size bytes and then return failure
		stream.retrieve( size );
		return false;
	}

	stream >> value;
	return !stream.error();
}

inline
bool watcherStreamToValue( BinaryIStream & stream, bool & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	return watcherStreamToValueType( stream, value, (WatcherDataType)type );
}


/* stream extractor function for a 32 bit int */
inline
bool watcherStreamToValueType( BinaryIStream & stream, int32 & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_INT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	if (size != sizeof(int32))
	{
		// TODO: should this really be downcasting?
		// -- we should only encounter this if a SET operation
		//    has sent an 8 byte WATCHER_TYPE_INT which will
		//    most likely mean the value is > int32 so downcasting
		//    will loose data.
		if (size != sizeof(int64))
		{
			// Read off size bytes and then return failure
			stream.retrieve( size );
			return false;
		}

		int64 tmpVal;
		stream >> tmpVal;
		value = (int)tmpVal;
	}
	else
	{
		stream >> value;
	}

	return !stream.error();
}

/* stream extractor function for a 64 bit int */
inline
bool watcherStreamToValueType( BinaryIStream & stream, int64 & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_INT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	// It should be safe to upcast
	if (size != sizeof(int64))
	{
		if (size != sizeof(int32))
		{
			// Read off size bytes and then return failure
			stream.retrieve( size );
			return false;
		}

		int32 tmpVal;
		stream >> tmpVal;
		value = (int64)tmpVal;
	}
	else
	{
		stream >> value;
	}

	return !stream.error();
}


inline
bool watcherStreamToValue( BinaryIStream & stream, int & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, (int32&)value,
								(WatcherDataType)type );
}


inline
bool watcherStreamToValue( BinaryIStream & stream, long & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, (int32&)value,
								(WatcherDataType)type );
}



/* stream extractor function for an unsigned 32 bit int */
inline
bool watcherStreamToValueType( BinaryIStream & stream, uint32 & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_UINT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	if (size != sizeof(uint32))
	{
		// Read off size bytes and then return failure
		stream.retrieve( size );
		return false;
	}

	stream >> value;
	return !stream.error();
}

/* stream extractor function for an unsigned 64 bit int */
inline
bool watcherStreamToValueType( BinaryIStream & stream, uint64 & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_UINT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	if (size != sizeof(uint64))
	{
		if (size != sizeof(uint32))
		{
			// Read off size bytes and then return failure
			stream.retrieve( size );
			return false;
		}

		uint32 tmpVal;
		stream >> tmpVal;
		value = (uint64)tmpVal;
	}

	stream >> value;
	return !stream.error();
}


inline
bool watcherStreamToValue( BinaryIStream & stream, unsigned int & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, (uint32&)value,
								(WatcherDataType)type );
}




/* stream extractor function for a float */
inline
bool watcherStreamToValueType( BinaryIStream & stream, float & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_FLOAT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	// Can attempt to downcast
	if (size != sizeof(float))
	{
		if (size != sizeof(double))
		{
			// Read off size bytes and then return failure
			stream.retrieve( size );
			return false;
		}

		double tmpVal;
		stream >> tmpVal;
		value = (float)tmpVal;
	}
	else
	{
		stream >> value;
	}

	return !stream.error();
}


/* stream extractor function for a double */
inline
bool watcherStreamToValueType( BinaryIStream & stream, double & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_FLOAT)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	int size;
	size = stream.readStringLength();

	// It should be safe to upcast
	if (size != sizeof(double))
	{
		if (size != sizeof(float))
		{
			// Read off size bytes and then return failure
			stream.retrieve( size );
			return false;
		}

		float tmpVal;
		stream >> tmpVal;
		value = (double)tmpVal;
	}
	else
	{
		stream >> value;
	}

	return !stream.error();
}


inline
bool watcherStreamToValue( BinaryIStream & stream, float & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, value, (WatcherDataType)type );
}


inline
bool watcherStreamToValue( BinaryIStream & stream, double & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, value, (WatcherDataType)type );
}




/* stream extractor function for a string */
inline
bool watcherStreamToValueType( BinaryIStream & stream, BW::string & value,
							WatcherDataType type )
{
	if (type != WATCHER_TYPE_STRING)
	{
		return watcherStreamToStringToValue( stream, value, type );
	}

	stream >> value;
	return !stream.error();
}

inline
bool watcherStreamToValue( BinaryIStream & stream, BW::string & value )
{
	char type;
	stream >> type;

	if (stream.error())
	{
		return false;
	}

	// Push the stream into the value now
	return watcherStreamToValueType( stream, value, (WatcherDataType)type );
}




/* Watcher Value -> String (for GET operations: protocol 1) */

template <class VALUE_TYPE>
BW::string watcherValueToString( const VALUE_TYPE & value )
{
	BW::stringstream stream;
	stream << value;

	return stream.str();
}

inline
BW::string watcherValueToString( const BW::string & value )
{
	return value;
}

inline
BW::string watcherValueToString( bool value )
{
	return value ? "true" : "false";
}


/**
 *	This class is the base class for all debug value watchers. It is part of the
 *	@ref WatcherModule.
 *
 *	@ingroup WatcherModule
 */
class Watcher : public SafeReferenceCount
{
public:

	/**
	 *	This enumeration is used to specify the type of the watcher.
	 */
	enum Mode
	{
		WT_INVALID,			///< Indicates an error.
		WT_READ_ONLY,		///< Indicates the watched value cannot be changed.
		WT_READ_WRITE,		///< Indicates the watched value can be changed.
		WT_DIRECTORY,		///< Indicates that the watcher has children.
		WT_CALLABLE			///< Indicates the watcher is a callable function
	};

	/// @name Construction/Destruction
	//@{
	/// Constructor.
	CSTDMF_DLL Watcher( const char * comment = "" );
	CSTDMF_DLL virtual ~Watcher();
	//@}

	/// @name Methods to be overridden
	//@{
	/**
	 *	This method is used to get a string representation of the value
	 *	associated with the path. The path is relative to this watcher.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the value associated with
	 *					this watcher is used.
	 *	@param result	A reference to a string where the resulting string will
	 *					be placed.
	 *	@param desc		A reference to a string that contains description of
	 *					this watcher.
	 *	@param mode		A reference to a watcher mode. It is set to the mode
	 *					of the returned value.
	 *
	 *	@return	True if successful, otherwise false.
	 */
	virtual bool getAsString( const void * base,
		const char * path,
		BW::string & result,
		BW::string & desc,
		Mode & mode ) const = 0;

	/**
	 *	This method is used to get a string representation of the value
	 *	associated with the path. The path is relative to this watcher.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the value associated with
	 *					this watcher is used.
	 *	@param result	A reference to a string where the resulting string will
	 *					be placed.
	 *	@param mode		A reference to a watcher mode. It is set to the mode
	 *					of the returned value.
	 *
	 *	@return	True if successful, otherwise false.
	 */

	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, Mode & mode ) const
	{
		BW::string desc;
		return this->getAsString( base, path, result, desc, mode );
	}

	/**
	 *	This method sets the value of the watcher associated with the input
	 *	path from a string. The path is relative to this watcher.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the value associated with
	 *					this watcher is set.
	 *	@param valueStr	A string representing the value which will be used to
	 *					set to watcher's value.
	 *
	 *	@return	True if the new value was set, otherwise false.
	 */
	virtual bool setFromString( void * base,
		const char * path,
		const char * valueStr )	= 0;

	/**
	 *	This method sets the value of the watcher associated with the input
	 *	path from a PathRequest. The Path Request is an abstraction of the
	 * data associated with watcher path being queried.
	 *
	 * NB: For asynchronous watcher set requests this method will always
	 * return success.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the value associated with
	 *					this watcher is set.
	 *	@param pathRequest	A reference to the PathRequest object to use
	 *					when setting the watcher's value.
	 *
	 *	@return	True if the new value was set, otherwise false.
	 */
	virtual bool setFromStream( void * base,
		const char * path,
		WatcherPathRequestV2 & pathRequest ) = 0;


	/**
	 *	This method visits each child of a directory watcher.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the children of this
	 *					object are visited.
	 *	@param pathRequest	The path request object to notify for each child
	 *					owned by the directory watcher.
	 *
	 *	@return True if the children were visited, false if specified watcher
	 *		could not be found or is not a directory watcher.
	 */
	virtual bool visitChildren( const void * base, const char *path,
			WatcherPathRequest & pathRequest ) { return false; }


	/**
	 *	This method visits each child of a directory watcher.
	 *
	 * NB: This method is a wrapper for the path request implementation of
	 * visitChildren.
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the children of this
	 *					object are visited.
	 *	@param visitor	An object derived from WatcherVisitor whose visit method
	 *					will be called for each child.
	 *
	 *	@return True if the children were visited, false if specified watcher
	 *		could not be found or is not a directory watcher.
	 */
	CSTDMF_DLL bool visitChildren( const void * base,
		const char * path,
		WatcherVisitor & visitor );


	/**
	 *	This method is used to get a stream representation of the value
	 *	associated with the path. The path is relative to this watcher.
	 *	The stream is comprised of
	 *	[data type] - (string/bool/int ...)
	 *	[data mode] - (read only/read write)
	 *	[data]
	 *
	 *	@param base		This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *	@param path		The path string used to find the desired watcher. If
	 *					the path is the empty string, the value associated with
	 *					this watcher is used.
	 *	@param pathRequest	A reference to a Watcher protocol v2 PathRequest
	 * 				containing the stream that we store the watcher value in.
	 *
	 *	@return	True if successful, otherwise false.
	 */
	virtual bool getAsStream( const void * base,
		const char * path, WatcherPathRequestV2 & pathRequest ) const = 0;



	/**
	 *	This method adds a watcher as a child to another watcher.
	 *
	 *	@param path		The path of the watcher to add the child to.
	 *	@param pChild	The watcher to add as a child.
	 *	@param withBase	This is a pointer used by a number of Watchers. It is
	 *					used as an offset into a struct or container.
	 *
	 *	@return True if successful, otherwise false. This method may fail if the
	 *			watcher could not be found or the watcher cannot have children.
	 */
	virtual bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL )
		{ return false; }

	virtual bool removeChild( const char * path )
		{ return false; }

	virtual WatcherPtr getChild( const char * path ) const
		{ return NULL; }

	void setComment( const BW::string & comment = BW::string( "" ) )
				{ comment_ = comment; }

	const BW::string & getComment() { return comment_; }
	//@}


	/// @name Static methods
	//@{
	CSTDMF_DLL static bool hasRootWatcherInternal();
	CSTDMF_DLL static Watcher & rootWatcherInternal();

	static bool hasRootWatcher();
	static Watcher & rootWatcher();

	static void partitionPath( const BW::string path,
			BW::string & resultingName,
			BW::string & resultingDirectory );

	//@}


	CSTDMF_DLL static void finiInternal();
	static void fini();

protected:
	/**
	 *	This method is a helper that returns whether or not the input path is
	 *	an empty path. An empty path is an empty string or a NULL string.
	 *
	 *	@param path	The path to test.
	 *
	 *	@return	True if the path is <i>empty</i>, otherwise false.
	 */
	static bool isEmptyPath( const char * path )
		{ return (path == NULL) || (*path == '\0'); }

	static bool isDocPath( const char * path )
	{
		if (path == NULL)
			return false;

		return (strcmp(path, "__doc__") == 0);
	}

	BW::string comment_;
};


/* ----- Implementations of protocol type streaming funcs ----- */
/* Watcher Value -> Stream (for GET operations: protocol 2) */
template <class ValueType>
void watcherValueToStream( BinaryOStream & result, const ValueType & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_STRING;
	result << (uchar)mode;
	result << watcherValueToString( value );
}

inline
void watcherValueToStream( BinaryOStream & result, WatcherDataType value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_TYPE;
	result << (uchar)mode;
	result.writeStringLength( sizeof(uchar) );
	result << (uchar)value;
}

inline
void watcherValueToStream( BinaryOStream & result, const int & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_INT;
	result << (uchar)mode;
	result.writeStringLength( sizeof(int) );
	result << value;
}

inline
void watcherValueToStream( BinaryOStream & result, const unsigned int & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_UINT;
	result << (uchar)mode;
	result.writeStringLength( sizeof(unsigned int) );
	result << value;
}

inline
void watcherValueToStream( BinaryOStream & result, const float & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_FLOAT;
	result << (uchar)mode;
	result.writeStringLength( sizeof(float) );
	result << value;
}

inline
void watcherValueToStream( BinaryOStream & result, const double & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_FLOAT;
	result << (uchar)mode;
	result.writeStringLength( sizeof(double) );
	result << value;
}

inline
void watcherValueToStream( BinaryOStream & result, const bool & value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_BOOL;
	result << (uchar)mode;
	result.writeStringLength( sizeof(bool) );
	result << value;
}


inline
void watcherValueToStream( BinaryOStream & result, const int64 value,
						   const Watcher::Mode & mode )
{
	result << (uchar)WATCHER_TYPE_INT;
	result << (uchar)mode;
	result.writeStringLength( sizeof(int64) );
	result << value;
}




/**
 *	This interface is used to implement a visitor. A visitor is an object that
 *	is used to <i>visit</i> each child of a directory watcher. It is part of the
 *	@ref WatcherModule.
 *
 * 	@see Watcher::visitChildren
 *
 * 	@ingroup WatcherModule
 */
class WatcherVisitor
{
public:
	/// Destructor.
	virtual ~WatcherVisitor() {};


	/**
	 *	This method is called once for each child of a directory watcher when
	 *	the visitChild member is called. This function can return false
	 * 	to stop any further visits.
	 *
	 *	@see Watcher::visitChildren
	 */
	virtual bool visit( Watcher::Mode mode,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & valueStr ) = 0;

	virtual bool visit( Watcher::Mode mode,
		const BW::string & label,
		const BW::string & valueStr )
	{
		BW::string desc;
		return this->visit( mode, label, desc, valueStr );
	}
};


/**
 *	This class implements a Watcher that can contain other Watchers. It is used
 *	by the watcher module to implement the tree of watchers. To find a watcher
 *	associated with a given path, you traverse this tree in a way similar to
 *	traversing a directory structure.
 *
 *	@see WatcherModule
 *
 *	@ingroup WatcherModule
 */
class DirectoryWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	CSTDMF_DLL DirectoryWatcher();
	CSTDMF_DLL virtual ~DirectoryWatcher();
	//@}

	/// @name Overrides from Watcher
	//@{
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const;

	virtual bool getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const;

	virtual bool setFromString( void * base, const char * path,
		const char * valueStr );

	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );

	virtual bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest );

	virtual bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL );

	virtual bool removeChild( const char * path );

	virtual WatcherPtr getChild( const char * path ) const;
	//@}

	/// @name Helper methods
	//@{
	CSTDMF_DLL static const char * tail( const char * path );
	//@}

private:
	struct DirData
	{
		WatcherPtr		watcher;
		void *			base;
		BW::string		label;
	};

	DirData * findChild( const char * path ) const;

	typedef BW::vector<DirData> Container;
	Container container_;
};

BW_END_NAMESPACE

#include "cstdmf/watcher_path_request.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used for watching the contents of vectors and other sequences.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
template <class SEQ> class SequenceWatcher : public Watcher
{
public:
	/// The type in the sequence.
	typedef typename SEQ::value_type SEQ_value;
	/// The type of a reference to an element in the sequence
	typedef typename SEQ::reference SEQ_reference;
	/// The type of the iterator to use.
	typedef typename SEQ::iterator SEQ_iterator;
	/// The type of the const iterator to use.
	typedef typename SEQ::const_iterator SEQ_constiterator;
	/// A function to convert an index into a string
	typedef BW::string (*SEQ_indexToString)( const void * base, int index );
	typedef int (*SEQ_stringToIndex)( const void * base,
									  const BW::string & name );
public:
	/// @name Construction/Destruction
	//@{
	/// Constructor.
	SequenceWatcher( SEQ & toWatch = *(SEQ*)NULL,
			const char ** labels = NULL ) :
		toWatch_( toWatch ),
		labels_( labels ),
		labelsub_( NULL ),
		child_( NULL ),
		subBase_( 0 ),
		indexToString_( NULL ),
		stringToIndex_( NULL )
	{
	}
	//@}

	/// @name Overrides from Watcher.
	//@{
	// Override from Watcher.
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
	{
		if (isEmptyPath(path))
		{
			result = "<DIR>";
			mode = WT_DIRECTORY;
			desc = comment_;
			return true;
		}

		try
		{
			SEQ_reference rChild = this->findChild( base, path );

			return child_->getAsString( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), result, desc, mode );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	// Override from Watcher
	virtual bool setFromString( void * base, const char * path,
		const char * valueStr )
	{
		try
		{
			SEQ_reference rChild = this->findChild( base, path );

			return child_->setFromString( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), valueStr );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	// Override from Watcher.
	virtual bool getAsStream( const void * base, const char * path,
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

		try
		{
			SEQ_reference rChild = this->findChild( base, path );

			return child_->getAsStream( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), pathRequest );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	// Override from Watcher
	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest )
	{
		try
		{
			SEQ_reference rChild = this->findChild( base, path );

			return child_->setFromStream( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), pathRequest );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	// Override from Watcher
	virtual bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest )
	{
		if (isEmptyPath(path))
		{
			SEQ	& useVector = *(SEQ*)(
				((uintptr)&toWatch_) + ((uintptr)base) );

			const char ** ppLabel = labels_;

			// Notify the request object of the sequence size
			MF_ASSERT( useVector.size() <= INT_MAX );
			pathRequest.addWatcherCount( ( int32 ) useVector.size() );

			int count = 0;
			for( SEQ_iterator iter = useVector.begin();
				iter != useVector.end();
				iter++, count++ )
			{
				BW::string	callLabel;
				BW::string desc;

				SEQ_reference rChild = *iter;

				if ((ppLabel != NULL) && (*ppLabel != NULL))
				{
					callLabel.assign( *ppLabel );
					ppLabel++;
				}
				else if (labelsub_)
				{
					Watcher::Mode unused;
					child_->getAsString( (void*)(subBase_ + (uintptr)&rChild),
						labelsub_, callLabel, desc, unused );
				}
				else if (indexToString_)
				{
					callLabel.assign( indexToString_( base, count ) );
				}
				if (callLabel.empty())
				{
					char temp[32];
					bw_snprintf( temp, sizeof(temp), "%u", count );
					callLabel.assign( temp );
				}

				// Add the current sequence element to the request handler
				if (!pathRequest.addWatcherPath( (void*)(subBase_ + (uintptr)&rChild),
										  this->tail( path ), callLabel, *child_ ))
				{
					break;
				}
			}

			return true;
		}
		else
		{
			try
			{
				SEQ_reference rChild = this->findChild( base, path );

				return child_->visitChildren(
					(void*)(subBase_ + (uintptr)&rChild),
					this->tail( path ), pathRequest );
			}
			catch (NoSuchChild &)
			{
				return false;
			}
		}
	}

	// Override from Watcher
	virtual bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL )
	{
		if (isEmptyPath( path ))
		{
			return false;
		}
		else if (strchr(path,'/') == NULL)
		{
			// we're a one-child family here
			if (child_ != NULL) return false;

			// ok, add away
			child_ = pChild;
			subBase_ = (uintptr)withBase;
			return true;
		}
		else
		{
			// we don't/can't check that our component of 'path'
			// exists here either.
			return child_->addChild( this->tail ( path ),
				pChild, withBase );
		}
	}
	//@}


	/// @name Helper methods
	//@{
	/**
	 *	This method sets the labels that are associated with the elements of
	 *	sequence that is being watched.
	 */
	void setLabels( const char ** labels )
	{
		labels_ = labels;
	}

	/**
	 *	This method sets the label sub-path associated with this object.
	 *
	 *	@todo More comments
	 */
	void setLabelSubPath( const char * subpath )
	{
		labelsub_ = subpath;
	}
	//@}

	/**
	 *	This method provides custom converter functions from an index to a
	 *	string and back again.
	 *
	 *	@todo More comments
	 */
	void setStringIndexConverter( SEQ_stringToIndex stringToIndex,
								  SEQ_indexToString indexToString )
	{
		indexToString_ = indexToString;
		stringToIndex_ = stringToIndex;
	}
	//@}

	/**
	 *	@see DirectoryWatcher::tail
	 */
	static const char * tail( const char * path )
	{
		return DirectoryWatcher::tail( path );
	}

private:

	class NoSuchChild
	{
	public:
		NoSuchChild( const char * path ): path_( path ) {}

		const char * path_;
	};

	SEQ_reference findChild( const void * base, const char * path ) const
	{
		SEQ	& useVector = *(SEQ*)(
			((const uintptr)&toWatch_) + ((const uintptr)base) );

		char * pSep = strchr( (char*)path, WATCHER_SEPARATOR );
		BW::string lookingFor = pSep ?
			BW::string( path, pSep - path ) : BW::string ( path );

		// see if it matches a string
		size_t	cLabels = 0;
		while (labels_ && labels_[cLabels] != NULL) cLabels++;

		size_t	lIter = 0;
		size_t	ende = std::min( cLabels, useVector.size() );
		SEQ_iterator	sIter = useVector.begin();
		while (lIter < ende)
		{
			if (!strcmp( labels_[lIter], lookingFor.c_str() ))
				return *sIter;

			lIter++;
			sIter++;
		}

		// try the label sub path if we have one
		if (labelsub_ != NULL)
		{
			for (sIter = useVector.begin(); sIter != useVector.end(); sIter++)
			{
				BW::string	tri;
				BW::string	desc;

				Watcher::Mode unused;
				SEQ_reference ref = *sIter;
				child_->getAsString( (void*)(subBase_ + (uintptr)&ref),
					labelsub_, tri, desc, unused );

				if (tri == lookingFor) return *sIter;
			}
		}

		if (stringToIndex_ != NULL)
		{
			// run it through a custom string to int converter
			lIter = stringToIndex_( base, lookingFor );

			if (lIter == (size_t)-1)
			{
				throw NoSuchChild( path );
			}
		}
		else
		{
			// now try it as a number
			lIter = atoi( lookingFor.c_str() );
		}
		for (sIter = useVector.begin(); sIter != useVector.end() && lIter > 0;
			sIter++) lIter--; // scan

		if (sIter != useVector.end())
			return *sIter;

		// don't have it, give up then
		throw NoSuchChild( path );
	}

	SEQ &			toWatch_;
	const char **	labels_;
	const char *	labelsub_;
	WatcherPtr		child_;
	uintptr			subBase_;

	SEQ_indexToString indexToString_;
	SEQ_stringToIndex stringToIndex_;
};


template< class Key >
class IMapKeyStringConverter
{
public:
	virtual ~IMapKeyStringConverter() {}

	virtual bool stringToValue( const BW::string & keyString, Key & key ) = 0;
	virtual const BW::string valueToString( const Key & key ) = 0;
};


/**
 *	This class is used for watching the contents of maps.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */

template <class MAP> class MapWatcher : public Watcher
{
private:
	typedef typename MapTypes<MAP>::_Tref MAP_reference;
	typedef typename MAP::key_type MAP_key;
	typedef typename MAP::iterator MAP_iterator;
	typedef typename MAP::const_iterator MAP_constiterator;
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 *
	 *	@param toWatch	Specifies the map to watch.
	 */
	MapWatcher( MAP & toWatch = *(MAP*)NULL, 
			IMapKeyStringConverter< MAP_key > * pKeyStringConverter = NULL ) :
		toWatch_( toWatch ),
		pKeyStringConverter_( pKeyStringConverter ),
		child_( NULL ),
		subBase_( 0 )
	{
	}
	//@}

	/// @name Overrides from Watcher.
	//@{
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
	{
		if (isEmptyPath(path))
		{
			result = "<DIR>";
			mode = WT_DIRECTORY;
			desc = comment_;
			return true;
		}

		try
		{
			MAP_reference rChild = this->findChild( base, path );

			return child_->getAsString( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), result, desc, mode );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	virtual bool setFromString( void * base, const char * path,
		const char * valueStr )
	{
		try
		{
			MAP_reference rChild = this->findChild( base, path );

			return child_->setFromString( (void*)(subBase_ + (uintptr)&rChild),
			this->tail( path ), valueStr );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	virtual bool getAsStream( const void * base, const char * path,
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

		try
		{
			MAP_reference rChild = this->findChild( base, path );

			return child_->getAsStream( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), pathRequest );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}

	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest )
	{
		try
		{
			MAP_reference rChild = this->findChild( base, path );

			return child_->setFromStream( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), pathRequest );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}


	virtual bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest )
	{
		if (isEmptyPath(path))
		{
			MAP	& useMap = *(MAP*)( ((uintptr)&toWatch_) + ((uintptr)base) );

			// Notify the path request of the map element count
			MF_ASSERT( useMap.size() <= INT_MAX );
			pathRequest.addWatcherCount( ( int )useMap.size() );

			int count = 0;
			MAP_iterator iter = useMap.begin();
			while (iter != useMap.end())
			{
				BW::string callLabel( this->keyToString( (*iter).first ) );

				MAP_reference reference = (*iter).second;
				uintptr offset = uintptr( &reference );

				if (!pathRequest.addWatcherPath(
						(void*)(subBase_ + offset ),
						this->tail( path ), callLabel, *child_ ))
				{
					break;
				}

				iter++;
				count++;
			}

			return true;
		}
		else
		{
			try
			{
				MAP_reference rChild = this->findChild( base, path );

				return child_->visitChildren(
					(void*)(subBase_ + (uintptr)&rChild),
					this->tail( path ), pathRequest );
			}
			catch (NoSuchChild &)
			{
				return false;
			}
		}
	}


	virtual bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL )
	{
		if (isEmptyPath( path ))
		{
			return false;
		}
		else if (strchr(path,'/') == NULL)
		{
			// we're a one-child family here
			if (child_ != NULL) return false;

			// ok, add away
			child_ = pChild;
			subBase_ = (uintptr)withBase;
			return true;
		}
		else
		{
			// we don't/can't check that our component of 'path'
			// exists here either.
			return child_->addChild( this->tail ( path ),
				pChild, withBase );
		}
	}
	//@}

	/**
	 *	This function is a helper. It strips the path from the input string and
	 *	returns a string containing the tail value.
	 *
	 *	@see DirectoryWatcher::tail
	 */
	static const char * tail( const char * path )
	{
		return DirectoryWatcher::tail( path );
	}

private:
	const BW::string keyToString( const MAP_key & key ) const
	{
		if (pKeyStringConverter_)
		{
			return pKeyStringConverter_->valueToString( key );
		}
		else
		{
			return watcherValueToString( key );
		}
	}


	bool stringToKey( const BW::string & string, MAP_key & out ) const
	{
		if (pKeyStringConverter_)
		{
			return pKeyStringConverter_->stringToValue( string, out );
		}
		else
		{
			return watcherStringToValue( string.c_str(), out );
		}
	}


	class NoSuchChild
	{
	public:
		NoSuchChild( const char * path ): path_( path ) {}

		const char * path_;
	};

	MAP_reference findChild( const void * base, const char * path ) const
	{
		MAP	& useMap = *(MAP*)(
			((const uintptr)&toWatch_) + ((const uintptr)base) );

		// find out what we're looking for
		char * pSep = strchr( (char*)path, WATCHER_SEPARATOR );
		BW::string lookingFor = pSep ?
			BW::string( path, pSep - path ) : BW::string ( path );

		MAP_key key;

		if (this->stringToKey( lookingFor, key ))
		{
			// see if that's in there anywhere
			MAP_iterator iter = useMap.find( key );
			if (iter != useMap.end())
			{
				return (*iter).second;
			}
		}

		// don't have it, give up then
		throw NoSuchChild( path );
	}

	MAP &		toWatch_;
	IMapKeyStringConverter< MAP_key > * pKeyStringConverter_;
	WatcherPtr	child_;
	uintptr		subBase_;
};


/**
 *	This class is a helper watcher that wraps another Watcher. It consumes no
 *	path and is only responsible for grabbing and giving a mutex when calling
 *	through to the wrapped Watcher.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */

class SafeWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	SafeWatcher( WatcherPtr watcher, SimpleMutex & mutex );
	//@}

	/// @name Overrides from Watcher.
	//@{
	// Override from Watcher
	virtual bool getAsString( const void * base, const char * path,
							  BW::string & result, BW::string & desc,
							  Watcher::Mode & mode ) const;

	// Override from Watcher
	virtual bool setFromString( void * base, const char * path,
								const char * valueStr );

	// Override from Watcher
	virtual bool getAsStream( const void * base, const char * path,
							  WatcherPathRequestV2 & pathRequest ) const;

	// Override from Watcher
	virtual bool setFromStream( void * base, const char * path,
								WatcherPathRequestV2 & pathRequest );

	// Override from Watcher
	virtual bool visitChildren( const void * base, const char * path,
								WatcherPathRequest & pathRequest );

	// Override from Watcher
	virtual bool addChild( const char * path, WatcherPtr pChild,
								void * withBase = NULL );
	//@}

private:

	class MutexHolder
	{
	public:
		MutexHolder( const SafeWatcher & safeWatcher ):
			safeWatcher_( safeWatcher )
		{
			safeWatcher_.grab();
		}

		~MutexHolder()
		{
			safeWatcher_.give();
		}

	private:
		const SafeWatcher & safeWatcher_;
	};

	void grab() const;
	void give() const;

	WatcherPtr pWatcher_;
	SimpleMutex & mutex_;
	mutable int grabCount_;
};


/**
 *	This class is a helper watcher that wraps another Watcher. It consumes no
 *	path and is only responsible for handling a ReadWriteLock when calling
 *	through to the wrapped Watcher.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */

class ReadWriteLockWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	ReadWriteLockWatcher ( WatcherPtr watcher, ReadWriteLock & readWriteLock );
	//@}

	/// @name Overrides from Watcher.
	//@{
	// Override from Watcher
	virtual bool getAsString( const void * base, const char * path,
							  BW::string & result, BW::string & desc,
							  Watcher::Mode & mode ) const;

	// Override from Watcher
	virtual bool setFromString( void * base, const char * path,
								const char * valueStr );

	// Override from Watcher
	virtual bool getAsStream( const void * base, const char * path,
							  WatcherPathRequestV2 & pathRequest ) const;

	// Override from Watcher
	virtual bool setFromStream( void * base, const char * path,
								WatcherPathRequestV2 & pathRequest );

	// Override from Watcher
	virtual bool visitChildren( const void * base, const char * path,
								WatcherPathRequest & pathRequest );

	// Override from Watcher
	virtual bool addChild( const char * path, WatcherPtr pChild,
								void * withBase = NULL );
	//@}

private:
	WatcherPtr pWatcher_;
	ReadWriteLock & readWriteLock_;
};


/**
 *	This abstract class from which watchers meant to access indirect bases
 *	derive.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
class DereferenceWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	CSTDMF_DLL DereferenceWatcher( WatcherPtr watcher, void * withBase = NULL );
	//@}

	/// @name Overrides from Watcher.
	//@{
	// Override from Watcher
	CSTDMF_DLL virtual bool getAsString( const void * base, const char * path,
							  BW::string & result, BW::string & desc,
							  Watcher::Mode & mode ) const;

	// Override from Watcher
	CSTDMF_DLL virtual bool setFromString( void * base, const char * path,
								const char * valueStr );

	// Override from Watcher
	CSTDMF_DLL virtual bool getAsStream( const void * base, const char * path,
							  WatcherPathRequestV2 & pathRequest ) const;

	// Override from Watcher
	CSTDMF_DLL virtual bool setFromStream( void * base, const char * path,
								WatcherPathRequestV2 & pathRequest );

	// Override from Watcher
	CSTDMF_DLL virtual bool visitChildren( const void * base, const char * path,
								WatcherPathRequest & pathRequest );

	// Override from Watcher
	CSTDMF_DLL virtual bool addChild( const char * path, WatcherPtr pChild,
						   void * withBase = NULL );
	// this base is passed through unchanged, because it is interpreted
	// deeper in the hierarchy than we are. All the bases we dereference are
	// passed to us by an owning Watcher (e.g. VectorWatcher), or the user.
	//@}
protected:
	WatcherPtr	watcher_;
	uintptr		sb_;

	virtual uintptr dereference(const void *) const = 0;
};


/**
 *	This class is a helper watcher that wraps another Watcher. It consumes no
 *	path and is only responsible for dereferencing base pointer and passing it
 *	through to the wrapped Watcher (except for the one in addChild, of course).
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
class BaseDereferenceWatcher : public DereferenceWatcher
{
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	BaseDereferenceWatcher( WatcherPtr watcher, void * withBase = NULL ) :
		DereferenceWatcher( watcher, withBase )
	{ }
	//@}
protected:
	// Override from DereferenceWatcher
	//@{
	uintptr dereference(const void * base) const
	{
		return *(uintptr*)base;
	}
	//@}
};

/**
 *	This class is a helper watcher that wraps another Watcher. It consumes no
 *	path and is only responsible for dereferencing base pointer (as if it were
 *	a smart pointer and passing it through to the wrapped Watcher.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
class SmartPointerDereferenceWatcher : public DereferenceWatcher
{
	typedef SmartPointer<uint> Pointer;
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	SmartPointerDereferenceWatcher( WatcherPtr watcher, void * withBase = NULL ) :
		DereferenceWatcher( watcher, withBase )
	{ }
	//@}

protected:
	// Override from DereferenceWatcher
	//@{
	uintptr dereference(const void * base) const
	{
		return (uintptr)((Pointer*)base)->get();
	}
	//@}
};

/**
 *	This class is a helper watcher that wraps another Watcher. It consumes no
 *	path and is only responsible for dereferencing base pointer (as if it were
 *	a key in another container, substituting it into the container and passing
 *	it through to the wrapped Watcher.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */

template <class CONTAINER_TYPE, class KEY_TYPE >
class ContainerBounceWatcher : public DereferenceWatcher
{
	CONTAINER_TYPE * indirection_;
public:
	/// @name Construction/Destruction
	//@{
	/**
	 *	Constructor.
	 */
	ContainerBounceWatcher( WatcherPtr watcher,
							CONTAINER_TYPE * indirection,
							void * withBase = NULL ) :
		DereferenceWatcher( watcher, withBase ),
		indirection_( indirection )
	{ }
	//@}

	void changeContainer( CONTAINER_TYPE * indirection )
	{
		MF_ASSERT( indirection != NULL );
		indirection_ = indirection;
	}

protected:
	// Override from DereferenceWatcher
	//@{
	uintptr dereference(const void * base) const
	{
		MF_ASSERT( indirection_ != NULL );
		MF_ASSERT( base != NULL );
		return (uintptr)&(*indirection_)[*(KEY_TYPE *)base];
	}
	//@}
};


// -----------------------------------------------------------------------------
// Section: MemberWatcher
// -----------------------------------------------------------------------------


template <class RETURN_TYPE,
	class OBJECT_TYPE >
class ReadOnlyMemberWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{

	/**
		*	Constructor.
		*
		*	This constructor should only be called from the Debug::addValue
		*	function.
		*/
	ReadOnlyMemberWatcher( RETURN_TYPE (OBJECT_TYPE::*getMethod)() const ) :
		pObject_( NULL ),
		getMethod_( getMethod )
	{}

	/**
		*	Constructor.
		*
		*	This constructor should only be called from the Debug::addValue
		*	function.
		*/
	ReadOnlyMemberWatcher( OBJECT_TYPE & rObject,
			RETURN_TYPE (OBJECT_TYPE::*getMethod)() const ) :
		pObject_( &rObject ),
		getMethod_( getMethod )
	{}

	//@}

protected:
	/// @name Overrides from Watcher.
	//@{
	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
	{
		if (!isEmptyPath( path )) return false;

		if (getMethod_ == (GetMethodType)NULL)
		{
			result = "";
			mode = WT_READ_ONLY;
			desc = comment_;
			return true;
		}

		const OBJECT_TYPE & useObject = *(OBJECT_TYPE*)(
			((const uintptr)pObject_) + ((const uintptr)base) );

		RETURN_TYPE value = (useObject.*getMethod_)();

		result = watcherValueToString( value );
		desc = comment_;

		mode = WT_READ_ONLY;
		return true;
	};

	virtual bool getAsStream( const void * base, const char * path,
			WatcherPathRequestV2 & pathRequest ) const
	{
		if (isEmptyPath( path ))
		{
			if (getMethod_ == (GetMethodType)NULL)
			{
				Watcher::Mode mode = WT_READ_ONLY;
				watcherValueToStream( pathRequest.getResultStream() , "", mode );
				pathRequest.setResult( comment_, mode, this, base );
				return true;
			}

			const OBJECT_TYPE & useObject = *(OBJECT_TYPE*)(
				((const uintptr)pObject_) + ((const uintptr)base) );

			RETURN_TYPE value = (useObject.*getMethod_)();

			Watcher::Mode mode = WT_READ_ONLY;

			// push the result into the reply stream
			watcherValueToStream( pathRequest.getResultStream() , value, mode );
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

		return false;
	}


	virtual bool setFromString( void * base, const char *path,
		const char * valueStr )
	{
		return false;
	}


	virtual bool setFromStream( void * base, const char *path,
		WatcherPathRequestV2 & pathRequest )
	{
		return false;
	}
	//@}

private:

	typedef RETURN_TYPE (OBJECT_TYPE::*GetMethodType)() const;

	OBJECT_TYPE * pObject_;
	RETURN_TYPE (OBJECT_TYPE::*getMethod_)() const;
};


/**
 *	This templatised class is used to store debug values that are a member of a
 *	class.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE,
	class OBJECT_TYPE,
	class CONSTRUCTION_TYPE = RETURN_TYPE>
class MemberWatcher : public Watcher
{
	public:
		/// @name Construction/Destruction
		//@{

		/**
		 *	Constructor.
		 *
		 *	This constructor should only be called from the Debug::addValue
		 *	function.
		 */
		MemberWatcher( RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
				void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ) = NULL ) :
			pObject_( NULL ),
			getMethod_( getMethod ),
			setMethod_( setMethod )
		{}

		/**
		 *	Constructor.
		 *
		 *	This constructor should only be called from the Debug::addValue
		 *	function.
		 */
		MemberWatcher( OBJECT_TYPE & rObject,
				RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
				void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ) = NULL ) :
			pObject_( &rObject ),
			getMethod_( getMethod ),
			setMethod_( setMethod )
		{}

		//@}

	protected:
		/// @name Overrides from Watcher.
		//@{
		virtual bool getAsString( const void * base, const char * path,
			BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
		{
			if (!isEmptyPath( path )) return false;

			if (getMethod_ == (GetMethodType)NULL)
			{
				result = "";
				mode = WT_READ_ONLY;
				desc = comment_;
				return true;
			}

			const OBJECT_TYPE & useObject = *(OBJECT_TYPE*)(
				((const uintptr)pObject_) + ((const uintptr)base) );

			RETURN_TYPE value = (useObject.*getMethod_)();

			result = watcherValueToString( value );
			desc = comment_;

			mode = (setMethod_ != (SetMethodType)NULL) ?
				WT_READ_WRITE : WT_READ_ONLY;
			return true;
		};

		virtual bool getAsStream( const void * base, const char * path,
				WatcherPathRequestV2 & pathRequest ) const
		{
			if (isEmptyPath( path ))
			{
				if (getMethod_ == (GetMethodType)NULL)
				{
					Watcher::Mode mode = WT_READ_ONLY;
					watcherValueToStream( pathRequest.getResultStream() , "", mode );
					pathRequest.setResult( comment_, mode, this, base );
					return true;
				}

				const OBJECT_TYPE & useObject = *(OBJECT_TYPE*)(
					((const uintptr)pObject_) + ((const uintptr)base) );

				RETURN_TYPE value = (useObject.*getMethod_)();

				Watcher::Mode mode = (setMethod_ != (SetMethodType)NULL) ?
						 WT_READ_WRITE : WT_READ_ONLY;

				// push the result into the reply stream
				watcherValueToStream( pathRequest.getResultStream() , value, mode );
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

			return false;
		}


		virtual bool setFromString( void * base, const char *path,
			const char * valueStr )
		{
			if (!isEmptyPath( path ) || (setMethod_ == (SetMethodType)NULL))
				return false;

			bool wasSet = false;

			CONSTRUCTION_TYPE value = CONSTRUCTION_TYPE();
			if (watcherStringToValue( valueStr, value ))
			{
				OBJECT_TYPE & useObject =
					*(OBJECT_TYPE*)( ((uintptr)pObject_) + ((uintptr)base) );
				(useObject.*setMethod_)( value );
				wasSet = true;
			}

			return wasSet;
		}


		virtual bool setFromStream( void * base, const char *path,
			WatcherPathRequestV2 & pathRequest )
		{
			if (!isEmptyPath( path ) || (setMethod_ == (SetMethodType)NULL))
				return false;

			bool wasSet = false;

			CONSTRUCTION_TYPE value = CONSTRUCTION_TYPE();
			BinaryIStream *istream = pathRequest.getValueStream();
			if (istream && watcherStreamToValue( *istream, value ))
			{
				OBJECT_TYPE & useObject =
					*(OBJECT_TYPE*)( ((uintptr)pObject_) + ((uintptr)base) );
				(useObject.*setMethod_)( value );
				wasSet = true;

				// push the result into the reply stream
				// assuming mode RW because we have already set
				Watcher::Mode mode = Watcher::WT_READ_WRITE;
				watcherValueToStream( pathRequest.getResultStream(), value, mode );
				pathRequest.setResult( comment_, mode, this, base );
			}

			return wasSet;
		}
		//@}

	private:

		typedef RETURN_TYPE (OBJECT_TYPE::*GetMethodType)() const;
		typedef void (OBJECT_TYPE::*SetMethodType)( RETURN_TYPE);

		OBJECT_TYPE * pObject_;
		RETURN_TYPE (OBJECT_TYPE::*getMethod_)() const;
		void (OBJECT_TYPE::*setMethod_)( RETURN_TYPE );
};


// -----------------------------------------------------------------------------
// Section: MemberWatcher - makeWatcher without object
// -----------------------------------------------------------------------------

/**
 *	This function is used to create a read-only watcher using a get method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const )
{
	return new MemberWatcher<const RETURN_TYPE &, OBJECT_TYPE, RETURN_TYPE>(
			getMethod, NULL );
}


/**
 *	This function is used to create a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( const RETURN_TYPE & ) )
{
	return new MemberWatcher<const RETURN_TYPE &, OBJECT_TYPE, RETURN_TYPE>(
			getMethod, setMethod );
}


/**
 *	This function is used to create a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ) )
{
	return new MemberWatcher<RETURN_TYPE, OBJECT_TYPE, RETURN_TYPE>(
			getMethod, setMethod );
}


/**
 *	This function is used to create a read-only watcher using a get method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( RETURN_TYPE (OBJECT_TYPE::*getMethod)() const )
{
	return new MemberWatcher<RETURN_TYPE, OBJECT_TYPE, RETURN_TYPE>(
				getMethod,
				static_cast< void (OBJECT_TYPE::*)( RETURN_TYPE ) >( NULL ) );
}


// VC++ 8 has problems disambiguating this and the const & version so it cannot
// be called makeWatcher
/**
 *	This function is used to create a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeNonRefWatcher( RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ),
						const char * comment = NULL	)
{
	return new MemberWatcher<RETURN_TYPE, OBJECT_TYPE>(
			getMethod, setMethod );
}


// -----------------------------------------------------------------------------
// Section: MemberWatcher - makeWatcher with object
// -----------------------------------------------------------------------------

/**
 *	This function is used to create a read-only watcher using a get method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( OBJECT_TYPE & rObject,
			const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const )
{
	return new MemberWatcher<const RETURN_TYPE &, OBJECT_TYPE, RETURN_TYPE>(
			rObject, getMethod, NULL );
}


/**
 *	This function is used to create a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher( OBJECT_TYPE & rObject,
						const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( const RETURN_TYPE & ) )
{
	return new MemberWatcher<const RETURN_TYPE &, OBJECT_TYPE, RETURN_TYPE>(
			rObject, getMethod, setMethod );
}


/**
 *	This function is used to create a read-only watcher using a get method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeWatcher(	OBJECT_TYPE & rObject,
						RETURN_TYPE (OBJECT_TYPE::*getMethod)() const )
{
	return new MemberWatcher<RETURN_TYPE, OBJECT_TYPE, RETURN_TYPE>(
				rObject,
				getMethod,
				static_cast< void (OBJECT_TYPE::*)( RETURN_TYPE ) >( NULL ) );
}


// VC++ 8 has problems disambiguating this and the const & version so it cannot
// be called makeWatcher
/**
 *	This function is used to create a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr makeNonRefWatcher( OBJECT_TYPE & rObject,
						RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ),
						const char * comment = NULL	)
{
	return new MemberWatcher<RETURN_TYPE, OBJECT_TYPE>(
			rObject, getMethod, setMethod );
}


// -----------------------------------------------------------------------------
// Section: DataWatcher
// -----------------------------------------------------------------------------

/**
 *	This templatised class is used to watch a given bit a data.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
template <class TYPE>
class DataWatcher : public Watcher
{
	public:
		/// @name Construction/Destruction
		//@{
		/**
		 *	Constructor. Leave 'path' as NULL to prevent the watcher adding
		 *	itself to the root watcher. In the normal case, the MF_WATCH style
		 *	macros should be used, as they do error checking and this
		 *	constructor cannot (it's certainly not going to delete itself
		 *	anyway).
		 */
		explicit DataWatcher( TYPE & rValue,
				Watcher::Mode access = Watcher::WT_READ_ONLY,
				const char * path = NULL ) :
			rValue_( rValue ),
			access_( access )
		{
			if (access_ != WT_READ_WRITE && access_ != WT_READ_ONLY)
				access_ = WT_READ_ONLY;

			if (path != NULL)
				Watcher::rootWatcher().addChild( path, this );
		}
		//@}

	protected:
		/// @name Overrides from Watcher.
		//@{
		// Override from Watcher.
		virtual bool getAsString( const void * base, const char * path,
			BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
		{
			if (isEmptyPath( path ))
			{
				const TYPE & useValue = *(const TYPE*)(
					((const uintptr)&rValue_) + ((const uintptr)base) );

				result = watcherValueToString( useValue );
				desc = comment_;
				mode = access_;
				return true;
			}
			else
			{
				return false;
			}
		};

		// Override from Watcher.
		virtual bool setFromString( void * base, const char * path,
			const char * valueStr )
		{
			if (isEmptyPath( path ) && (access_ == WT_READ_WRITE))
			{
				TYPE & useValue =
					*(TYPE*)( ((uintptr)&rValue_) + ((uintptr)base) );

				return watcherStringToValue( valueStr, useValue );
			}
			else
			{
				return false;
			}
		}

		// Override from Watcher.
		virtual bool getAsStream( const void * base, const char * path,
			WatcherPathRequestV2 & pathRequest ) const
		{
			if (isEmptyPath( path ))
			{
				const TYPE & useValue = *(const TYPE*)(
					((const uintptr)&rValue_) + ((const uintptr)base) );

				Watcher::Mode mode = access_;
				watcherValueToStream( pathRequest.getResultStream(), useValue, mode );
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
				return false;
			}
		}

		// Override from Watcher.
		virtual bool setFromStream( void * base, const char * path,
			WatcherPathRequestV2 & pathRequest )
		{
			bool status = false;

			if (isEmptyPath( path ) && (access_ == WT_READ_WRITE))
			{
				TYPE & useValue =
					*(TYPE*)( ((uintptr)&rValue_) + ((uintptr)base) );

				BinaryIStream *istream = pathRequest.getValueStream();
				if (istream && watcherStreamToValue( *istream, useValue ))
				{
					status = true;
				}

				// push the result into the reply stream
				// assuming mode RW because we have already set
				Watcher::Mode mode = Watcher::WT_READ_WRITE;
				watcherValueToStream( pathRequest.getResultStream(), useValue, mode );
				pathRequest.setResult( comment_, mode, this, base );
			}

			return status;
		}
		//@}


	private:
		TYPE & rValue_;
		Watcher::Mode access_;
};


/**
 *	This template function helps to create new DataWatchers.
 */
template <class TYPE>
WatcherPtr makeWatcher( TYPE & rValue,
				Watcher::Mode access = Watcher::WT_READ_ONLY )
{
	return new DataWatcher< TYPE >( rValue, access );
}


/**
 *	This template function helps to create new DataWatchers.
 */
template <class CLASS, class TYPE>
WatcherPtr makeWatcher( TYPE CLASS::*memberPtr,
				Watcher::Mode access = Watcher::WT_READ_ONLY )
{
	CLASS * pNull = NULL;
	return new DataWatcher< TYPE >( pNull->*memberPtr, access );
}


// -----------------------------------------------------------------------------
// Section: FunctionWatcher
// -----------------------------------------------------------------------------

/**
 *	This templatised class is used to watch the result of a given function.
 *
 *	@see WatcherModule
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class CONSTRUCTION_TYPE = RETURN_TYPE>
class FunctionWatcher : public Watcher
{
	public:
		/// @name Construction/Destruction
		//@{
		/**
		 *	Constructor. Leave 'path' as NULL to prevent the watcher adding
		 *	itself to the root watcher. In the normal case, the MF_WATCH style
		 *	macros should be used, as they do error checking and this
		 *	constructor cannot (it's certainly not going to delete itself
		 *	anyway).
		 */
		explicit FunctionWatcher( RETURN_TYPE (*getFunction)(),
				void (*setFunction)( RETURN_TYPE ) = NULL,
				const char * path = NULL ) :
			getFunction_( getFunction ),
			setFunction_( setFunction )
		{
			if (path != NULL)
				Watcher::rootWatcher().addChild( path, this );
		}
		//@}

	protected:
		/// @name Overrides from Watcher.
		//@{
		// Override from Watcher.
		virtual bool getAsString( const void * base, const char * path,
			BW::string & result, BW::string & desc, Watcher::Mode & mode ) const
		{
			if (isEmptyPath( path ))
			{
				result = watcherValueToString( (*getFunction_)() );
				mode = (setFunction_ != NULL) ? WT_READ_WRITE : WT_READ_ONLY;
				desc = comment_;
				return true;
			}
			else
			{
				return false;
			}
		};

		// Override from Watcher.
		virtual bool setFromString( void * base, const char * path,
			const char * valueStr )
		{
			bool result = false;

			if (isEmptyPath( path ) && (setFunction_ != NULL))
			{
				CONSTRUCTION_TYPE value = CONSTRUCTION_TYPE();
				result = watcherStringToValue( valueStr, value );

				if (result)
				{
					(*setFunction_)( value );
				}
			}

			return result;
		}

		// Override from Watcher.
		virtual bool getAsStream( const void * base, const char * path,
			WatcherPathRequestV2 & pathRequest ) const
		{
			if (isEmptyPath( path ))
			{
				Watcher::Mode mode = (setFunction_ != NULL) ? WT_READ_WRITE : WT_READ_ONLY;
				watcherValueToStream( pathRequest.getResultStream(), (*getFunction_)(), mode );
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
				return false;
			}
		}

		// Override from Watcher.
		virtual bool setFromStream( void * base, const char * path,
			WatcherPathRequestV2 & pathRequest )
		{
			bool status = false;

			if (isEmptyPath( path ) && (setFunction_ != NULL))
			{
				CONSTRUCTION_TYPE value = CONSTRUCTION_TYPE();

				BinaryIStream *istream = pathRequest.getValueStream();
				if (istream && watcherStreamToValue( *istream, value ))
				{
					status = true;
					(*setFunction_)( value );
				}

				// push the result into the reply stream
				// assuming mode RW because we have already set
				Watcher::Mode mode = Watcher::WT_READ_WRITE;
				watcherValueToStream( pathRequest.getResultStream(), value, mode );
				pathRequest.setResult( comment_, mode, this, base );
			}

			return status;
		}
		//@}


	private:
		RETURN_TYPE (*getFunction_)();
		void (*setFunction_)( RETURN_TYPE );
};


// -----------------------------------------------------------------------------
// Section: CallableWatcher
// -----------------------------------------------------------------------------

/**
 *	This class is a base class that is useful for implementing callable
 *	watchers.
 */
class CallableWatcher : public Watcher
{

private:
	/**
	 *	This helper class stores information about an argument of the callable
	 *	watcher.
	 */
	class ArgDescription
	{
	public:
		ArgDescription( WatcherDataType type, const BW::string & description );

		WatcherDataType type() const			{ return type_; }
		const BW::string & description() const	{ return description_; }

	private:
		WatcherDataType type_;
		BW::string description_;
	};

public:
	enum ExposeHint
	{
		WITH_ENTITY,
		ALL,
		WITH_SPACE,
		LEAST_LOADED,
		LOCAL_ONLY
	};

	CSTDMF_DLL CallableWatcher( ExposeHint hint = LOCAL_ONLY,
			const char * comment = "" );

	CSTDMF_DLL void addArg( WatcherDataType type, const char * description = "" );

	ExposeHint exposedHint() const		{ return exposeHint_; }
	void exposeHint( ExposeHint hint )	{ exposeHint_ = hint; }

protected:
	// Override from Watcher.
	CSTDMF_DLL virtual bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, Watcher::Mode & mode ) const;

	CSTDMF_DLL virtual bool setFromString( void * base, const char * path,
		const char * valueStr );

	CSTDMF_DLL virtual bool getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const;

	static int tupleElementStreamSize( int payloadSize )
	{
		const int TYPE_OVERHEAD = 2 * sizeof( uint8 );

		return TYPE_OVERHEAD +
			BinaryOStream::calculatePackedIntSize( payloadSize ) +
			payloadSize;
	}

	CSTDMF_DLL BinaryOStream & startResultStream( WatcherPathRequestV2 & pathRequest,
		const BW::string & output, int returnValueSize ) const;

	CSTDMF_DLL void endResultStream( WatcherPathRequestV2 & pathRequest,
		const void * base );

private:
	static bool isArgsPath( const char * path );
	static bool isExposePath( const char * path );

private:
	ExposeHint exposeHint_;
	BW::vector< ArgDescription > argList_;
};


/**
 *	This class implements a callable watcher that does not expect arguments.
 */
class NoArgCallableWatcher : public CallableWatcher
{
public:
	NoArgCallableWatcher(
			ExposeHint hint = LOCAL_ONLY, const char * comment = "" );

protected:
	virtual bool setFromStream( void * base,
			const char * path,
			WatcherPathRequestV2 & pathRequest );

	virtual bool onCall( BW::string & output, BW::string & value ) = 0;
};


/**
 *	This class implements a callable watcher that's handled by a function.
 */
class NoArgFuncCallableWatcher : public NoArgCallableWatcher
{
public:
	typedef bool (*Function)( BW::string & output, BW::string & value );
	NoArgFuncCallableWatcher( Function func,
			ExposeHint hint = LOCAL_ONLY, const char * comment = "" );

private:
	virtual bool onCall( BW::string & output, BW::string & value );

	Function func_;
};


/**
 *	This class implements a callable watcher that lets its subclass
 *	decode the parameter stream as it sees fit, and returns a string
 *	result.
 */
class SimpleCallableWatcher : public CallableWatcher
{
public:
	SimpleCallableWatcher(
			ExposeHint hint = LOCAL_ONLY, const char * comment = "" );
	virtual ~SimpleCallableWatcher() {}

protected:
	virtual bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest );

private:
	/**
	 *	This method is called with a stream containing parameters
	 *	and should write its output to output, and its return value
	 *	to value, and return true on success, or false if anything
	 *	goes wrong in parameter streaming or the function call fails.
	 */
	virtual bool onCall( BW::string & output, BW::string & value,
		int parameterCount, BinaryIStream & parameters  ) = 0;
};

extern Watcher * g_pRootWatcher;
extern const char * g_pRootWatcherPath;

inline bool Watcher::hasRootWatcher()
{
	if (!hasRootWatcherInternal())
	{
		return false;
	}
	return g_pRootWatcherPath == NULL || g_pRootWatcher != NULL;
}

inline Watcher & Watcher::rootWatcher()
{
	Watcher & watcher = rootWatcherInternal();
	if (g_pRootWatcherPath == NULL)
	{
		return watcher;
	}
	if (g_pRootWatcher == NULL)
	{
		g_pRootWatcher = new DirectoryWatcher();
		watcher.addChild( g_pRootWatcherPath, g_pRootWatcher );
	}
	return *g_pRootWatcher;
}

inline void Watcher::fini()
{
	if (hasRootWatcher())
	{
		Watcher & watcher = rootWatcherInternal();
		if (g_pRootWatcher == &watcher)
		{
			finiInternal();
		}
		else if (g_pRootWatcher != NULL)
		{
			watcher.removeChild( g_pRootWatcherPath );
		}
	}
}


/**
 *	This function is used to add a read-only watcher using a get-accessor
 *	method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr addReferenceWatcher( const char * path,
			OBJECT_TYPE & rObject,
			const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const,
			const char * comment = NULL	)
{
	WatcherPtr pNewWatcher = makeWatcher( rObject, getMethod, NULL );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}


/**
 *	This function is used to add a read/write watcher using a get and set
 *	methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr addReferenceWatcher( const char * path,
						OBJECT_TYPE & rObject,
						const RETURN_TYPE & (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( const RETURN_TYPE & ),
						const char * comment = NULL	)
{
	WatcherPtr pNewWatcher = makeWatcher( rObject, getMethod, setMethod );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}


/**
 *	This function is used to add a read-only watcher using a get-accessor
 *	method.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE, class OBJECT_TYPE2>
WatcherPtr addWatcher(	const char * path,
						OBJECT_TYPE & rObject,
						RETURN_TYPE (OBJECT_TYPE2::*getMethod)() const,
						const char * comment = NULL	)
{
	WatcherPtr pNewWatcher =
		new MemberWatcher<RETURN_TYPE, OBJECT_TYPE, RETURN_TYPE>(
				rObject,
				getMethod,
				static_cast< void (OBJECT_TYPE::*)( RETURN_TYPE ) >( NULL ) );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}


/**
 *	This function is used to add a read/write watcher using a get and set
 *	accessor methods.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE, class OBJECT_TYPE>
WatcherPtr addWatcher(	const char * path,
						OBJECT_TYPE & rObject,
						RETURN_TYPE (OBJECT_TYPE::*getMethod)() const,
						void (OBJECT_TYPE::*setMethod)( RETURN_TYPE ),
						const char * comment = NULL )
{
	// WatcherPtr pNewWatcher = makeWatcher( rObject, getMethod, setMethod );
	WatcherPtr pNewWatcher = new MemberWatcher<RETURN_TYPE, OBJECT_TYPE>(
			rObject, getMethod, setMethod );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}


/**
 *	This function is used to add a watcher of a function value.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE>
WatcherPtr addWatcher(	const char * path,
						RETURN_TYPE (*getFunction)(),
						void (*setFunction)( RETURN_TYPE ),
						const char * comment = NULL )
{
	WatcherPtr pNewWatcher =
		new FunctionWatcher<RETURN_TYPE>( getFunction, setFunction );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}


/**
 *	This function is used to add a read-only watcher of a function value.
 *
 *	@ingroup WatcherModule
 */
template <class RETURN_TYPE>
WatcherPtr addWatcher(	const char * path,
						RETURN_TYPE (*getFunction)(),
						const char * comment = NULL	)
{
	void (*setFunction)( RETURN_TYPE ) = NULL;

	return ::BW_NAMESPACE addWatcher( path, getFunction, setFunction, comment );
}


/**
 *	This function is used to add a watcher of a simple value.
 *
 *	@ingroup WatcherModule
 */
template <class TYPE>
WatcherPtr addWatcher(	const char * path,
						TYPE & rValue,
						Watcher::Mode access = Watcher::WT_READ_WRITE,
						const char * comment = NULL	)
{
	WatcherPtr pNewWatcher = makeWatcher( rValue, access );

	if (!Watcher::rootWatcher().addChild( path, pNewWatcher ))
	{
		pNewWatcher = NULL;
	}
	else if (comment)
	{
		BW::string s( comment );
		pNewWatcher->setComment( s );
	}

	return pNewWatcher;
}

/**
 *	This function is used to get an existing watcher by path.
 *
 *	@ingroup WatcherModule
 */
inline WatcherPtr getWatcher( const char * path )
{
	return Watcher::rootWatcher().getChild( path );
}

/** 
 *	This class implements a freezable watcher, that defines a boolean
 *	watcher that is used to automatically freeze data going through a
 *	process.
 */
template <typename DataType>
class FreezeWatcher
{
public:
	FreezeWatcher( const char * watcherName, const char * comment ) : 
		isFrozen_( false ),
		name_( watcherName ),
		wasFrozen_( false )
	{
		addWatcher( watcherName, isFrozen_, Watcher::WT_READ_WRITE, comment );
	}

	~FreezeWatcher()
	{
		if (Watcher::hasRootWatcher())
		{
			Watcher::rootWatcher().removeChild( name_ );
		}
	}

	void freezeValue( DataType & value )
	{
		if (isFrozen_)
		{
			if (wasFrozen_)
			{
				// Override latest value with the stored value
				value = data_;
			}
			else
			{
				// Record latest value
				data_ = value;
			}
		}
		wasFrozen_ = isFrozen_;
	}

private:
	const char* name_;
	bool isFrozen_;
	bool wasFrozen_;
	DataType data_;
};


/**
 *	This watcher allows for a watcher to watch an object with an absolute 
 *	address, circumventing the usual mechanism of offseting from a base address.
 */
class AbsoluteWatcher : public Watcher
{
public:
	AbsoluteWatcher( WatcherPtr pWatcher, void * absoluteAddress ) :
		pWatcher_( pWatcher ),
		absoluteAddress_( reinterpret_cast< uintptr >( absoluteAddress ) )
	{}

	// Overrides from Watcher
	bool getAsString( const void * base, const char * path,
		BW::string & result, BW::string & desc, 
		Watcher::Mode & mode ) const /* override */;
	bool setFromString( void * base, const char * path, 
		const char * valueStr ) /* override */;
	bool getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const /* override */;
	bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) /* override */;
	bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest ) /* override */;
	bool addChild( const char * path, WatcherPtr pChild,
		void * withBase = NULL ) /* override */;

private:
	WatcherPtr	pWatcher_;
	uintptr		absoluteAddress_;
};


/**
 *	This macro is used to watch a value at run-time for debugging purposes. This
 *	value can then be inspected and changed at run-time through a variety of
 *	methods. These include using the Web interface or using the in-built menu
 *	system in an appropriate build of the client. (This is displayed by pressing
 *	\<F7\>.)
 *
 *	The simplest usage of this macro is when you want to watch a fixed variable
 *	such as a global or a variable that is static to a file, class, or method.
 *	The macro should be called once during initialisation for each value that
 *	is to be watched.
 *
 *	For example, to watch a value that is static to a file, do the following:
 *
 *	@code
 *	static int myValue = 72;
 *	...
 *	MF_WATCH( "myValue", myValue );
 *	@endcode
 *
 *	This allows you to inspect and change the value of @a myValue. The first
 *	argument to the macro is a string specifying the path associated with this
 *	value. This is used in displaying and setting this value via the different
 *	%Watcher interfaces.
 *
 *	If you want to not allow the value to be changed using the Watch interfaces,
 *	include the extra argument Watcher::WT_READ_ONLY.
 *
 *	@code
 *	MF_WATCH( "myValue", myValue, Watcher::WT_READ_ONLY );
 *	@endcode
 *
 *	Any type of value can be watched as long as it has streaming operators to
 *	ostream and istream.
 *
 *	You can also look at a value associated with an object using accessor
 *	methods on that object.
 *
 *	For example, if you had the following class:
 *	@code
 *	class ExampleClass
 *	{
 *		public:
 *			int getValue() const;
 *			void setValue( int setValue ) const;
 *	};
 *
 *	ExampleClass exampleObject_;
 *	@endcode
 *
 *	You can watch this value as follows:
 *	@code
 *	MF_WATCH( "some value", exampleObject_,
 *			ExampleClass::getValue,
 *			ExampleClass::setValue );
 *	@endcode
 *
 *	If you have overloaded your accessor methods, use the @ref MF_ACCESSORS
 *	macro to help with casting.
 *
 *	If you want the value to be read-only, do not add the set accessor to the
 *	macro call.
 *	@code
 *	MF_WATCH( "some value", exampleObject_,
 *			ExampleClass::getValue );
 *	@endcode
 *
 *	If you want to watch a value and the accessors take and return a reference
 *	to the object, instead of a copy of the object.
 *
 *	@ingroup WatcherModule
 *
 *	@todo More details in comment including the run-time interfaces for Watcher.
 */
#define MF_WATCH				::BW_NAMESPACE addWatcher

/**
 *	This macro is used to watch a value at run-time for debugging purposes. This
 *	macro should be used instead of MF_WATCH if the accessors take and return a
 *	reference to the object being watched.
 *
 *	For example,
 *
 *	@code
 *	class ExampleClass
 *	{
 *		const Vector3 & getPosition() const;
 *		void setPosition( const Vector3 & newPosition );
 *	};
 *	ExampleClass exampleObject_;
 *
 *	...
 *
 *	MF_WATCH_REF( "someValue", exampleObject_,
 *			ExampleClass::getPosition,
 *			ExampleClass::setPosition );
 *	@endcode
 *
 *	For more details, see MF_WATCH.
 *
 *	@ingroup WatcherModule
 */
#define MF_WATCH_REF			::BW_NAMESPACE addReferenceWatcher

/**
 *	This is a simple macro that helps us do casting.
 *	This allows us to do
 *
 *	@code
 *	MF_WATCH( "Comms/Desired in",
 *		g_server,
 *		MF_ACCESSORS( uint32, ServerConnection, bandwidthFromServer ) );
 *
 *	MF_WATCH( "Comms/Desired out",
 *		g_server,
 *		MF_ACCESSORS_EX( uint32, ServerConnection,
 *			getBandwidthFromClient, setBandwidthFromClient ) );
 *	@endcode
 *
 * instead of
 *
 *	@code
 *	MF_WATCH( "Comms/Desired in",
 *		g_server,
 *		(uint32 (MyClass::*)() const)(MyClass::bandwidthFromServer),
 *		(void   (MyClass::*)(uint32))(MyClass::bandwidthFromServer) );
 *	@endcode
 *
 *	@ingroup WatcherModule
 */
#define MF_ACCESSORS( TYPE, CLASS, METHOD )							\
	static_cast< TYPE (CLASS::*)() const >(&CLASS::METHOD),			\
	static_cast< void (CLASS::*)(TYPE)   >(&CLASS::METHOD)

#define MF_ACCESSORS_EX( TYPE, CLASS, GET_METHOD, SET_METHOD )			\
	static_cast< TYPE (CLASS::*)() const >(&CLASS::GET_METHOD),			\
	static_cast< void (CLASS::*)(TYPE)   >(&CLASS::SET_METHOD)


/**
 *	This macro is like MF_ACCESSORS except that the read accessor is NULL.
 *
 *	@see MF_ACCESSORS
 *
 *	@ingroup WatcherModule
 */
#define MF_WRITE_ACCESSOR( TYPE, CLASS, METHOD )					\
	static_cast< TYPE (CLASS::*)() const >( NULL ),					\
	static_cast< void (CLASS::*)(TYPE)   >( &CLASS::METHOD )


#define MF_FREEZE_DATA_WATCHER( TYPE, VAR, NAME, COMMENT ) \
	static FreezeWatcher<TYPE> freezeWatcher##VAR( NAME, COMMENT );\
	freezeWatcher##VAR.freezeValue( VAR );

BW_END_NAMESPACE

#else // ENABLE_WATCHERS

#define DECLARE_WATCHER_DATA( name )

BW_BEGIN_NAMESPACE

// We want to stub out the calls to watch, but VS2003 doesn't understand variable
// length arguments for macros, hence the odd define.
// A watch such as MF_WATCH( "something", object, function() ) compiles to:
//	false && ( "something", object, function() );
// Which is a valid C++ expression, effectively short-circuted to a NOP.
#if defined(_WIN32) && ( _MSC_VER < 1400 )
	#define MF_WATCH false &&
	#define MF_WATCH_REF false &&
#else
	#define MF_WATCH( ... )
	#define MF_WATCH_REF( ... )
#endif

#define MF_ACCESSORS( TYPE, CLASS, METHOD )	NULL

class Watcher
{
public:
	enum Mode
	{
		WT_INVALID,			///< Indicates an error.
		WT_READ_ONLY,		///< Indicates the watched value cannot be changed.
		WT_READ_WRITE,		///< Indicates the watched value can be changed.
		WT_DIRECTORY		///< Indicates that the watcher has children.
	};
};

#define MF_FREEZE_DATA_WATCHER( TYPE, VAR, NAME, COMMENT )

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

#endif // WATCHER_HPP
