#ifndef VERY_LARGE_OBJECT_BEHAVIOUR_HPP
#define VERY_LARGE_OBJECT_BEHAVIOUR_HPP

BW_BEGIN_NAMESPACE

class VeryLargeObject;

class VeryLargeObjectBehaviour
{
public:
	static void updateLocalVars(
		VeryLargeObject & object, const Matrix & m, Chunk * pChunk );

	static BoundingBox calcVLOBoundingBox(
		const Matrix & worldTransform, VeryLargeObject & object,
		const BoundingBox & vloBB );

	static bool edTransform(
		VeryLargeObject & object, Chunk * pChunk, BoundingBox & vloBB,
		ChunkItemPtr chunkItem,
		Matrix & o_Transform, bool & o_DrawTransient,
		const Matrix & m, bool transient );
};

BW_END_NAMESPACE

#endif // VERY_LARGE_OBJECT_BEHAVIOUR_HPP
