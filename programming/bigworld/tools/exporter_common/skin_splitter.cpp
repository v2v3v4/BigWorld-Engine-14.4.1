#include "pch.hpp"

#include "cstdmf/guard.hpp"
#include "skin_splitter.hpp"

BW_BEGIN_NAMESPACE

/*
 *	This method creates a list of node indices by combining 
 *	relationships until the list contains up to a specified number of 
 *	nodes. Relationships that are added to the node list will be removed.
 *
 *	param nodeLimit the number of nodes allowed in the list
 *	param nodeList the returned list of node indices
 *	return false if a relationship contains too many nodes.
 */
bool SkinSplitter::createList(uint32 nodeLimit, BW::vector<uint32>& nodeList)
{
	BW_GUARD;
	// Grab the first relationship
	nodeList = relationships_.back();
	// if the relationship is bigger that nodeLimit, we have failed.
	if (nodeList.size() > nodeLimit )
		return false;
	relationships_.pop_back();

	// find a relationship that when combined with our current nodelist does not
	// exceed nodeLimit
	int index = 0;
	while((index = findAppropriateRelationship(nodeLimit, nodeList)) != -1)
	{
		// Get the appropriate relationship and add its nodes to our ndoeList
		const BoneRelationship& r = relationships_[index];
		for (uint i = 0; i < r.size(); i++)
		{
			if(std::find(nodeList.begin(), nodeList.end(), r[i]) == nodeList.end())
				nodeList.push_back( r[i] );
		}
		// Remove the added relationship.
		relationships_.erase( relationships_.begin() + index );
	}
	return true;
}


/*
 *	This method tries to find a relationship that can be combined with the current nodelist
 *	and not make the nodelist go over nodeLimit. It tries to find the most suitable
 *	relationship to combine with ie the one that will increase bonecount the least.
 *	param nodeLimit the max number of nodes in the nodelist
 *	param nodeList the current list of nodes
 *	return the index of the most appropriate relationship to combine with or -1 if none found.
 */
int	SkinSplitter::findAppropriateRelationship(
	uint32 nodeLimit, 
	const BW::vector<uint32>& nodeList) const
{
	BW_GUARD;
	BW::vector<uint32>::const_iterator b = nodeList.begin();
	BW::vector<uint32>::const_iterator e = nodeList.end();
	int index = -1;
	uint diff = static_cast<uint>(nodeLimit - nodeList.size() + 1);
	// iterate over the relationships to find an appropriate one.
	for (uint i = 0; i < relationships_.size(); i++)
	{
		uint curDiff = 0;
		const BoneRelationship& br = relationships_[i];
		for (uint32 j = 0; j < br.size(); j++)
		{
			if( std::find( b, e, br[j] ) == e )
				curDiff++;
		}
		if (curDiff < diff)
		{
			diff = curDiff;
			index = int(i);
		}
	}

	return index;
}


/*
 *	This method finds a relationship completely containing another one.
 *	param relationship the relationship to find a match for.
 *	param perfectMatch only find relationships that match exactly
 *  return the index of the found relationship or -1 if none found
 */
int SkinSplitter::findRelationship( 
	const BoneRelationship& relationship, 
	bool perfectMatch )
{
	BW_GUARD;
	// iterate over all relationships
	for (uint i = 0; i < relationships_.size(); i++)
	{
		if (relationships_[i].size() == relationship.size() || !perfectMatch)
		{
			// compare each element of the relationship to see if it can be found
			BoneRelationship::iterator it = relationships_[i].begin();
			BoneRelationship::iterator end = relationships_[i].end();
			bool success = true;
			for (uint32 j = 0; j < relationship.size(); j++)
			{
				if (std::find( it, end, relationship[j]) == end)
				{
					success = false;
					j = static_cast<uint32>(relationship.size());
				}
			}
			if (success)
				return i;
		}
	}
	return -1;
}

BW_END_NAMESPACE

