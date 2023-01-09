#ifndef __IMAGE_PACKER_HPP__
#define __IMAGE_PACKER_HPP__


#include "base_packer.hpp"
#include "packers.hpp"

#include <string>


BW_BEGIN_NAMESPACE

/**
 *	This class converts all .bmp, .tga and .jpg images to .dds, using a
 *	.texformat file if it exists. It also copies .dds files that don't have
 *	a corresponding bmp, jpg or tga file.
 */
class ImagePacker : public BasePacker
{
private:
	enum Type {
		IMAGE,
		DDS
	};

public:
	ImagePacker() : type_( IMAGE ) {}

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

#endif // __IMAGE_PACKER_HPP__
