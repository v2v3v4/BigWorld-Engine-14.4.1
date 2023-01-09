#ifndef SPEEDTREE_UTIL_HPP
#define SPEEDTREE_UTIL_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "math/vector3.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

namespace speedtree
{
	/**
	 *	Creates a string that uniquely identifies a tree loaded from 
	 *	the given filename and generated with the provided seed number.
	 *
	 *	@param	filename	full path to the SPT file used to load the tree.
	 *	@param	seed		seed number used to generate the tree.
	 *
	 *	@return				the requested string.
	 */
	void createTreeDefName( const BW::StringRef & filename, uint seed, char * dst,
		size_t dstSize );

	float LineInBox(const Vector3& vMin, const Vector3& vMax, const Vector3& vStart, const Vector3& vEnd);
	float LineInEllipsoid(const Vector3& vSize, const Vector3& vStart, const Vector3& vEnd);

	//-- Decomposes matrix into rotation matrix 3x3, translation and scale.
	void  decomposeMatrix(const Matrix& iMat, Quaternion& oRotMat, Vector3& translation, Vector3& scale);

} //namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_UTIL_HPP
