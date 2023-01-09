#pragma once


#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/binary_block.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class RenderTarget;
}


class MaterialPreview : public Singleton< MaterialPreview >
{
public:
	MaterialPreview();
	
	BinaryPtr generatePreview(
		const BW::string & materialName,
		const BW::string & matterName,
		const BW::string & tintName );

private:
	SmartPointer<Moo::RenderTarget> rt_;
};

BW_END_NAMESPACE

