#ifndef __FX_PACKER_HPP__
#define __FX_PACKER_HPP__

#include "base_packer.hpp"
#include "packers.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class simply copies all .fxo files that already exist, it does not
 *	rebuild the shaders here (a separate offline shader compilation tool
 *	exists for this). Sources are only copied if res_packer has been
 *	configured to do so (see config.hpp).
 */
class FxPacker : public BasePacker
{
public:
	enum Type {
		FX,
		FXH,
		FXO
	};

	FxPacker() : type_( FX ) {}

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

#endif // __FX_PACKER_HPP__
