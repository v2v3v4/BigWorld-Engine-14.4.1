#ifndef _MORPHER_HOLDER_HPP_
#define _MORPHER_HOLDER_HPP_

#include "mfxexp.hpp"

BW_BEGIN_NAMESPACE

class MorpherHolder
{
public:
	MorpherHolder(INode* node)
	{
		// Disable the Morpher modifier if it is present
		mod_ = MFXExport::findMorphModifier( node );
		if ( mod_ )
			mod_->DisableMod();
	}

	~MorpherHolder()
	{
		if (mod_)
			mod_->EnableMod();
	}

private:
	Modifier* mod_;
};

BW_END_NAMESPACE

#endif  // morpher_holder.hpp