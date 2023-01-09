#include "pch.hpp"
#include "node_catalogue_holder.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
NodeCatalogueHolder::NodeCatalogueHolder()
{
	new Moo::NodeCatalogue();
}


/**
 *	Destructor
 */
NodeCatalogueHolder::~NodeCatalogueHolder()
{
	delete Moo::NodeCatalogue::pInstance();
}

BW_END_NAMESPACE

// exporter_utility.cpp