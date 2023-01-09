#ifndef ENTITY_TYPE_HPP
#define ENTITY_TYPE_HPP

#include "Python.h"

#include "cstdmf/bw_vector.hpp"

#include "connection/entity_def_constants.hpp"
#include "cstdmf/stdmf.hpp"
#include "entitydef/entity_description_map.hpp"


BW_BEGIN_NAMESPACE

class ClientApp;


/**
 *	This class maintains an entity template, as constructed from
 *	its definition file. It has a static variable that contains
 *  a vector of all defined entity types, and static variable that
 *  contains a map of all defined entity descriptions.
 */
class EntityType
{
private:
	EntityType( const EntityDescription & description, const ScriptType & type );

public:
	// our destructor should be too, but VC++ doesn't like it
	~EntityType();

	static bool init( const BW::string & standinEntity );
	static bool fini();

	static EntityDefConstants entityDefConstants();

	static EntityType * find( uint type );
	static EntityType * find( const BW::string & name );

	enum StreamContents { BASE_PLAYER_DATA, CELL_PLAYER_DATA,
		TAGGED_CELL_PLAYER_DATA, TAGGED_CELL_ENTITY_DATA };

	PyObject * newDictionary( DataSectionPtr pSection = NULL,
			bool isPlayer = false ) const;
	PyObject * newDictionary( BinaryIStream & stream,
		StreamContents contents ) const;

	int index() const								{ return description_.clientIndex(); }
	const BW::string name() const					{ return description_.name(); }
	const EntityDescription & description() const	{ return description_; }
	ScriptType type() const							{ return type_; }

private:
	static BW::vector< EntityType * > s_types_;
	static EntityDescriptionMap entityDescriptionMap_;

	const EntityDescription & description_;
	ScriptType type_;
};

BW_END_NAMESPACE

#endif // ENTITY_TYPE_HPP
