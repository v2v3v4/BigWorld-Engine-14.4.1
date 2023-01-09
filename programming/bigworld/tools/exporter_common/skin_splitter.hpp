#ifndef SKIN_SPLITTER_HPP
#define SKIN_SPLITTER_HPP

BW_BEGIN_NAMESPACE

typedef BW::vector<uint32> BoneRelationship;

/**
 *	This helper class handles and manipulates the relationship between 
 *	different bones in a triangle list to ease splitting.
 */
class SkinSplitter
{
public:
	template
		<typename TriangleVector,
		typename BloatVertexVector,
		typename BoneVertexVector>
	SkinSplitter(
		const TriangleVector& triangles, 
		const BloatVertexVector& vertices, 
		const BoneVertexVector& boneVertices );

	bool createList( uint32 nodeLimit, BW::vector<uint32>& nodeList );

	template <typename BoneVertex>
	static bool checkIndex( 
		const BoneVertex& v,
		const BW::vector<bool>& indexUsed );

	template<typename TriangleVector,
		typename BloatVertexVector,
		typename BoneVertexVector>
	static void splitTriangles( 
		TriangleVector& triangles, 
		TriangleVector& splitTriangles,
		const BW::vector<uint32>& boneIndices,
		const BloatVertexVector& vertices,
		const BoneVertexVector& boneVertices, size_t numBones );

	/*
	 * The number of relationships in the list.
	 */
	uint size() { return static_cast<uint>(relationships_.size()); }

private:

	int	findAppropriateRelationship( uint32 nodeLimit, 
		const BW::vector<uint32>& nodeList) const;

	template <typename BoneVertex>
	void addRelationship( const BoneVertex& v1, const BoneVertex& v2, 
		const BoneVertex& v3 );

	int findRelationship( const BoneRelationship& relationship, 
		bool perfectMatch = true );

	template <typename BoneVertex>
	void addRelationship( BoneRelationship& relationship, 
		const BoneVertex& v );

	BW::vector<BoneRelationship>	relationships_;
};

BW_END_NAMESPACE

#include "skin_splitter.ipp"

#endif // SKIN_SPLITTER_HPP
