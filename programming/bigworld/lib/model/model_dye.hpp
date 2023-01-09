#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_DYE_HPP
#define MODEL_DYE_HPP


BW_BEGIN_NAMESPACE

class Model;


/**
 *	This class stores the indexes necessary to apply a dye to a specific
 *	model.
 */
class ModelDye
{
public:
	ModelDye( const Model & model, int matterIndex, int tintIndex );

	ModelDye( const ModelDye & modelDye );
	ModelDye & operator=( const ModelDye & modelDye );

	bool isNull() const;

	int16 matterIndex() const;
	int16 tintIndex() const;

private:
	int16	matterIndex_;
	int16	tintIndex_;
};

BW_END_NAMESPACE


#endif // MODEL_DYE_HPP
