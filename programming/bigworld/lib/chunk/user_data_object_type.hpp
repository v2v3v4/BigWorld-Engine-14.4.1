#ifndef USER_DATA_OBJECT_TYPE_HPP
#define USER_DATA_OBJECT_TYPE_HPP

#include <Python.h>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"
#include "entitydef/user_data_object_description.hpp"

#include "cstdmf/unique_id.hpp"


BW_BEGIN_NAMESPACE

class UserDataObject;
class BinaryIStream;
class BinaryOStream;

class UserDataObjectType;
typedef SmartPointer<UserDataObjectType> UserDataObjectTypePtr;

typedef BW::map<const BW::string, UserDataObjectTypePtr> UserDataObjectTypes;
struct UserDataObjectInitData;

typedef BW::set<BW::string> UserDataObjectTypesInDifferentDomain;

#define USER_DATA_OBJECT_ATTR_STR "userDataObjects"

/**
 *	This class is used to represent an user data object type.
 */
class UserDataObjectType : public SafeReferenceCount
{
public:
	UserDataObjectType( const UserDataObjectDescription& description,
			PyTypeObject * pType );

	~UserDataObjectType();

	UserDataObject * newUserDataObject( const UniqueID & guid ) const;

	bool hasProperty( const char * attr ) const;

	//DataDescription * propIndex( int index ) const	{ return propDescs_[index];}
	//int propCount( ) const	{ return propDescs_.size();}
	PyTypeObject * pPyType() const 	{ return pPyType_; }
	void setPyType( PyTypeObject * pPyType );

	const char * name() const;

	/// @name static methods
	//@{
	static bool init();
	static bool load( UserDataObjectTypes& types, 
		UserDataObjectTypesInDifferentDomain& typesInDifferentDomain );	
	static void migrate( UserDataObjectTypes& types, 
		UserDataObjectTypesInDifferentDomain& typesInDifferentDomain );

	static void clearStatics();

	static UserDataObjectTypePtr getType( const char * className );
	static bool knownTypeInDifferentDomain( const char* className );

	static UserDataObjectTypes& getTypes();
	//@}
	const UserDataObjectDescription& description()  const { return description_; };


private:
	UserDataObjectDescription description_;
	PyTypeObject * pPyType_;

	BW::vector<DataDescription*>	propDescs_;

	static UserDataObjectTypes s_curTypes_;
	static UserDataObjectTypesInDifferentDomain s_typesInDifferentDomain_;
};

typedef SmartPointer<UserDataObjectType> UserDataObjectTypePtr;


#ifdef CODE_INLINE
#include "user_data_object_type.ipp"
#endif

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_TYPE_HPP

//user_data_object_type.hpp
