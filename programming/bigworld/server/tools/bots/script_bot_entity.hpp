#ifndef SCRIPT_BOT_ENTITY_HPP
#define SCRIPT_BOT_ENTITY_HPP

#include "entity.hpp"

#include "entity_type.hpp"

#include "connection_model/bw_entity.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/smartpointer.hpp"
#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

class ClientApp;
class PyEntity;

typedef ScriptObjectPtr< PyEntity > PyEntityPtr;

class ScriptBotEntity : public Entity
{
public:
	ScriptBotEntity( const ClientApp & clientApp, const EntityType & type );
	~ScriptBotEntity();

	  /* Entity overrides */
	  ScriptObject pPyEntity() const;

private:
	/* BWEntity overrides */
	// Method and property events
	void onMethod( int methodID, BinaryIStream & data );
	void onProperty( int propertyID, BinaryIStream & data,
		bool isInitialising );
	void onNestedProperty( BinaryIStream & data, bool isSlice,
		bool isInitialising );

	// Lifecycle events
	bool initCellEntityFromStream( BinaryIStream & data );
	bool initBasePlayerFromStream( BinaryIStream & data );
	bool initCellPlayerFromStream( BinaryIStream & data );
	bool restorePlayerFromStream( BinaryIStream & data );
	void onBecomePlayer();
	void onBecomeNonPlayer();
	void onDestroyed();

	/* Entity data */
	PyEntityPtr pPyEntity_;
};

BW_END_NAMESPACE

#endif // SCRIPT_BOT_ENTITY_HPP
