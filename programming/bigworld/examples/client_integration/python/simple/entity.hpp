#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "connection_model/bw_entity.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class PyEntity;

BW_END_NAMESPACE


class EntityType;


class Entity : public BW::BWEntity
{
public:
	Entity( const EntityType & type, BW::BWConnection * pBWConnection );
	virtual ~Entity();

	// BWEntity implementation
	virtual void onMethod( int methodID, BW::BinaryIStream & data );
	virtual void onProperty( int propertyID, BW::BinaryIStream & data,
		   bool isInitialising );
	virtual void onNestedProperty( BW::BinaryIStream & data, bool isSlice,
			bool isInitialising );

	virtual int getMethodStreamSize( int methodID ) const;
	virtual int getPropertyStreamSize( int propertyID ) const;

	virtual const BW::string entityTypeName() const;

	// Entity interface
	const EntityType & type() const { return type_; }

	BW::PyEntity * pPyEntity() { return pPyEntity_; }

protected:
	// Overrides from BWEntity.
	virtual bool initCellEntityFromStream( BW::BinaryIStream & data );
	virtual bool initBasePlayerFromStream( BW::BinaryIStream & data );
	virtual bool initCellPlayerFromStream( BW::BinaryIStream & data );
	virtual bool restorePlayerFromStream( BW::BinaryIStream & data );

private:
	void onDestroyed();

	const EntityType & type_;
	BW::PyEntity * pPyEntity_;
};


#endif // ENTITY_HPP
