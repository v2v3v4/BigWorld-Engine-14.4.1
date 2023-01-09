
#include "pch.hpp"

#include "resmgr_test_utils.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"


BW_BEGIN_NAMESPACE

namespace ResMgrTest
{
	/**
	 *	Erases the resource with the specified name from the resource path.
	 *	@return True if the resource with the specified name doesn't exist
	 *		(either didn't exist before or was successfully deleted).
	 */
	bool eraseResource( const BW::string & resourceName )
	{
		BW_GUARD;

		//Check whether the file exists first.
		if (BWResource::instance().fileSystem()->getFileType( resourceName ) ==
			IFileSystem::FT_NOT_FOUND)
		{
			return true;
		}

		return BWResource::instance().fileSystem()->eraseFileOrDirectory(
			resourceName );
	}
}

BW_END_NAMESPACE
