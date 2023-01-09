#ifndef ENTITY_TYPE_HPP
#define ENTITY_TYPE_HPP

#include "Python.h"

#include "cstdmf/bw_vector.hpp"

#include "cstdmf/stdmf.hpp"
#include "entitydef/entity_description_map.hpp"


BW_BEGIN_NAMESPACE

class EntityDefConstants;

BW_END_NAMESPACE


/**
 *	This class maintains an entity template, as constructed from
 *	its definition file. It has a static variable that contains
 *  a vector of all defined entity types, and static variable that
 *  contains a map of all defined entity descriptions.
 */
class EntityType
{
private:
	EntityType( const BW::EntityDescription & description,
				PyTypeObject * pType );

public:
	~EntityType();

	static bool init( const BW::string & clientPath );
	static bool fini();

	static const BW::EntityDefConstants & entityDefConstants();

	static EntityType * find( BW::EntityTypeID entityTypeID );
	static EntityType * find( const BW::string & name );

	PyObject * newDictionary( BW::DataSectionPtr pSection = NULL ) const;
	PyObject * newDictionary( BW::BinaryIStream & stream,
								int dataDomains ) const;
	// This expects a tagged client property stream.
	PyObject * newDictionary( BW::BinaryIStream & stream ) const;

	const BW::string & name() const { return description_.name(); }
	const BW::EntityDescription & description() const { return description_; }
	PyTypeObject * pType() const { return pType_; }

private:
	typedef BW::vector< EntityType * > Types;
	static Types		types_;
	static BW::EntityDescriptionMap entityDescriptionMap_;

	const BW::EntityDescription & description_;
	PyTypeObject * pType_;
};



#endif // ENTITY_TYPE_HPP
