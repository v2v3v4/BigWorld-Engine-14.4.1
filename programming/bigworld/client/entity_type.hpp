#ifndef ENTITY_TYPE_HPP
#define ENTITY_TYPE_HPP

#include "cstdmf/bw_vector.hpp"

#include "Python.h"	// forward declaration doesn't work :(

#include "cstdmf/stdmf.hpp"

#include "entitydef/entity_description_map.hpp"


BW_BEGIN_NAMESPACE

class EntityDefConstants;

/**
 *	This class maintains an entity template, as constructed from
 *	its definition file and python scripts.
 *
 *	It is used to create new python class objects and to retrieve
 *	static entity type information.
 */
class EntityType
{
private:
	EntityType( const EntityDescription & description,
		PyObject * pModule,
		PyTypeObject * pClass );

public:
	~EntityType();			// our destructor should be too, but VC++ doesn't like it

	static bool init();
	static bool reload();
	static void fini();

	static EntityType * find( uint type );
	static EntityType * find( const BW::string & name );

	static PyObject * getPreloads();

	enum StreamContents { BASE_PLAYER_DATA, CELL_PLAYER_DATA,
		TAGGED_CELL_PLAYER_DATA, TAGGED_CELL_ENTITY_DATA };

	PyObject * newDictionary( DataSectionPtr pSection = NULL,
		bool includeOwnClient = false ) const;
	PyObject * newDictionary( BinaryIStream & stream,
		StreamContents contents ) const;

	int index() const					{ return description_.clientIndex(); }
	const BW::string & name() const	{ return description_.name(); }

	const EntityDescription & description() const	{ return description_; }

	PyTypeObject * pClass() const		{ return pClass_; }
	PyTypeObject * pPlayerClass() const;

	static const EntityDefConstants & entityDefConstants()
	{
		return s_entityDefConstants_;
	}

	void prepareCreationStream( MemoryOStream& outputStream,
		const DataSectionPtr& pProps ) const;
	void prepareCreationStream( MemoryOStream & outputStream,
		const ScriptDict & props ) const;
	void prepareBasePlayerStream( MemoryOStream & outputStream,
		const ScriptDict & props ) const;
	void prepareCellPlayerStream( MemoryOStream & outputStream,
		const ScriptDict & props ) const;

private:
	static BW::vector<EntityType*>				s_types_;
	static BW::map<BW::string,EntityTypeID>	s_typeNames_;

	static EntityDefConstants s_entityDefConstants_;

	void swizzleClass( PyTypeObject *& pOldClass );

	const EntityDescription & description_;
	PyObject * pModule_;
	PyTypeObject * pClass_;
	mutable PyTypeObject * pPlayerClass_;
};

BW_END_NAMESPACE


#endif // ENTITY_TYPE_HPP
