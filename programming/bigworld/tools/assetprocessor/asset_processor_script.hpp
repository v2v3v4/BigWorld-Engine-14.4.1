#ifndef ASSET_PROCESSOR_SCRIPT_HPP
#define ASSET_PROCESSOR_SCRIPT_HPP

#include "moo/visual.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class BSPProxy;
	typedef SmartPointer< BSPProxy > BSPProxyPtr;	
};

/*~ module AssetProcessor
 */
namespace AssetProcessorScript
{
	void init();
	void fini();

	int populateWorldTriangles( Moo::Visual::Geometry& geometry, 
							RealWTriangleSet & ws,
							const Matrix & m,
							BW::vector<BW::string>& materialIDs );

	BW::string generateBSP2( const BW::string& resourceID,
								Moo::BSPProxyPtr& ret,
								BW::vector<BW::string>& retMaterialIDs,
								uint32& nVisualTris,
								uint32& nDegenerateTris );
	BW::string replaceBSPData( const BW::string& visualName,
								Moo::BSPProxyPtr pGeneratedBSP,
								BW::vector<BW::string>& bspMaterialIDs );
};

BW_END_NAMESPACE

#endif
