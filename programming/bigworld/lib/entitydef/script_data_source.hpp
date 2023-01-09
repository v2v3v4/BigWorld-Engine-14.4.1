#ifndef SCRIPT_DATA_SOURCE_HPP
#define SCRIPT_DATA_SOURCE_HPP

#include "data_source.hpp"

#include "bwentity/bwentity_api.hpp"
#include "script/script_object.hpp"

namespace BW
{

class ScriptObject;
class ScriptType;

// -----------------------------------------------------------------------------
// Section: ScriptDataSource
// -----------------------------------------------------------------------------
/**
 *	This class provides a DataSource implementation that reads from a
 *	ScriptObject.
 */
class BWENTITY_API ScriptDataSource : public DataSource
{
public:
	ScriptDataSource( const ScriptObject & object );

	// DataSource implementation
	bool read( float & value );
	bool read( double & value );
	bool read( uint8 & value );
	bool read( uint16 & value );
	bool read( uint32 & value );
	bool read( uint64 & value );
	bool read( int8 & value );
	bool read( int16 & value );
	bool read( int32 & value );
	bool read( int64 & value );
	bool read( BW::string & value );
	bool read( BW::wstring & value );
	bool read( BW::Vector2 & value );
	bool read( BW::Vector3 & value );
	bool read( BW::Vector4 & value );
#if defined( MF_SERVER )
	bool read( EntityMailBoxRef & value );
#endif // defined( MF_SERVER )

#if !defined( EDITOR_ENABLED )
	bool read( BW::UniqueID & value );
#endif // !defined( EDITOR_ENABLED )

	bool readNone( bool & isNone );
	bool readBlob( BW::string & value );

	bool beginSequence( size_t & count );
	bool enterItem( size_t item );
	bool leaveItem();

	bool beginClass();
	bool beginDictionary( const FixedDictDataType * pType );
	bool enterField( const BW::string & name );
	bool leaveField();

	bool readCustomType( const FixedDictDataType & type,
		BinaryOStream & stream, bool isPersistentOnly );
	bool readCustomType( const FixedDictDataType & type,
		DataSection & section );
	bool readCustomType( const UserDataType & type,
		BinaryOStream & stream, bool isPersistentOnly );
	bool readCustomType( const UserDataType & type,
		DataSection & section );

	// ScriptDataSource methods
	bool read( ScriptObject & value );

private:
	enum CollectionType
		{ None, Sequence, Class, Dictionary };

	ScriptObject & currentObject();

	bool hasParent() const;

	const ScriptObject & parentObject() const;
	CollectionType parentType() const;

	ScriptObject root_;

	typedef std::pair< ScriptObject, CollectionType > Node;
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

	// This is used to hold the CollectionType we're reading between
	// beginSequence/beginClass/beginDictionary and enterItem/enterField
	CollectionType readingType_;
};

} // namespace BW

#endif // SCRIPT_DATA_SOURCE_HPP
