#pragma once
#ifndef DEPENDENCY_GRAPH_HPP
#define DEPENDENCY_GRAPH_HPP

#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_unordered_set.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class allows nodes to be added, dependencies set between those
 *	nodes, then an ordering calculated that will allow processing in correct
 *	dependency order.
 */
template<typename NodeKey>
class DependencyGraph
{
private:
	/**
	 *	Internal class representing a node on the graph.
	 */
	template<typename Key>
	class Node
	{
	public:
		Node( const Key & key )
			:
			key_( key)
			, numDependencies_( 0 )
		{
		}

		void addDependee( Node & node )
		{
			BW_GUARD;

			if ( dependees_.count( node.key_) == 0 )
			{
				dependees_.insert( node.key_ );
				++node.numDependencies_;
			}
		}

		void removeDependee( Node & node )
		{
			BW_GUARD;

			MF_ASSERT( dependees_.count( node.key_) != 0 );
			dependees_.erase( node.key_ );
			--node.numDependencies_;
			MF_ASSERT( node.numDependencies_ >= 0 );
		}

		int getNumDependencies() const
		{
			BW_GUARD;

			return numDependencies_;
		}

		const BW::unordered_set<Key> & getDependees() const
		{
			BW_GUARD;

			return dependees_;
		}

	private:
		///Key identifying this node uniquely.
		const Key key_;
		///The number of dependencies this node has on other nodes.
		int numDependencies_;
		///Keys for nodes depending on this node.
		BW::unordered_set<Key> dependees_;
	};

	typedef BW::unordered_map< NodeKey, Node<NodeKey> > GraphNodeMap;
	
public:
	void addNode( const NodeKey & key );
	void addDependency( const NodeKey & depending, const NodeKey & dependsOn );
	bool getOrder( BW::vector<NodeKey> & dependencyOrder ) const;
	typename GraphNodeMap::size_type size() const;
	void clear();

private:
	///Stores the graph nodes.
	GraphNodeMap nodes_;
};




//---Implementation Details---

/**
 *	@pre True.
 *	@post Added a node with the key @a key to the graph if one didn't
 *		already exist.
 */
template<typename NodeKey>
void DependencyGraph<NodeKey>::addNode(
	const NodeKey & key )
{
	BW_GUARD;

	if (nodes_.count( key ) == 0)
	{
		nodes_.insert( std::make_pair( key, Node<NodeKey>( key ) ) );
	}
}


/**
 *	@pre Nodes exist on the graph with the keys @a depending and @a dependsOn.
 *	@post Added a dependency of @a depending on @a dependsOn.
 */
template<typename NodeKey>
void DependencyGraph<NodeKey>::addDependency(
	const NodeKey & depending, const NodeKey & dependsOn )
{
	BW_GUARD;

	typename GraphNodeMap::iterator srchDepending = nodes_.find( depending );
	MF_ASSERT( srchDepending != nodes_.end() );
	typename GraphNodeMap::iterator srchDependsOn = nodes_.find( dependsOn );
	MF_ASSERT( srchDependsOn != nodes_.end() );
	
	srchDependsOn->second.addDependee( srchDepending->second );
}


/**
 *	@pre True.
 *	@post On true return: set @a dependency order to the order to traverse
 *			the nodes to satisfy dependencies.
 *		On false return: unable to sort. E.g., there's a cycle.
 */
template<typename NodeKey>
bool DependencyGraph<NodeKey>::getOrder(
	BW::vector<NodeKey> & dependencyOrder ) const
{
	BW_GUARD;

	dependencyOrder.clear();
	dependencyOrder.reserve( size() );
	
	//Do a topological sort.
	GraphNodeMap sortNodes( nodes_ );
	BW::vector<NodeKey> noDependencyKeys;
	noDependencyKeys.reserve( size() );
	for (typename GraphNodeMap::const_iterator itr = sortNodes.begin();
		itr != sortNodes.end();
		++itr)
	{
		if (itr->second.getNumDependencies() == 0)
		{
			noDependencyKeys.push_back( itr->first );
		}
	}

	BW::vector<typename GraphNodeMap::iterator> toRemove;
	toRemove.reserve( size() );
	while (!noDependencyKeys.empty())
	{
		const NodeKey key = noDependencyKeys.back();
		noDependencyKeys.pop_back();
		dependencyOrder.push_back( key );

		typename GraphNodeMap::iterator nodeItr = sortNodes.find( key );
		MF_ASSERT( nodeItr != sortNodes.end() );
		Node<NodeKey> & node = nodeItr->second;
		MF_ASSERT( node.getNumDependencies() == 0 );

		toRemove.clear();
		for (typename BW::unordered_set<NodeKey>::const_iterator depItr =
				node.getDependees().begin();
			depItr != node.getDependees().end();
			++depItr)
		{
			typename GraphNodeMap::iterator depNodeItr = sortNodes.find( *depItr );
			MF_ASSERT( depNodeItr != sortNodes.end() );
			MF_ASSERT( depNodeItr->first == *depItr );
			toRemove.push_back( depNodeItr );

			//If will have no dependencies once this one is removed.
			if ( depNodeItr->second.getNumDependencies() == 1 )
			{
				noDependencyKeys.push_back( depNodeItr->first );
			}
		}

		for (typename
			BW::vector<typename GraphNodeMap::iterator>::const_iterator itr =
				toRemove.begin();
			itr != toRemove.end();
			++itr)
		{
			node.removeDependee( (*itr)->second );
		}
	}

	//Should be no edges left now.
	for (typename GraphNodeMap::const_iterator itr = sortNodes.begin();
		itr != sortNodes.end();
		++itr)
	{
		if (itr->second.getNumDependencies() != 0)
		{
			return false;
		}
	}

	return true;
}


/**
 *	@return The number of nodes in the graph.
 */
template<typename NodeKey>
typename DependencyGraph<NodeKey>::GraphNodeMap::size_type
	DependencyGraph<NodeKey>::size() const
{
	BW_GUARD;

	return nodes_.size();
}


/**
 *	@pre True.
 *	@post Removed all nodes from the graph.
 */
template<typename NodeKey>
void DependencyGraph<NodeKey>::clear()
{
	BW_GUARD;

	nodes_.clear();
}

BW_END_NAMESPACE

#endif // DEPENDENCY_GRAPH_HPP
