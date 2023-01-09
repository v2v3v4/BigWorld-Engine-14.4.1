#include "pch.hpp"

#include "matter.hpp"

#include "model.hpp"
#include "tint.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
Matter::Matter(	const BW::string & name,
				const BW::string & replaces ) :
	name_( name ),
	replaces_( replaces ),
	emulsion_( 0 ),
	emulsionCookie_( Model::getUnusedBlendCookie() )
{
	BW_GUARD;
	tints_.push_back( new Tint( "Default", true ) );
}


/**
 *	Constructor used when inheriting matters from a parent model.
 */
Matter::Matter(	const BW::string & name,
				const BW::string & replaces,
				const Tints & tints ) :
	name_( name ),
	replaces_( replaces ),
	emulsion_( 0 ),
	emulsionCookie_( Model::getUnusedBlendCookie() )
{
	BW_GUARD;
	tints_.reserve( tints.size() );
	for (uint i=0; i < tints.size(); i++)
	{
		tints_.push_back( new Tint( tints[i]->name_, false ) );
	}
}


/**
 *	Copy constructor
 */
Matter::Matter( const Matter & other ) :
	tints_( other.tints_ ),
	replaces_( other.replaces_ ),
	primitiveGroups_( other.primitiveGroups_ ),
	emulsion_( 0 ),
	emulsionCookie_( Model::getUnusedBlendCookie() )
{
	BW_GUARD;	
}


/**
 *	Destructor
 */
Matter::~Matter()
{
	BW_GUARD;	
}


/**
 *	Assignment operator
 */
Matter & Matter::operator=( const Matter & other )
{
	BW_GUARD;
	tints_ = other.tints_;
	replaces_ = other.replaces_;
	primitiveGroups_ = other.primitiveGroups_;
	emulsion_ = 0;
	emulsionCookie_ = Model::getUnusedBlendCookie();

	return *this;
}


/**
 *	This method selects the given tint as the one that the model should be
 *	soaked in. It is suspended in an emulsion.
 */
void Matter::emulsify( int tint )
{
	BW_GUARD;
	if (uint(tint) >= tints_.size())
		tint = 0;	// use default if not found
	emulsion_ = tint;
	emulsionCookie_ = Model::blendCookie();
}


/**
 *	This method soaks the model that this matter lives in with the stored tint,
 *	or the default tint, if there is no emulsion.
 */
void Matter::soak()
{
	BW_GUARD;
	int tint = (emulsionCookie_ == Model::blendCookie()) ? emulsion_ : 0;

	TintPtr rTint = tints_[ tint ];

	rTint->applyTint();
	for (uint i = 0; i < primitiveGroups_.size(); i++)
	{
		primitiveGroups_[i]->material_ = rTint->effectMaterial_;
	}
}

BW_END_NAMESPACE

// model_matter.cpp
