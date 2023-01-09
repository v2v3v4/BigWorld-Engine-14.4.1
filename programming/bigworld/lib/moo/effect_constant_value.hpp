#ifndef EFFECT_CONSTANT_VALUE_HPP
#define EFFECT_CONSTANT_VALUE_HPP

// DX Headers
#include <d3dx9effect.h>
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

typedef SmartPointer<class EffectConstantValue> EffectConstantValuePtr;

/**
 *	This class implements the constant value base class.
 *	
 */
class EffectConstantValue : public ReferenceCount
{
public:
	virtual ~EffectConstantValue() {}
	virtual bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle) = 0;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_CONSTANT_VALUE_HPP
