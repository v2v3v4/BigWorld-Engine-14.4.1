#ifndef META_DATA_TYPE_HPP
#define META_DATA_TYPE_HPP

#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
class DataType;

typedef SmartPointer< DataSection > DataSectionPtr;
typedef SmartPointer< DataType > DataTypePtr;


/**
 *	This class is the base class for objects that are used to create data types.
 */
class MetaDataType
{
public:
	MetaDataType() {}			// call addMetaType in your constructor
	virtual ~MetaDataType()	{}	// call delMetaType in your destructor

	static MetaDataType * find( const BW::string & name );

	static void fini();

	/**
	 *	This virtual method should return your basic meta type name
	 */
	virtual const char * name() const = 0;

	/**
	 *	This virtual method is used in the creation of DataTypes. Once a
	 *	metatype is found for the current data section, it is asked for the type
	 *	associated with that data section.
	 *
	 *	Derived MetaDataTypes should override this method to return their data
	 *	type.
	 */
	virtual DataTypePtr getType( DataSectionPtr pSection ) = 0;

	static void addAlias( const BW::string& orig, const BW::string& alias );

protected:
	static void addMetaType( MetaDataType * pMetaType );
	static void delMetaType( MetaDataType * pMetaType );

private:
	typedef BW::map< BW::string, MetaDataType * > MetaDataTypes;
	static MetaDataTypes * s_metaDataTypes_;
};

BW_END_NAMESPACE

#endif // META_DATA_TYPE_HPP
