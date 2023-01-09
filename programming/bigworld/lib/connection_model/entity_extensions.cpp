#include "pch.hpp"

#include "entity_extensions.hpp"

#include "bw_entity.hpp"
#include "entity_extension.hpp"
#include "entity_extension_factory_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EntityExtensions::EntityExtensions() :
	extensions_( NULL ),
	numExtensions_( 0 )
{
}


/**
 *	Destructor.
 */
EntityExtensions::~EntityExtensions()
{
	MF_ASSERT( extensions_ == NULL );
}


/**
 *	This method notifies and cleans up our collection of EntityExtensions
 */
void EntityExtensions::clear()
{
	for (int i = 0; i < numExtensions_; ++i)
	{
		extensions_[i]->onEntityDestroyed();
	}

	delete [] extensions_;

	extensions_ = NULL;
}


/**
 *
 */
void EntityExtensions::init( BWEntity & entity,
		EntityExtensionFactoryManager & factories )
{
	MF_ASSERT( (extensions_ == NULL) && (numExtensions_ == 0));

	numExtensions_ = static_cast<int>(factories.size());
	extensions_ = new EntityExtension *[ factories.size() ];

	for (int i = 0; i < numExtensions_; ++i)
	{
		extensions_[i] = entity.createExtension( factories[i] );
	}
}

BW_END_NAMESPACE

// entity_extensions.cpp
