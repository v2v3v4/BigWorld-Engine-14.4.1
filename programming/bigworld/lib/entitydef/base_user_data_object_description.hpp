/**
 * 	@file
 *
 *	This file provides the implementation of the base class from which
 *  entities defined with a def derived.
 *
 * 	@ingroup udo
 */

#ifndef BASE_USER_DATA_OBJECT_DESCRIPTION_HPP
#define BASE_USER_DATA_OBJECT_DESCRIPTION_HPP

#include "script/script_object.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stringmap.hpp"
#include <float.h>

#include "data_description.hpp"
#include "method_description.hpp"
#include "network/basictypes.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class MD5;

class AddToStreamVisitor;
/**
 *	This class is used to describe a type of User Data Object. It describes all properties
 *	a chunk item. It is normally created on startup when the user data objects.xml file is parsed.
 *
 * 	@ingroup udo
 */
class BaseUserDataObjectDescription
{
public:
	BaseUserDataObjectDescription();
	virtual ~BaseUserDataObjectDescription();

	// TODO: Move this to UserDataObjectDescription
	bool	parse( const BW::string & name,
				DataSectionPtr pSection = NULL );

	void addToDictionary( DataSectionPtr pSection, ScriptDict sDict ) const;

	BWENTITY_API const BW::string&		name() const;


	BWENTITY_API unsigned int		propertyCount() const;
	BWENTITY_API DataDescription*	property( unsigned int n ) const;
	BWENTITY_API DataDescription*	findProperty(
		const char * name, const char * component = "" ) const;
	
	DataDescription* findCompoundProperty( const char * name ) const;

protected:

	virtual	bool			parseProperties( DataSectionPtr pProperties,
										const BW::string & componentName ) = 0;
	virtual bool			parseInterface( DataSectionPtr pSection,
										const char * interfaceName,
										const BW::string & componentName );
	virtual bool			parseImplements( DataSectionPtr pInterfaces,
										const BW::string & componentName );

	virtual const BW::string getDefsDir() const = 0;
	
	

	BW::string			name_;

	typedef BW::vector< DataDescription >		Properties;
	Properties 			properties_;

	typedef StringMap< unsigned int > PropertyMap;

	PropertyMap & getComponentProperties( const char * component );

#ifdef EDITOR_ENABLED
	BW::string			editorModel_;
#endif
	
private:
	typedef StringMap< PropertyMap >  ComponentPropertyMap;
	ComponentPropertyMap propertyMap_;
};

#ifdef CODE_INLINE
#include "base_user_data_object_description.ipp"
#endif

BW_END_NAMESPACE

#endif // BASE_USER_DATA_OBJECT_DESCRIPTION_HPP

