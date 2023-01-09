#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_TINT_HPP
#define MODEL_TINT_HPP

#include "moo/forward_declarations.hpp"

#include "dye_property.hpp"
#include "dye_selection.hpp"


BW_BEGIN_NAMESPACE

class Tint;
typedef SmartPointer< Tint > TintPtr;


/**
 *	Inner class to represent a tint
 *	@todo update
 */
class Tint : public ReferenceCount
{
public:
	Tint( const BW::string & name, bool defaultTint = false );
	virtual ~Tint();

	void applyTint();

	void updateFromEffect();

	BW::string name_;

	Moo::ComplexEffectMaterialPtr			effectMaterial_;	// newvisual
	BW::vector< DyeProperty >		properties_;		// *visual
	BW::vector< DyeSelection >		sourceDyes_;		// textural

private:
	Tint( const Tint & );
	Tint & operator=( const Tint & );

	bool default_;
};

BW_END_NAMESPACE


#endif // MODEL_TINT_HPP

