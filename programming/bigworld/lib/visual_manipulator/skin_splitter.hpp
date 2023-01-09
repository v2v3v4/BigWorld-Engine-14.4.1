#ifndef VISUAL_MANIPULATOR_SKIN_SPLITTER_HPP
#define VISUAL_MANIPULATOR_SKIN_SPLITTER_HPP

#include "mesh_piece.hpp"


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{
	typedef BW::vector<uint32> BoneRelationship;

	/**
	 *	This helper class handles and manipulates the relationship between 
	 *	different bones in a triangle list to ease splitting.
	 */
	class SkinSplitter
	{
	public:
		SkinSplitter(  const TriangleVector& triangles, const VertexVector& vertices );

		bool createList(uint32 nodeLimit, BW::vector<uint32>& nodeList);

		static bool checkIndex( const BloatVertex& v,
			const BW::vector<bool>& indexUsed );

		static void splitTriangles( TriangleVector& triangles, TriangleVector& splitTriangles,
			const BW::vector<uint32>& boneIndices, const VertexVector& vertices, size_t numBones );

		/*
		 * The number of relationships in the list.
		 */
		uint size() { return static_cast<uint>(relationships_.size()); }

	private:

		int	findAppropriateRelationship(uint32 nodeLimit, const BW::vector<uint32>& nodeList) const;

		void addRelationship( const BloatVertex& v1, const BloatVertex& v2, 
								   const BloatVertex& v3 );

		int findRelationship( const BoneRelationship& relationship, bool perfectMatch = true );

		void addRelationship( BoneRelationship& relationship, const BloatVertex& v );

		BW::vector<BoneRelationship>	relationships_;
	};

}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_SKIN_SPLITTER_HPP
