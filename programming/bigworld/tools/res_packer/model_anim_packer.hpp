#ifndef __ANIM_PACKER_HPP__
#define __ANIM_PACKER_HPP__


#include "base_packer.hpp"
#include "packers.hpp"

#include <string>


BW_BEGIN_NAMESPACE

/**
 *	This class converts all .animaion files related to a model to .anca, by
 *	loading the model.
 */
class ModelAnimPacker : public BasePacker
{
private:
	enum Type {
		MODEL,
		ANIMATION,
		ANCA
	};

public:
	ModelAnimPacker() : type_( MODEL ) {}

	virtual bool prepare( const BW::string & src, const BW::string & dst );
	virtual bool print();
	virtual bool pack();

private:
	DECLARE_PACKER()
	BW::string src_;
	BW::string dst_;
	Type type_;
};

BW_END_NAMESPACE

#endif // __ANIM_PACKER_HPP__
