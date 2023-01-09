#include "pch.hpp"
#include "node_catalogue.hpp"


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( Moo::NodeCatalogue );

namespace Moo
{


/**
 *	Static method to find or add a node in the node catalogue. 
 *	It implicitly locks the node catalogue, so you don't need to manually
 *	lock it before using this method.
 *
 *	@param	pNode			Source Node to use to find / add to the catalogue.
 *	@return	A pointer to the Node that was found / added. This method should
 *		never return NULL. This pointer is not the same as the pNode argument
 *		pointer.
 */
Node * NodeCatalogue::findOrAdd( const NodePtr& pNode )
{
	BW_GUARD;
	Node * pStorageNode = NULL;

	SimpleMutexHolder smh( instance().nodeCatalogueLock_ );

	NodeCatalogue::iterator found =
		instance().StringHashMap<NodePtr>::find(
			pNode->identifier().c_str() );

	if (found != instance().end())
	{
		pStorageNode = found->second.getObject();
	}
	else
	{
		pStorageNode = new Node();
		pStorageNode->identifier( pNode->identifier() );

		instance().insert( std::make_pair(
			pStorageNode->identifier(),
			pStorageNode ) );
	}

	//Note: the pStorageNode will be always the first loaded node from .visual
	//files, ie. if a.visual and b.visual both has "biped" node, then pStorageNode
	//will be the node loaded from a.visual if a.visual's "biped" is ever the first
	//node loaded with this name "biped".
	return pStorageNode;
}


/**
 *	Static method to find a node by name in the Model node catalogue.
 *	Note that this method hides the StringHashMap implementation of find.
 *	It implicitly locks the node catalogue, so you don't need to manually
 *	lock it before using this method.
 *
 *	@param	identifier		Name of the node to find.	
 *	@return NodePtr			Existing Node, or NULL if a node by that name 
 *							does not yet exist.
 */
NodePtr NodeCatalogue::find( const char * identifier )
{
	BW_GUARD;
	SimpleMutexHolder smh( instance().nodeCatalogueLock_ );

	NodeCatalogue::iterator found = 
		instance().StringHashMap<NodePtr>::find( identifier );

	if (found != instance().end()) return found->second;

	return NULL;
}


/**
 *	Static method to grab the lock for the node catalogue.
 *	The node catalogue must be locked when accessing it directly,
 *	e.g. through the [] operator.
 */
void NodeCatalogue::grab()
{
	BW_GUARD;
	instance().nodeCatalogueLock_.grab();
}


/**
 *	Static method to release the lock for the node catalogue.
 *	The node catalogue must be locked when accessing it directly,
 *	e.g. through the [] operator.
 */
void NodeCatalogue::give()
{
	instance().nodeCatalogueLock_.give();
}


} //namespace Moo

BW_END_NAMESPACE

// node_catalogue.cpp
