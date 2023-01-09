#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MOO_FORWARD_DECLARATIONS_HPP
#define MOO_FORWARD_DECLARATIONS_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

	class Animation;
	typedef SmartPointer<Animation>				AnimationPtr;
	typedef ConstSmartPointer<Animation>		ConstAnimationPtr;

	class EffectMaterial;
	typedef SmartPointer<EffectMaterial>		EffectMaterialPtr;
	typedef ConstSmartPointer<EffectMaterial>	ConstEffectMaterialPtr;

	class BaseTexture;
	typedef SmartPointer<BaseTexture>			BaseTexturePtr;
	typedef ConstSmartPointer<BaseTexture>		ConstBaseTexturePtr;

	class ComplexEffectMaterial;
	typedef SmartPointer<ComplexEffectMaterial>			ComplexEffectMaterialPtr;
	typedef ConstSmartPointer<ComplexEffectMaterial>	ConstComplexEffectMaterialPtr;

	class EffectProperty;
	typedef SmartPointer<EffectProperty>		EffectPropertyPtr;
	typedef ConstSmartPointer<EffectProperty>	ConstEffectPropertyPtr;

	class Material;
	typedef SmartPointer<Material>				MaterialPtr;
	typedef ConstSmartPointer<Material>			ConstMaterialPtr;

	class ManagedEffect;
	typedef SmartPointer<ManagedEffect>			ManagedEffectPtr;
	typedef ConstSmartPointer<ManagedEffect>	ConstManagedEffectPtr;

	class Node;
	typedef SmartPointer<Node>					NodePtr;
	typedef ConstSmartPointer<Node>				ConstNodePtr;

	class StreamedDataCache;
	class VertexDeclaration;

	class Visual;
	typedef SmartPointer<Visual>				VisualPtr;
	typedef ConstSmartPointer<Visual>			ConstVisualPtr;

	class Vertices;
	typedef SmartPointer<Vertices>				VerticesPtr;

	class Primitive;
	typedef SmartPointer<Primitive>				PrimitivePtr;

	class RenderTarget;
	typedef SmartPointer<RenderTarget>					RenderTargetPtr;
} // namespace Moo

BW_END_NAMESPACE


#endif // MOO_FORWARD_DECLARATIONS_HPP
