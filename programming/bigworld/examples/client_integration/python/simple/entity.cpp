#include "pch.hpp"

#include "entity.hpp"
#include "entity_type.hpp"
#include "py_entity.hpp"

#include "common/simple_client_entity.hpp"

#include "network/msgtypes.hpp"

#include "pyscript/pyobject_base.hpp"

#if defined( __GNUC__ )
#include <tr1/type_traits>
#else /* defined( __GNUC__ ) */
#include <type_traits>
#endif /* defined( __GNUC__ ) */

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


/**
 *	Constructor.
 */
Entity::Entity( const EntityType & type, BW::BWConnection * pBWConnection ) :
		BWEntity( pBWConnection ),
		type_( type ),
		pPyEntity_( NULL )
{
	BW_STATIC_ASSERT( std::tr1::is_polymorphic< BW::PyEntity >::value == false,
		PyEntity_is_virtual_but_uses_PyType_GenericAlloc );

	PyObject * pObject = PyType_GenericAlloc( type.pType(), 0 );
	pPyEntity_ = new (pObject) BW::PyEntity( *this );
}


/**
 *	Destructor.
 */
Entity::~Entity()
{
	// onDestroyed must happen between construction and destruction
	MF_ASSERT( pPyEntity_ == NULL );
}


/**
 *	This method is called when a message is received from the server telling us
 *	to call a method on this entity.
 */
void Entity::onMethod( int methodID, BW::BinaryIStream & data )
{
	BW::SimpleClientEntity::methodEvent(
		BW::ScriptObject( pPyEntity_, BW::ScriptObject::FROM_BORROWED_REFERENCE ),
		type_.description(), methodID, data );
}


/**
 *	This method is called when a message is received from the server telling us
 *	to change a property on this entity.
 */
void Entity::onProperty( int propertyID, BW::BinaryIStream & data,
		bool isInitialising )
{
	BW::SimpleClientEntity::propertyEvent(
		BW::ScriptObject( pPyEntity_, BW::ScriptObject::FROM_BORROWED_REFERENCE ),
		type_.description(), propertyID, data, !isInitialising );
}


/**
 *	This method is called when the server wants to update a nested property.
 */
void Entity::onNestedProperty( BW::BinaryIStream & data, bool isSlice,
		bool isInitialising )
{
	BW::SimpleClientEntity::nestedPropertyEvent(
		BW::ScriptObject( pPyEntity_, BW::ScriptObject::FROM_BORROWED_REFERENCE ),
		type_.description(), data, !isInitialising, isSlice );
}


/**
 *	This method is called when the network layer needs to know how big a given
 *	method message is.
 */
int Entity::getMethodStreamSize( int methodID ) const
{
	const BW::MethodDescription * pMethodDescription =
			type_.description().client().exposedMethod( methodID );

	IF_NOT_MF_ASSERT_DEV( pMethodDescription )
	{
		return -2;
	}

	return pMethodDescription->streamSize( true );
}


/**
 *	This method is called when the network layer needs to know how big a given
 *	property message is.
 */
int Entity::getPropertyStreamSize( int propertyID ) const
{
	return type_.description().clientServerProperty( propertyID )->streamSize();
}


const BW::string Entity::entityTypeName() const
{
	return type_.name();
}


bool Entity::initCellEntityFromStream( BW::BinaryIStream & data )
{
	return pPyEntity_->initFromStream( data );
}


bool Entity::initBasePlayerFromStream( BW::BinaryIStream & data )
{
	return pPyEntity_->initBasePlayerFromStream( data );
}


bool Entity::initCellPlayerFromStream( BW::BinaryIStream & data )
{
	return pPyEntity_->initCellPlayerFromStream( data );
}


bool Entity::restorePlayerFromStream( BW::BinaryIStream & data )
{
	return BW::SimpleClientEntity::resetPropertiesEvent(
		BW::ScriptObject( pPyEntity_,
			BW::ScriptObject::FROM_BORROWED_REFERENCE ),
		type_.description(), data );
}


void Entity::onDestroyed()
{
	MF_ASSERT( pPyEntity_ != NULL );

	pPyEntity_->onEntityDestroyed();

	Py_DECREF( pPyEntity_ );
	pPyEntity_ = NULL;
}


// entity.cpp
