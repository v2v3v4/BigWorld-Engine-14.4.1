#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_FORWARD_DECLARATIONS_HPP
#define MODEL_FORWARD_DECLARATIONS_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class TransformFashion;
typedef SmartPointer< TransformFashion >			TransformFashionPtr;
typedef ConstSmartPointer< TransformFashion >		ConstTransformFashionPtr;

class MaterialFashion;
typedef SmartPointer< MaterialFashion >				MaterialFashionPtr;
typedef ConstSmartPointer< MaterialFashion >		ConstMaterialFashionPtr;

class Matter;

class Model;
typedef SmartPointer< Model >						ModelPtr;
typedef ConstSmartPointer< Model >					ConstModelPtr;

class ModelAction;
typedef SmartPointer< ModelAction >					ModelActionPtr;
typedef ConstSmartPointer< ModelAction >			ConstModelActionPtr;

class ModelActionsIterator;

class ModelAnimation;
typedef SmartPointer< ModelAnimation >				ModelAnimationPtr;
typedef ConstSmartPointer< ModelAnimation >			ConstModelAnimationPtr;

class ModelMap;

class SuperModel;

class SuperModelAction;
typedef SmartPointer< SuperModelAction >			SuperModelActionPtr;
typedef ConstSmartPointer< SuperModelAction >		ConstSuperModelActionPtr;

class SuperModelAnimation;
typedef SmartPointer< SuperModelAnimation >			SuperModelAnimationPtr;
typedef ConstSmartPointer< SuperModelAnimation >	ConstSuperModelAnimationPtr;

class SuperModelDye;
typedef SmartPointer< SuperModelDye >				SuperModelDyePtr;
typedef ConstSmartPointer< SuperModelDye >			ConstSuperModelDyePtr;

class Tint;
typedef SmartPointer< Tint >						TintPtr;
typedef ConstSmartPointer< Tint >					ConstTintPtr;

BW_END_NAMESPACE


#endif // MODEL_FORWARD_DECLARATIONS_HPP

