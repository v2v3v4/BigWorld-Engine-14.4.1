#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_MATTER_HPP
#define MODEL_MATTER_HPP

#include "forward_declarations.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/visual.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Inner class to represent the base Model's dye definition
 */
class Matter
{
public:
	typedef BW::vector<TintPtr>	Tints;
	typedef BW::vector< Moo::Visual::PrimitiveGroup * >	PrimitiveGroups;

public:
	Matter(	const BW::string & name,
			const BW::string & replaces );
	Matter( const BW::string & name,
			const BW::string & replaces,
			const Tints & tints );
	Matter( const Matter & other );
	~Matter();

	Matter & operator= ( const Matter & other );

	void emulsify( int tint = 0 );
	void soak();


	BW::string				name_;
	BW::string				replaces_;

	Tints					tints_;
	PrimitiveGroups			primitiveGroups_;

private:
	int		emulsion_;
	int		emulsionCookie_;
};

BW_END_NAMESPACE

#endif // MODEL_MATTER_HPP
