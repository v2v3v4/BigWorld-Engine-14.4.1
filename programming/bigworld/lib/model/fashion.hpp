#ifdef _MSC_VER 
#pragma once
#endif

#ifndef FASHION_HPP
#define FASHION_HPP

#include "forward_declarations.hpp"
#include "cstdmf/static_array.hpp"

BW_BEGIN_NAMESPACE


typedef BW::vector<TransformFashionPtr> TransformFashionVector;
typedef BW::vector<MaterialFashionPtr> MaterialFashionVector;

// Amount of space to reserve off the stack for lists of temporary fashion
// vectors.
const size_t MAX_TRANSFORMS = 128;
typedef DynamicEmbeddedArrayWithWarning<TransformFashion *, MAX_TRANSFORMS>
	TmpTransforms;

class Matrix;
class SuperModel;

/**
 *	This abstract class represents a fashion in which a supermodel
 *	may be drawn. It could be an animation at a certain time
 *	and blend ratio, or it could be a material override
 */
class Fashion : public ReferenceCount
{
public:
	Fashion();
	virtual ~Fashion();
};


/**
 *	This class represents any animations or other transforms that apply
 *	to a SuperModel.
 */
class TransformFashion : public Fashion
{
public:

	/**
	 *	Apply transform to the given SuperModel.
	 *	@param superModel the SuperModel on which to apply.
	 *	@param world the current world transform.
	 *		Note that the TransformFashion can modify the world transform.
	 */
	virtual void dress( SuperModel & superModel, Matrix& world ) = 0;

	/**
	 *	Check if this transform fashion requires the models to re-calculate
	 *	their transform state after this fashion is applied.
	 *	@return true if this transform requires a re-calculation of transform
	 *		state, false if it does not.
	 */
	virtual bool needsRedress() const = 0;
};


/**
 *	This class represents any materials that apply to a SuperModel before
 *	drawing.
 */
class MaterialFashion : public Fashion
{
public:

	/**
	 *	Apply material to the given SuperModel.
	 *	@param superModel the SuperModel on which to apply.
	 */
	virtual void dress( SuperModel & superModel ) = 0;

	/**
	 *	Removes any materials that have been applied.
	 *	@param superModel the SuperModel on which to apply.
	 */
	virtual void undress( SuperModel & superModel ) {};
};

BW_END_NAMESPACE



#endif // FASHION_HPP
