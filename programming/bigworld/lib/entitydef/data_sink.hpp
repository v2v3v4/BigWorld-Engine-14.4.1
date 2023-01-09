#ifndef DATA_SINK_HPP
#define DATA_SINK_HPP

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"

namespace BW
{

class BinaryIStream;
class DataSection;

class ArrayDataType;
class ClassDataType;
class FixedDictDataType;
class TupleDataType;
class UserDataType;

class Vector2;
class Vector3;
class Vector4;
#if defined( MF_SERVER )
class EntityMailBoxRef;
#endif // defined( MF_SERVER )
#if !defined( EDITOR_ENABLED )
class UniqueID;
#endif // !defined( EDITOR_ENABLED )

/**
 *	This interface is used by entitydef to fill an object from a stream without
 *	needing to know what code underlies the object.
 */
class BWENTITY_API DataSink
{
public:
	virtual ~DataSink() {};

	// Basic BigWorld network types: These all return
	// false if the write fails (i.e. incorrect type or bad state)
	virtual bool write( const float & value ) = 0;			// FLOAT32
	virtual bool write( const double & value ) = 0;			// FLOAT64
	virtual bool write( const uint8 & value ) = 0;			// UINT8
	virtual bool write( const uint16 & value ) = 0;			// UINT16
	virtual bool write( const uint32 & value ) = 0;			// UINT32
	virtual bool write( const uint64 & value ) = 0;			// UINT64
	virtual bool write( const int8 & value ) = 0;			// INT8
	virtual bool write( const int16 & value ) = 0;			// INT16
	virtual bool write( const int32 & value ) = 0;			// INT32
	virtual bool write( const int64 & value ) = 0;			// INT64
	// Note: Must handle in-string NULLs correctly.
	virtual bool write( const BW::string & value ) = 0;		// STRING, PYTHON
	virtual bool write( const BW::wstring & value ) = 0;	// UNICODE_STRING
	virtual bool write( const BW::Vector2 & value ) = 0;	// VECTOR2
	virtual bool write( const BW::Vector3 & value ) = 0;	// VECTOR3
	virtual bool write( const BW::Vector4 & value ) = 0;	// VECTOR4
#if defined( MF_SERVER )
	virtual bool write( const EntityMailBoxRef & value ) = 0;	// MAILBOX
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	// TODO: Get a consistent UDORef object in place
	// This method is writing a UDO_REF, which apart from the
	// editor, is stored and streamed simply by GUID.
	// The editor instead receives a two-tuple of strings:
	// ( GUID, ChunkID )
	virtual bool write( const BW::UniqueID & value ) = 0;	// UDO_REF
#endif // !defined( EDITOR_ENABLED )

	/**
	 *	This method is called to report whether a possibly-None
	 *	value is None.
	 *
	 *	If isNone is false, further writes will occur to write the
	 *	actual data type.
	 *
	 *	@param	isNone	Indicates if a None value is being written.
	 *	@return			True unless the write fails (i.e. the DataSink
	 *					knows that it could not have been a None value)
	 */
	virtual bool writeNone( bool isNone ) = 0;

	/**
	 *	This method is called to store an unstructured blob of data.
	 */
	virtual bool writeBlob( const BW::string & value ) = 0;	// BLOB
	/**
	 *	This method is called to store an unstructured blob of data.
	 */
	/* Not required yet...
	virtual bool writeBlob( const char * pBuffer, size_t buffLen ) = 0;
	 */

	// BigWorld sequence handling
	/**
	 *	This method is called to begin writing an ARRAY to the DataSink.
	 *
	 *	@param	pType	The ArrayDataType being written.
	 *	@param	count	The count of elements that will be written.
	 *	@return			True unless the write fails (i.e. the next data the
	 *					DataSink would write cannot hold an array.)
	 */
	virtual bool beginArray( const ArrayDataType * pType, size_t count ) = 0;
	/**
	 *	This method is called to begin writing a TUPLE to the DataSink.
	 *
	 *	@param	pType	The TupleDataType being written.
	 *	@param	count	The count of elements that will be written.
	 *	@return			True unless the write fails (i.e. the next data the
	 *					DataSink would write cannot hold a tuple.)
	 */
	virtual bool beginTuple( const TupleDataType * pType, size_t count ) = 0;
	/**
	 *	This method is called before the element of a sequence is written.
	 *
	 *	writeArray or writeTuple will be called before this method.
	 *
	 *	@param	item	The index of the element in the sequence.
	 *	@return			True unless the write fails (i.e. the next data the
	 *					DataSink would write cannot hold a sequence element.)
	 */
	virtual bool enterItem( size_t item ) = 0;
	/**
	 *	This method is called after the element of a sequence is written.
	 *
	 *	@return			True unless the write fails (i.e. the last data the
	 *					DataSink wrote was not a sequence element.)
	 */
	virtual bool leaveItem() = 0;

	// BigWorld structure handling
	/**
	 *	This method is called to begin writing a CLASS to the DataSink.
	 *
	 *	@param	pType	The ClassDataType being written.
	 *	@return			True unless the write fails (i.e. the next data the
	 *					DataSink would write cannot hold a class.)
	 */
	virtual bool beginClass( const ClassDataType * pType ) = 0;
	/**
	 *	This method is called to begin writing a FIXED_DICT to the DataSink.
	 *
	 *	@param	pType			The FixedDictDataType being written.
	 *	@return					True unless the write fails (i.e. the next data
	 *							the DataSink would write cannot hold a class.)
	 */
	virtual bool beginDictionary( const FixedDictDataType * pType ) = 0;
	/**
	 *	This method is called before an attribute of a class or field of a
	 *	dictionary is written.
	 *
	 *	beginClass or beginDictionary will be called before this method.
	 *
	 *	@param	name	The name of the attribute or field.
	 *	@return			True unless the write fails (i.e. the next data the
	 *					DataSink would write cannot hold an attribute or field)
	 */
	virtual bool enterField( const BW::string & name ) = 0;
	/**
	 *	This method is called after an attribute of a class or field of a
	 *	dictionary is written.
	 *
	 *	@return			True unless the write fails (i.e. the last data the
	 *					DataSink wrote was not an attribute or field)
	 */
	virtual bool leaveField() = 0;

	// BigWorld user data type handling
	// BigWorld user data types can customise their stream and DataSection
	// representations, so it's up to the DataSink to actually deal with
	// this in a system-appropriate way.
	/**
	 *	This method is called to write a whole FIXED_DICT custom type from a
	 *	stream.
	 *
	 *	@param	type				The FixedDictDataType being written.
	 *	@param	stream				The stream containing the data to write.
	 *	@param	isPersistentOnly	true if this stream contains only persistent
	 *								data.
	 *	@return						True unless the write fails (i.e. we were
	 *								not expecting a dictionary in a stream or
	 *								the stream could not be decoded)
	 */
	virtual bool writeCustomType( const FixedDictDataType & type,
		BinaryIStream & stream, bool isPersistentOnly ) = 0;
	/**
	 *	This method is called to write a whole FIXED_DICT custom type from a
	 *	DataSection.
	 *
	 *	@param	type				The FixedDictDataType being written.
	 *	@param	section				The DataSection containing the data.
	 *	@return						True unless the write fails (i.e. we were
	 *								not expecting a dictionary in a stream or
	 *								the stream could not be decoded)
	 */
	virtual bool writeCustomType( const FixedDictDataType & type,
		DataSection & section ) = 0;
	/**
	 *	This method is called to write a whole USER_TYPE custom type from a
	 *	stream.
	 *
	 *	@param	type				The FixedDictDataType being written.
	 *	@param	stream				The stream containing the data to write.
	 *	@param	isPersistentOnly	true if this stream contains only persistent
	 *								data.
	 *	@return						True unless the write fails (i.e. we were
	 *								not expecting a dictionary in a stream or
	 *								the stream could not be decoded)
	 */
	virtual bool writeCustomType( const UserDataType & type,
		BinaryIStream & stream, bool isPersistentOnly ) = 0;
	/**
	 *	This method is called to write a whole USER_TYPE custom type from a
	 *	DataSection.
	 *
	 *	@param	type				The FixedDictDataType being written.
	 *	@param	section				The DataSection containing the data.
	 *	@return						True unless the write fails (i.e. we were
	 *								not expecting a dictionary in a stream or
	 *								the stream could not be decoded)
	 */
	virtual bool writeCustomType( const UserDataType & type,
		DataSection & section ) = 0;
};


/**
 *	This class is a DataSink that returns 'false' for every interface, for
 *	easy creation of single-typed DataSink.
 */
class BWENTITY_API UnsupportedDataSink : public DataSink
{
	bool write( const float & ) { return false; }
	bool write( const double & ) { return false; }
	bool write( const uint8 & ) { return false; }
	bool write( const uint16 & ) { return false; }
	bool write( const uint32 & ) { return false; }
	bool write( const uint64 & ) { return false; }
	bool write( const int8 & ) { return false; }
	bool write( const int16 & ) { return false; }
	bool write( const int32 & ) { return false; }
	bool write( const int64 & ) { return false; }
	bool write( const BW::string & ) { return false; }
	bool write( const BW::wstring & ) { return false; }
	bool write( const BW::Vector2 & ) { return false; }
	bool write( const BW::Vector3 & ) { return false; }
	bool write( const BW::Vector4 & ) { return false; }
#if defined( MF_SERVER )
	bool write( const EntityMailBoxRef & ) { return false; }
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	bool write( const BW::UniqueID & ) { return false; }
#endif // !defined( EDITOR_ENABLED )
	bool writeNone( bool ) { return false; }
	bool writeBlob( const BW::string & ) { return false; }
	bool beginArray( const ArrayDataType *, size_t ) { return false; }
	bool beginTuple( const TupleDataType *, size_t ) { return false; }
	bool enterItem( size_t ) { return false; }
	bool leaveItem() { return false; }
	bool beginClass( const ClassDataType * ) { return false; }
	bool beginDictionary( const FixedDictDataType * ) { return false; }
	bool enterField( const BW::string & ) { return false; }
	bool leaveField() { return false; }
	bool writeCustomType( const FixedDictDataType &, BinaryIStream &, bool )
		{ return false; }
	bool writeCustomType( const FixedDictDataType &, DataSection & )
		{ return false; }
	bool writeCustomType( const UserDataType &, BinaryIStream &, bool )
		{ return false; }
	bool writeCustomType( const UserDataType &, DataSection & )
		{ return false; }
};

} // namespace BW


#endif // DATA_SINK_HPP
