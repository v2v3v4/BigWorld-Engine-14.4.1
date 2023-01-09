#include "pch.hpp"

#include "model_actions_iterator.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

ModelActionsIterator::ModelActionsIterator( const Model * pModel ) :
	pModel_( pModel )
{
	BW_GUARD;
	while (pModel_ != NULL)
	{
		index_ = 0;
		if (pModel_->actions_.size() != 0) break;

		pModel_ = &*pModel_->parent_;
	}
}


const ModelAction & ModelActionsIterator::operator*()
{
	BW_GUARD;
	return *pModel_->actions_[ index_ ];
}


const ModelAction * ModelActionsIterator::operator->()
{
	BW_GUARD;
	return &*pModel_->actions_[ index_ ];
}


void ModelActionsIterator::operator++( int )
{
	BW_GUARD;
	index_++;
	while (index_ == pModel_->actions_.size())
	{
		pModel_ = pModel_->parent_.get();
		if (pModel_ == NULL)
		{
			break;
		}

		index_ = 0;
	}
}

bool ModelActionsIterator::operator==( const ModelActionsIterator & other ) const
{
	BW_GUARD;
	return pModel_ == other.pModel_ &&
		(pModel_ == NULL || index_ == other.index_);
}

bool ModelActionsIterator::operator!=( const ModelActionsIterator & other ) const
{
	BW_GUARD;
	return !this->operator==( other );
}

BW_END_NAMESPACE


// actions_iterator.cpp
