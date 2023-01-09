/**
 * 	@file
 *
 *	This file provides the implementation of the UserDataObjectDescription class.
 */
#ifndef USER_DATA_OBJECT_DESCRIPTION_HPP
#define USER_DATA_OBJECT_DESCRIPTION_HPP

#include <float.h>

#include "data_description.hpp"
#include "method_description.hpp"
#include "resmgr/datasection.hpp"
#include "base_user_data_object_description.hpp"


BW_BEGIN_NAMESPACE

class MD5;

class AddToStreamVisitor;
/**
 *	This class is used to describe a type of User Data Object. It describes all properties of
 *	the data object. It is normally created on startup when the user_data_objects.xml file is parsed.
 */
class UserDataObjectDescription: public BaseUserDataObjectDescription
{
public:
	UserDataObjectDescription():
		domain_(NONE)
	{}

	/* A user data object only lives on one of the base, cell or client.
	   Accordingly any DATA_OBJECT links on entities should have the appropriate flag set
	   so that it can be read in the appropriate domain. */

	enum UserDataObjectDomain{
		NONE = 0x0,
		BASE = 0x1,
		CELL = 0x2,
		CLIENT =0x4,
	};
	UserDataObjectDomain domain() const{ return domain_;	}

protected:
	bool parseProperties( DataSectionPtr pProperties,
			const BW::string & componentName );
	bool parseInterface( DataSectionPtr pSection, const char * interfaceName,
			const BW::string & componentName );

	const BW::string	getDefsDir() const;
	const BW::string	getClientDir() const;
	const BW::string	getCellDir() const;
	const BW::string	getBaseDir() const;

	/* This will be specified in every UserDataObject def file
	 * And interpreted by the chunkloading sequence 
	 * for each domain to determine if it should load a given user data object
	 * TODO: DEFAULT TO 0, and print error 
	 */
	UserDataObjectDomain domain_;
};

BW_END_NAMESPACE

#endif // UserDataObject_DESCRIPTION_HPP

