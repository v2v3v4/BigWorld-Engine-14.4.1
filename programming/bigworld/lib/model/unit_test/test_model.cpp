#include "pch.hpp"

#include "cstdmf/bw_map.hpp"
#include "moo/animation.hpp"

#include "model/super_model.hpp"
#include "moo/texture_manager.hpp"
#include "moo/visual_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/resmgr_test_utils.hpp"
#include "pyscript/py_data_section.hpp"
#include "cstdmf/debug.hpp"
#include "duplo/pymodel.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

namespace TestModel
{
	bool reconvertModelAnimations( const BW::string & modelName )
	{
		NOTICE_MSG( "reconverting animations for %s", modelName.c_str() );

		const BW::string animationCacheName = modelName + ".anca";

		//Delete the animations that have been converted into a combined
		//cached file .
		if (!ResMgrTest::eraseResource( animationCacheName ))
		{
			ERROR_MSG( "Failed to remove resource %s.\n",
				animationCacheName.c_str() );
			return false;
		}
		MF_ASSERT( !BWResource::fileExists( animationCacheName ) );


		const BW::string fullModelname = modelName + ".model";

		ModelPtr modelptr = Model::get( fullModelname );
		if (modelptr == NULL)
		{
			ERROR_MSG( "Failed to load %s.\n", fullModelname.c_str() );
			return false;
		}

		//Check to see the anca file was created.
		if (BWResource::fileExists( animationCacheName ) == false)
		{
			ERROR_MSG( "Failed to create %s.\n", animationCacheName.c_str() );
			return false;
		}

		NOTICE_MSG( "Finished loading %s.\n", fullModelname.c_str() );

		return true;
	}
}


TEST( Moo_ModelAnimationReload )
{
	CHECK( TestModel::reconvertModelAnimations(
		"characters/npc/rat/models/rat" ) );
	CHECK( TestModel::reconvertModelAnimations(
		"characters/npc/crab/crab" ) );
	CHECK( TestModel::reconvertModelAnimations(
		"characters/npc/chicken/chicken" ) );
};

BW_END_NAMESPACE
