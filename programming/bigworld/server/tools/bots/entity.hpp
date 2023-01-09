#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "entity_type.hpp"

#include "connection_model/bw_entity.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/smartpointer.hpp"
#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

class ClientApp;
class PyEntity;

typedef ScriptObjectPtr< PyEntity > PyEntityPtr;

class Entity : public BWEntity
{
public:
	Entity( const ClientApp & clientApp, const EntityType & type );
	~Entity();

	const EntityType & type() const				{ return type_; }
	virtual ScriptObject pPyEntity() const = 0;

	const ClientApp & getClientApp() const		{ return clientApp_; }

private:
	/* BWEntity overrides */
	int getMethodStreamSize( int methodID ) const;
	int getPropertyStreamSize( int propertyID ) const;

	// Accessors
	const BW::string entityTypeName() const;

	// Lifecycle events
	void onEnterWorld();
	void onChangeControl( bool isControlling, bool isInitialising );

	/* Entity data */
	const EntityType & type_;
	const ClientApp & clientApp_;
};


typedef SmartPointer< Entity > EntityPtr;


BW_END_NAMESPACE

#endif // ENTITY_HPP
