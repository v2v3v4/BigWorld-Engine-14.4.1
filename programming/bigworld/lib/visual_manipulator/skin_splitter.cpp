#include "pch.hpp"

#include "cstdmf/guard.hpp"
#include "skin_splitter.hpp"


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;

/*
 *	Constructor for this class.
 *	param indices is the indices to build the relationship list from
 *	param pVHolder is the vertices to build the relationship list from
 */
SkinSplitter::SkinSplitter( const TriangleVector& triangles, 
	const VertexVector& vertices )
{
	BW_GUARD;
	// Build relationship list.
	// For each triangle add the relationship between its bones
	// to the relationship list.
	for (size_t i = 0; i < triangles.size(); ++i)
	{
		const Triangle& tri = triangles[i];
		addRelationship( vertices[tri.a], vertices[tri.b], vertices[tri.c] );
	}

	// For each relationship in the relationship list,
	// see if the relationship is fully contained within
	// another relationship, if so, get rid of the relationship
	// that is fully contained in another one.
	uint32 relationshipCount = static_cast<uint32>(relationships_.size());
	for (uint32 i = 0; i < relationshipCount; i++)
	{
		BoneRelationship br = relationships_.front();
		relationships_.erase( relationships_.begin() );
		if (findRelationship( br, false ) == -1)
		{
			relationships_.push_back( br );
		}
	}
}


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
 * check if a vertex index is used by the current nodes.
 * param pVH the vertices to check
 * param indexUsed, the list of boneIndices to check, the bone 
 * indices are set to true for a bone that is used and false for 
 * one that is not.
 * param ind the index to check
 * return true if success
 */
bool SkinSplitter::checkIndex( const BloatVertex& v,
			const BW::vector<bool>& indexUsed )
{
	BW_GUARD;
	for (size_t i = 0; i < 4;++i)
	{
		if (v.weights[i] > 0.f && !indexUsed[v.indices[i]])
			return false;
	}
	return true;
}


/*
 * This method moves the indices that have triangles that reference the
 * current bonelist to a separate list of indices.
 * param indices the full index list
 * param splitIndices the indices that reference the current bone list.
 * param boneIndices the indices of the bones to check again.
 * pVHolder the vertices
 */
void SkinSplitter::splitTriangles( TriangleVector& triangles, TriangleVector& splitTriangles,
	const BW::vector<uint32>& boneIndices, const VertexVector& vertices, size_t numBones )
{
	BW_GUARD;
	// create a index list for the nodes, true means the node is included in the list,
	// false means not included.
	BW::vector<bool> indexUsed( numBones, false );
	for (uint32 i = 0; i < boneIndices.size(); i++)
	{
		indexUsed[boneIndices[i]] = true;
	}

	// Iterate over our triangles, if all the vertices in the triangle 
	// are affected by all the bones, move them to the split list.

	TriangleVector::const_iterator triIt = triangles.begin();

	while (triIt != triangles.end())
	{
		if (checkIndex(vertices[triIt->a], indexUsed) &&
			checkIndex(vertices[triIt->b], indexUsed) &&
			checkIndex(vertices[triIt->c], indexUsed))
		{
			splitTriangles.push_back( *triIt );
			triIt = triangles.erase( triIt );
		}
		else
		{
			triIt++;
		}
	}
}


/*
 *	This method tries to find a relationship that can be combined with the current nodelist
 *	and not make the nodelist go over nodeLimit. It tries to find the most suitable
 *	relationship to combine with ie the one that will increase bonecount the least.
 *	param nodeLimit the max number of nodes in the nodelist
 *	param nodeList the current list of nodes
 *	return the index of the most appropriate relationship to combine with or -1 if none found.
 */
int	SkinSplitter::findAppropriateRelationship(uint32 nodeLimit, const BW::vector<uint32>& nodeList) const
{
	BW_GUARD;
	BW::vector<uint32>::const_iterator b = nodeList.begin();
	BW::vector<uint32>::const_iterator e = nodeList.end();
	int index = -1;
	uint diff = nodeLimit - static_cast<uint>(nodeList.size() + 1);
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
 *	This method adds unique node relationships based on the triangle described
 *	by the inputs
 *	param a, b, c the three indices describing a triangle
 *	pVHolder the vertices
 */
void SkinSplitter::addRelationship( const BloatVertex& v1, const BloatVertex& v2, 
								   const BloatVertex& v3 )
{
	BW_GUARD;
	// Create the relationship
	BoneRelationship relationship;
	addRelationship( relationship, v1 );
	addRelationship( relationship, v2 );
	addRelationship( relationship, v3 );

	// If this relationship has not been defined already add it to the list.
	int relindex = findRelationship( relationship );
	if (relindex < 0)
	{
		relationships_.push_back( relationship );
	}
}


/*
 *	This method finds a relationship completely containing another one.
 *	param relationship the relationship to find a match for.
 *	param perfectMatch only find relationships that match exactly
 *  return the index of the found relationship or -1 if none found
 */
int SkinSplitter::findRelationship( const BoneRelationship& relationship, bool perfectMatch )
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
					j = static_cast<uint>(relationship.size());
				}
			}
			if (success)
				return i;
		}
	}
	return -1;
}


/*
 *	This method adds the relationship for a vertex to a relationship list.
 *	param relationship the relationship to add to
 *	param v the index of the vertex to add the relationship for
 *	param pVHolder the vertices
 */
void SkinSplitter::addRelationship( BoneRelationship& relationship, const BloatVertex& v )
{
	BW_GUARD;
	for (size_t i = 0; i < 4; ++i)
	{
		if (v.weights[i] > 0.f)
		{
			if( std::find( relationship.begin(), relationship.end(),
				v.indices[i]) == relationship.end() )
			{
				relationship.push_back( v.indices[i] );
			}
		}
	}
}

BW_END_NAMESPACE
