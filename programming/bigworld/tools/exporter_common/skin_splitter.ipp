BW_BEGIN_NAMESPACE

/*
 *	Constructor for the SkinSplitter class.
 *	param indices is the indices to build the relationship list from
 *	param vertices is the vertices to build the relationship list from
 *	param boneVertices is the bone vertices to build the relationship list from
 */
template<
typename TriangleVector, 
typename BloatVertexVector, 
typename BoneVertexVector>
SkinSplitter::SkinSplitter(
	const TriangleVector& triangles, 
	const BloatVertexVector& vertices, 
	const BoneVertexVector& boneVertices )
{
	BW_GUARD;
	// Build relationship list.
	// For each triangle add the relationship between its bones
	// to the relationship list.
	for (size_t i = 0; i < triangles.size(); ++i)
	{
		const VisualMesh::Triangle& tri = triangles[i];
		addRelationship( 
			boneVertices[vertices[tri.index[0]].vertexIndex], 
			boneVertices[vertices[tri.index[1]].vertexIndex], 
			boneVertices[vertices[tri.index[2]].vertexIndex] );
	}

	// For each relationship in the relationship list,
	// see if the relationship is fully contained within
	// another relationship, if so, get rid of the relationship
	// that is fully contained in another one.
	size_t relationshipCount = relationships_.size();
	for (size_t i = 0; i < relationshipCount; i++)
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
 * check if a vertexes indices is used by the current nodes.
 * param v the vertex to check
 * param indexUsed the list of boneIndices to check, the bone 
 * indices are set to true for a bone that is used and false for 
 * one that is not.
 * return true if success
 */
template <typename BoneVertex>
bool SkinSplitter::checkIndex( const BoneVertex& v,
			const BW::vector<bool>& indexUsed )
{
	BW_GUARD;

	float weights[3] = { 0.f, 0.f, 0.f };
	int indices[3] = { 0, 0, 0 };

	getWeights( v, weights );
	getIndices( v, indices );

	for (size_t i = 0; i < 3;++i)
	{
		if (weights[i] > 0.f && !indexUsed[indices[i]])
			return false;
	}
	return true;
}


/*
 * This method moves the triangles that reference the
 * current bonelist to a separate list of triangle.
 * param triangles the full triangle list
 * param splitTriangles the triangles that reference the current bone list.
 * param boneIndices the indices of the bones to check against.
 * vertices the vertices
 * boneVertices the bone vertices
 * numBones the total number of bones used pre split
 */
template <
typename TriangleVector, 
typename BloatVertexVector, 
typename BoneVertexVector>
void SkinSplitter::splitTriangles( 
	TriangleVector& triangles, 
	TriangleVector& splitTriangles,
	const BW::vector<uint32>& boneIndices,
	const BloatVertexVector& vertices,
	const BoneVertexVector& boneVertices, size_t numBones )
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
	VisualMesh::TriangleVector::const_iterator triIt = triangles.begin();

	while (triIt != triangles.end())
	{
		if (checkIndex(boneVertices[vertices[triIt->index[0]].vertexIndex], indexUsed) &&
			checkIndex(boneVertices[vertices[triIt->index[1]].vertexIndex], indexUsed) &&
			checkIndex(boneVertices[vertices[triIt->index[2]].vertexIndex], indexUsed))
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
 *	This method adds unique node relationships based on the triangle described
 *	by the inputs
 *	param a, b, c the three indices describing a triangle
 *	pVHolder the vertices
 */
template <typename BoneVertex>
void SkinSplitter::addRelationship( 
	const BoneVertex& v1, 
	const BoneVertex& v2,				   
	const BoneVertex& v3 )
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
 *	This method adds the relationship for a vertex to a relationship list.
 *	param relationship the relationship to add to
 *	param v the vertex to add the relationship for
 */
template <typename BoneVertex>
void SkinSplitter::addRelationship( 
	BoneRelationship& relationship, 
	const BoneVertex& v )
{
	BW_GUARD;

	float weights[3] = { 0.f, 0.f, 0.f };
	int indices[3] = { 0, 0, 0 };

	getWeights( v, weights );
	getIndices( v, indices );

	for (size_t i = 0; i < 3; ++i)
	{
		if (weights[i] > 0.f)
		{
			if( std::find( relationship.begin(), relationship.end(),
				indices[i]) == relationship.end() )
			{
				relationship.push_back( indices[i] );
			}
		}
	}
}

BW_END_NAMESPACE

