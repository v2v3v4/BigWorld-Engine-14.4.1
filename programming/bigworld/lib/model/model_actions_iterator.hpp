#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_ACTIONS_ITERATOR_HPP
#define MODEL_ACTIONS_ITERATOR_HPP

#include "forward_declarations.hpp"
#include "model.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class ModelActionsIterator
{
public:
	ModelActionsIterator( const Model * pModel );
	const ModelAction & operator*();
	const ModelAction * operator->();
	void operator++( int );
	bool operator==( const ModelActionsIterator & other ) const;
	bool operator!=( const ModelActionsIterator & other ) const;
private:
	const Model * pModel_;
	int index_;
};

BW_END_NAMESPACE

#endif // MODEL_ACTIONS_ITERATOR_HPP
