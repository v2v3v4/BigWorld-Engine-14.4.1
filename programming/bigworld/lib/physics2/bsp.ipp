/**
 *	@file
 */

#ifdef CODE_INLINE
#define INLINE inline
#else
/// INLINE macro.
#define INLINE
#endif

INLINE
const BoundingBox& BSPTree::boundingBox() const
{ 
	return boundingBox_;
}

INLINE
WorldTriangle*  BSPTree::triangles() const
{ 
	return pTriangles_;
}

INLINE
uint			BSPTree::numTriangles() const
{ 
	return numTriangles_;
}

INLINE
bool BSPTree::empty() const
{ 
	return numTriangles_ == 0;
}

INLINE
uint			BSPTree::numNodes() const
{ 
	return numNodes_;
}

// bsp.ipp
