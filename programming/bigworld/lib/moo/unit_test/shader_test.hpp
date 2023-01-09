#ifndef MOO_UNIT_TEST_SHADER_TEST_HPP
#define MOO_UNIT_TEST_SHADER_TEST_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_USE_NAMESPACE

namespace BW
{
	namespace Moo
	{
		class EffectMaterial;
		typedef SmartPointer< EffectMaterial > EffectMaterialPtr;
	}
}

class ShaderTest
{
public:
	ShaderTest();

	Moo::EffectMaterialPtr init( const BW::string& effectFileName );    
	void fini();

	bool test( uint32 technique, const BW::string& refImageFile );

	static void getImageFileNameForEffect( const BW::string&	effectFileName,
										   uint32				techniqueIndex,
										   BW::string&			refImageFileName );
private:
	Moo::EffectMaterialPtr pEffect_;
};

#endif
