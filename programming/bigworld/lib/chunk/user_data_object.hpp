#ifndef USER_DATA_OBJECT_HPP
#define USER_DATA_OBJECT_HPP

#include "script/script_object.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "user_data_object_type.hpp"
#include "math/matrix.hpp"
#include "cstdmf/unique_id.hpp"
#include "cstdmf/guard.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
class UserDataObject;
class UserDataObjectType;

typedef SmartPointer< UserDataObject >					UserDataObjectPtr;
typedef BW::map< UniqueID, UserDataObjectPtr >			UserDataObjectMap;

struct UserDataObjectInitData
{
	UniqueID guid;
	Position3D position;
	Direction3D direction;
	DataSectionPtr propertiesDS;
};


/*~ class BigWorld.UserDataObject
 *  @components{ cell }
 *  A user data object. Defined by DEF files.
 *
 */
class UserDataObject : public PyObjectPlus
{
	Py_Header( UserDataObject, PyObjectPlus )

public:
	// Preventing NaN's getting through, hopefully
	static bool isValidPosition( const Position3D & c )
	{
		const float MAX_ENTITY_POS = 1000000000.f;
		return (-MAX_ENTITY_POS < c.x && c.x < MAX_ENTITY_POS &&
			-MAX_ENTITY_POS < c.z && c.z < MAX_ENTITY_POS);
	}

	static UserDataObject * get( const UniqueID & guid );

	static UserDataObjectPtr findOrLoad(
		const UserDataObjectInitData& initData, UserDataObjectTypePtr pType );

	static UserDataObject * createRef( const BW::string & guid );
	static UserDataObject * createRef( const UniqueID & guid );
	static bool createRefType();

	/// @name Construction and Destruction
	//@{
	UserDataObject( UserDataObjectTypePtr pUserDataObjectType,
		   const UniqueID & guid );
	~UserDataObject();

	//@}

	void incChunkItemRefCount();
	void decChunkItemRefCount();

	bool isLoaded() const;
	bool isInCollection() const;

	/// @name Accessors
	//@{
	const UniqueID & guid() const;
	const Position3D & position() const;
	const Direction3D & direction() const;
	// DEBUG
	//@}

	const UserDataObjectType & getType() const { return *pUserDataObjectType_; }

	void resetType( UserDataObjectTypePtr pNewType );

private:
	UserDataObject( const UserDataObject & );

	void load( const UserDataObjectInitData & initData,
			UserDataObjectTypePtr pType );
	void unload();

	void addToCollection();
	void removeFromCollection();

	ScriptObject getUDODict() const;

	/// @name Script related methods
	//@{
	PY_RO_ATTRIBUTE_DECLARE( guid_, guid )
	PY_RO_ATTRIBUTE_DECLARE( (Vector3&) globalDirection_, direction )
	PY_RO_ATTRIBUTE_DECLARE( (Vector3&) globalPosition_, position)
	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.yaw, yaw )
	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.pitch, pitch )
	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.roll, roll )

	PyObject * pyGet_spaceID();
	PY_RO_ATTRIBUTE_SET( spaceID )

	void callScriptInit();
	void callScriptDel();

	void setGlobalPosition( const Vector3 & v );
	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	//@}

	// Private data
	UserDataObjectTypePtr	pUserDataObjectType_;

	typedef BW::map<UniqueID,UserDataObject*> UdoMap;
	static UdoMap s_created_;

	UniqueID		guid_;
	Position3D		globalPosition_;
	Direction3D		globalDirection_;
	bool			isLoaded_;

	// Number of ChunkUserDataObjects owning this.
	int				chunkItemRefCount_;
};

#ifdef CODE_INLINE
#include "user_data_object.ipp"
#endif

BW_END_NAMESPACE

#endif // CUSTOM_CHUNK_ITEM_HPP

