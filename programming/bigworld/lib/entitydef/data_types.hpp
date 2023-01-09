/**
 * 	@file data_types.hpp
 *
 *	Concrete data types as referred to by DataDescriptions
 *
 *	@ingroup entity
 */

#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include "data_description.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a simple meta data type that holds a single DataType
 *	object and does no parsing of the contents of its &lt;Type> sections.
 */
template <class DATATYPE>
class SimpleMetaDataType : public MetaDataType
{
public:
	SimpleMetaDataType( const char * name ) :
		name_( name )
	{
		MetaDataType::addMetaType( this );
	}
	virtual ~SimpleMetaDataType()
	{
		MetaDataType::delMetaType( this );
	}

	virtual const char * name() const
	{
		return name_.c_str();
	}

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		return new DATATYPE( this );
	}

private:
	BW::string name_;
};


#define DATA_TYPE_LINK_ITEM( NAME )									\
	int force_link_##NAME = 0;										\


/**
 *	A simple macro used to register simple data types. That is, types that do
 *	not have a meta data type.
 */
#define SIMPLE_DATA_TYPE( TYPE, NAME )								\
	SimpleMetaDataType< TYPE > s_##NAME##_metaDataType( #NAME );	\
	DATA_TYPE_LINK_ITEM( NAME )										\

BW_END_NAMESPACE

#endif // DATA_TYPES_HPP

// data_types.hpp
