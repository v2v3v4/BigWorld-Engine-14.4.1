#ifndef DATA_SOURCE_HPP
#define DATA_SOURCE_HPP

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"

namespace BW
{

class BinaryOStream;
class DataSection;

class FixedDictDataType;
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
 *	This interface is used by entitydef to fill a stream from an object without
 *	needing to know what code underlies the object.
 */
class BWENTITY_API DataSource
{
public:
	virtual ~DataSource() {};

	// Basic BigWorld network types: These all return
	// false if the read fails (i.e. incorrect type)
	virtual bool read( float & value ) = 0;			// FLOAT32
	virtual bool read( double & value ) = 0;		// FLOAT64
	virtual bool read( uint8 & value ) = 0;			// UINT8
	virtual bool read( uint16 & value ) = 0;		// UINT16
	virtual bool read( uint32 & value ) = 0;		// UINT32
	virtual bool read( uint64 & value ) = 0;		// UINT64
	virtual bool read( int8 & value ) = 0;			// INT8
	virtual bool read( int16 & value ) = 0;			// INT16
	virtual bool read( int32 & value ) = 0;			// INT32
	virtual bool read( int64 & value ) = 0;			// INT64
	// Note: Must handle in-string NULLs correctly.
	virtual bool read( BW::string & value ) = 0;	// STRING, PYTHON
	virtual bool read( BW::wstring & value ) = 0;	// UNICODE_STRING
	virtual bool read( BW::Vector2 & value ) = 0;	// VECTOR2
	virtual bool read( BW::Vector3 & value ) = 0;	// VECTOR3
	virtual bool read( BW::Vector4 & value ) = 0;	// VECTOR4
#if defined( MF_SERVER )
	virtual bool read( EntityMailBoxRef & value ) = 0;	// MAILBOX
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	// TODO: Get a consistent UDORef object in place
	// This method is reading a UDO_REF, which apart from the
	// editor, is stored and streamed simply by GUID.
	// The editor instead receives a fixed two-tuple of strings,
	// being the GUID and ChunkID.
	virtual bool read( BW::UniqueID & value ) = 0;	// UDO_REF
#endif // !defined( EDITOR_ENABLED )

	/**
	 *	This method is called to query a possibly-None value to
	 *	see if it is None.
	 *
	 *	If isNone is false on return, further reads will occur to
	 *	read the actual data type.
	 *
	 *	@param	isNone	Output boolean set to true if the current
	 *					value being read is None, and false otherwise.
	 *	@return			True unless the read fails (i.e. the DataSource
	 *					knows that it could not have been a None value)
	 */
	virtual bool readNone( bool & isNone ) = 0;

	/**
	 *	This method is called to read an unstructured blob of data.
	 */
	virtual bool readBlob( BW::string & value ) = 0;	// BLOB

	// BigWorld sequence handling
	/**
	 *	This method is called to begin reading an ARRAY or TUPLE from
	 *	the DataSource.
	 *
	 *	@param	count	The count of elements contained in the sequence or 0
	 *					if the DataSource does not know.
	 *					TODO: Better unknown-length signal.
	 *	@return			True unless the read fails (i.e. the next data the
	 *					DataSource would read is not a sequence.)
	 */
	virtual bool beginSequence( size_t & count ) = 0;
	/**
	 *	This method is used before the element of a sequence is read.
	 *
	 *	readSequence will have been called before this method.
	 *
	 *	@param	item	The index of the element in the sequence.
	 *	@return			True unless the read fails (i.e. the next data the
	 *					DataSource would read is not a sequence element.)
	 */
	virtual bool enterItem( size_t item ) = 0;
	/**
	 *	This method is used after the element of a sequence is read.
	 *
	 *	@return			True unless the read fails (i.e. the last data the
	 *					DataSource read was not a sequence element.)
	 */
	virtual bool leaveItem() = 0;

	// BigWorld structure handling
	/**
	 *	This method is called to begin reading a CLASS from the DataSource.
	 *
	 *	@return			True unless the read fails (i.e. the next data the
	 *					DataSource would read is not attributed.)
	 */
	virtual bool beginClass() = 0;
	/**
	 *	This method is called to begin reading a FIXED_DICT from the
	 *	DataSource.
	 *
	 *	@param	pType			The FixedDictDataType being read.
	 *	@return					True unless the read fails (i.e. the next data
	 *							the DataSource would read is not a mapping.)
	 */
	virtual bool beginDictionary( const FixedDictDataType * pType ) = 0;
	/**
	 *	This method is called before an attribute of a class or field of a
	 *	dictionary is read.
	 *
	 *	beginClass or beginDictionary will be called before this method.
	 *
	 *	@param	name	The name of the attribute or field.
	 *	@return			True unless the read fails (i.e. the next data the
	 *					DataSink would read is not a field of a structure)
	 */
	virtual bool enterField( const BW::string & name ) = 0;
	/**
	 *	This method is called after an attribute of a class or field of a
	 *	dictionary is read.
	 *
	 *	beginClass or beginDictionary will be called before this method.
	 *
	 *	@param	name	The name of the attribute or field.
	 *	@return			True unless the read fails (i.e. the last data the
	 *					DataSink read wass not a field of a structure)
	 */
	virtual bool leaveField() = 0;

	// BigWorld user data type handling
	// BigWorld user data types can customise their stream and DataSection
	// representations, so it's up to the DataSource to actually deal with
	// this in a system-appropriate way.
	/**
	 *	This method is called to read a whole FIXED_DICT custom type into
	 *	a stream.
	 *
	 *	@param	type				The FixedDictDataType being read.
	 *	@param	stream				The stream to write to
	 *	@param	isPersistentOnly	true if this stream is only to contain
	 *								persistent data.
	 *	@return						True unless the read fails (i.e. the next
	 *								data the DataSource would read is not a
	 *								mapping which supports providing its data as
	 *								a single blob.)
	 */
	virtual bool readCustomType( const FixedDictDataType & type,
		BinaryOStream & stream, bool isPersistentOnly ) = 0;
	/**
	 *	This method is called to read a whole FIXED_DICT custom type into
	 *	a DataSection.
	 *
	 *	@param	type				The FixedDictDataType being read.
	 *	@param	stream				The stream to write to
	 *	@param	isPersistentOnly	true if this stream is only to contain
	 *								persistent data.
	 *	@return						True unless the read fails (i.e. the next
	 *								data the DataSource would read is not a
	 *								mapping which supports providing its data as
	 *								a single blob.)
	 */
	virtual bool readCustomType( const FixedDictDataType & type,
		DataSection & section ) = 0;
	/**
	 *	This method is called to read a whole USER_TYPE custom type into
	 *	a stream.
	 *
	 *	@param	type				The FixedDictDataType being read.
	 *	@param	stream				The stream to write to
	 *	@param	isPersistentOnly	true if this stream is only to contain
	 *								persistent data.
	 *	@return						True unless the read fails (i.e. the next
	 *								data the DataSource would read is not a
	 *								mapping which supports providing its data as
	 *								a single blob.)
	 */
	virtual bool readCustomType( const UserDataType & type,
		BinaryOStream & stream, bool isPersistentOnly ) = 0;
	/**
	 *	This method is called to read a whole USER_TYPE custom type into
	 *	a DataSection.
	 *
	 *	@param	type				The FixedDictDataType being read.
	 *	@param	stream				The stream to write to
	 *	@param	isPersistentOnly	true if this stream is only to contain
	 *								persistent data.
	 *	@return						True unless the read fails (i.e. the next
	 *								data the DataSource would read is not a
	 *								mapping which supports providing its data as
	 *								a single blob.)
	 */
	virtual bool readCustomType( const UserDataType & type,
		DataSection & section ) = 0;

};


/**
 *	This class is a DataSource that returns 'false' for every interface, for
 *	easy creation of single-typed DataSources.
 */
class BWENTITY_API UnsupportedDataSource : public DataSource
{
	bool read( float & ) { return false; }
	bool read( double & ) { return false; }
	bool read( uint8 & ) { return false; }
	bool read( uint16 & ) { return false; }
	bool read( uint32 & ) { return false; }
	bool read( uint64 & ) { return false; }
	bool read( int8 & ) { return false; }
	bool read( int16 & ) { return false; }
	bool read( int32 & ) { return false; }
	bool read( int64 & ) { return false; }
	bool read( BW::string & ) { return false; }
	bool read( BW::wstring & ) { return false; }
	bool read( BW::Vector2 & ) { return false; }
	bool read( BW::Vector3 & ) { return false; }
	bool read( BW::Vector4 & ) { return false; }
#if defined( MF_SERVER )
	bool read( EntityMailBoxRef & ) { return false; }
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	bool read( BW::UniqueID & ) { return false; }
#endif // !defined( EDITOR_ENABLED )
	bool readNone( bool & ) { return false; }
	bool readBlob( BW::string & ) { return false; }
	bool beginSequence( size_t & ) { return false; }
	bool enterItem( size_t ) { return false; }
	bool leaveItem() { return false; }
	bool beginClass() { return false; }
	bool beginDictionary( const FixedDictDataType * ) { return false; }
	bool enterField( const BW::string & ) { return false; }
	bool leaveField() { return false; }
	bool readCustomType( const FixedDictDataType &, BinaryOStream &, bool )
		{ return false; }
	bool readCustomType( const FixedDictDataType &, DataSection & )
		{ return false; }
	bool readCustomType( const UserDataType &, BinaryOStream & , bool )
		{ return false; }
	bool readCustomType( const UserDataType &, DataSection & )
		{ return false; }
};

} // namespace BW

#endif // DATA_SOURCE_HPP
