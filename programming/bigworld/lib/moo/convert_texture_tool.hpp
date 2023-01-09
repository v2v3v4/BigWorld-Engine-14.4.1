#ifndef PROCESSOR_CONVERTTEXTURETOOL_HPP
#define PROCESSOR_CONVERTTEXTURETOOL_HPP

#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/stringmap.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/texture_detail_level_manager.hpp"

BW_BEGIN_NAMESPACE

class Renderer;

namespace Moo
{
	class ConvertTextureTool
	{
	public:
		ConvertTextureTool() 
			: hWnd_( NULL )
			, pRenderer_(NULL)
		{
		};

		bool init();
		void fini();

		// convert the texture, no questions asked.
		bool convert(const BW::string& srcName,
			const BW::string& destName, 
			TextureDetailLevelPtr& texLod,
			const bool bCompressionOption );
		
		bool initResources();
		void resourcesFini();
		bool initMoo();
		void MooFini();

private:
		HWND hWnd_;
		Renderer* pRenderer_;
	};

	

} // namespace Moo

BW_END_NAMESPACE

#endif // PROCESSOR_CONVERTTEXTURETOOL_HPP
