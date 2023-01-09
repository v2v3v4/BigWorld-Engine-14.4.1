#ifndef SCRIPT_DATA_SINK_HPP
#define SCRIPT_DATA_SINK_HPP

#include "data_sink.hpp"

#include "bwentity/bwentity_api.hpp"
#include "script/script_object.hpp"

namespace BW
{

class DataType;

// -----------------------------------------------------------------------------
// Section: ScriptDataSink
// -----------------------------------------------------------------------------
/**
 *	This class provides a DataSink implementation that populates a ScriptObject.
 */
class BWENTITY_API ScriptDataSink : public DataSink
{
public:
	ScriptDataSink();
	ScriptObject finalise();

	// DataSink implementation
	bool write( const float & value );
	bool write( const double & value );
	bool write( const uint8 & value );
	bool write( const uint16 & value );
	bool write( const uint32 & value );
	bool write( const uint64 & value );
	bool write( const int8 & value );
	bool write( const int16 & value );
	bool write( const int32 & value );
	bool write( const int64 & value );
	bool write( const BW::string & value );
	bool write( const BW::wstring & value );
	bool write( const BW::Vector2 & value );
	bool write( const BW::Vector3 & value );
	bool write( const BW::Vector4 & value );
#if defined( MF_SERVER )
	bool write( const EntityMailBoxRef & value );
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	bool write( const BW::UniqueID & value );
#endif // !defined( EDITOR_ENABLED )

	bool writeNone( bool isNone );
	bool writeBlob( const BW::string & value );

	bool beginArray( const ArrayDataType * pType, size_t count );
	bool beginTuple( const TupleDataType * pType, size_t count );
	bool enterItem( size_t item );
	bool leaveItem();

	bool beginClass( const ClassDataType * pType );
	bool beginDictionary( const FixedDictDataType * pType );
	bool enterField( const BW::string & name );
	bool leaveField();

	bool writeCustomType( const FixedDictDataType & type,
		BinaryIStream & stream, bool isPersistentOnly );
	bool writeCustomType( const FixedDictDataType & type,
		DataSection & section );
	bool writeCustomType( const UserDataType & type,
		BinaryIStream & stream, bool isPersistentOnly );
	bool writeCustomType( const UserDataType & type,
		DataSection & section );

	// ScriptDataSink methods
	bool write( const ScriptObject & value );

private:
	enum CollectionType { None, Array, Tuple, Class, Dictionary, Custom };

	ScriptObject & currentObject();
	ScriptObject & parentObject();
	CollectionType parentType() const;
	size_t & parentItemIndex();
	BW::string & parentFieldName();

	struct Node
	{
		Node( CollectionType type ) : collectionType( type ) {}

		ScriptObject object;
		// These describe the parent and slot of the above Object
		CollectionType collectionType;
		size_t		item;
		BW::string	field;
	};

	ScriptObject root_;

	typedef BW::vector< Node >  VectorOfNodes;
	class BWENTITY_API VectorOfNodesHolder
	{
	public:
		VectorOfNodesHolder();
		~VectorOfNodesHolder();
		operator VectorOfNodes & ();
	private:
		VectorOfNodes * data_;
	} parentHolder_;
	VectorOfNodes & parents_;

	// This is used to hold the CollectionType we're building between
	// beginArray/beginTuple and enterItem, or between
	// beginClass/beginDictionary and enterField
	CollectionType buildingType_;
};

} // namespace BW

#endif // SCRIPT_DATA_SINK_HPP
