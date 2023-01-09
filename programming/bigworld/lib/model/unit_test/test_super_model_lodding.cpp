#include "pch.hpp"

#include "math/matrix.hpp"
#include "model/super_model.hpp"
#include "moo/render_context.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "moo/lod_settings.hpp"

BW_BEGIN_NAMESPACE

AutoConfigString s_graphicsSettingsXML( "system/graphicsSettingsXML" );

namespace TestModel
{
	bool initLodSettings( BWResource& bwresource, LodSettings& lodSettings )
	{
		bool result = true;

		// Setup object LOD options
		DataSectionPtr pSection =
			bwresource.openSection( s_graphicsSettingsXML );
		result &= ( pSection.exists() );
		DataSectionPtr lodOptionsSec = pSection->openSection( "objectLOD" );
		result &= ( lodOptionsSec.exists() );

		lodSettings.init( lodOptionsSec );
		return result;
	}


	bool setupTests( BWResource& bwresource, LodSettings& lodSettings )
	{
		bool result = true;

		// Run things that configure themselves from a config file
		result &= ( AutoConfig::configureAllFrom( AutoConfig::s_resourcesXML ) );
		result &= initLodSettings( bwresource, lodSettings );

		// Set eye position to (0,0,-3)
		// View matrix stores -eye.xyz
		Matrix cameraMatrix( Matrix::identity );
		cameraMatrix._41 = 0.0f;
		cameraMatrix._42 = 0.0f;
		cameraMatrix._43 = 3.0f;

		Moo::rc().reset();
		Moo::rc().view( cameraMatrix );
		Moo::rc().updateProjectionMatrix();
		Moo::rc().updateViewTransforms();
		Moo::rc().updateLodViewMatrix();

		return result;
	}


	/**
	 *	Test calculate LOD between (0,0,0) and (0,0,0).
	 *	Eye is at (0,0,-3), look at vector (0,0,1).
	 */
	/**
	 *	Test calculate LOD between:
	 * |y
	 * | /z
	 * |/
	 * +----------x
	 * x Object(x=0, z=0)
	 *
	 * x         
	 * Camera(z=-3)
	 */
	bool testLodSettingsCalculateLod1( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		const Matrix world( Matrix::identity );

		const float result = lodSettings.calculateLod( world );
		// (pos(0,0,0) dot lookat(0,0,1)) = 0
		// 0 + neg_eyez(3) = 3
		const float expectedResult = 3.0f;

		return almostEqual( result, expectedResult );
	}


	/**
	 *	Test calculate LOD between:
	 * |y
	 * | /z
	 * |/
	 * +----------x
	 *       
	 * x         x Object(x=100, z=-3)
	 * Camera(z=-3)
	 */
	bool testLodSettingsCalculateLod2( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		Matrix world( Matrix::identity );
		world.setTranslate( 100.0f, 0.0f, -3.0f );

		const float result = lodSettings.calculateLod( world );
		// (pos(100,0,-3) dot lookat(0,0,1)) = -3
		// -3 + neg_eyez(3) = 0
		const float expectedResult = 0.0f;

		return almostEqual( result, expectedResult );
	}

	/**
	 *	Test calculate LOD between:
	 * |y
	 * | /z x Object(x=100, z=100)
	 * |/
	 * +----------x
	 *       
	 * x
	 * Camera(z=-3)
	 */
	bool testLodSettingsCalculateLod3( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		Matrix world( Matrix::identity );
		world.setTranslate( 0.0f, 0.0f, 100.0f );

		const float result = lodSettings.calculateLod( world );
		// (pos(0,0,100) dot lookat(0,0,1)) = 100 
		// 100 + neg_eyez(3) = 
		const float expectedResult = 103.0f;

		return almostEqual( result, expectedResult );
	}

	/**
	 *	Test calculate LOD between:
	 * |y
	 * | /z
	 * |/
	 * +----------x
	 *       
	 * x
	 * Camera(z=-3)
	 *
	 *
	 *
	 * x Object(x=0, z=-100)
	 */
	bool testLodSettingsCalculateLod4( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		Matrix world( Matrix::identity );
		world.setTranslate( 0.0f, 0.0f, -100.0f );

		const float result = lodSettings.calculateLod( world );
		// (pos(0,0,-100) dot lookat(0,0,1)) = -100
		// -100 + neg_eyez(3) = 
		const float expectedResult = 0.0f;

		return almostEqual( result, expectedResult );
	}


	/**
	 *	Test a lod on a super model.
	 *	@param superModel the model to test.
	 *	@param world world transform of model.
	 *	@param lod the lod to test.
	 *	@param expectedResult visible/not visible.
	 *	@param expectedNextLodUp distance when lod will change,
	 *		getting closer.
	 *	@param expectedNextLodDown distance when lod will change,
	 *		getting further.
	 *	@return true if test passes.
	 */
	bool checkLod( SuperModel& superModel,
		const Matrix& world,
		float lod,
		bool expectedResult,
		float expectedNextLodUp,
		float expectedNextLodDown )
	{
		bool finalResult = true;

		superModel.updateLodsOnly( world, lod );

		const bool result = superModel.isLodVisible();

		finalResult &= ( result == expectedResult );
		finalResult &= almostEqual( superModel.getLod(), lod );
		finalResult &= almostEqual(
			superModel.getLodNextUp(), expectedNextLodUp );
		finalResult &= almostEqual(
			superModel.getLodNextDown(), expectedNextLodDown );

		return finalResult;
	}


	/**
	 *	Test a lod on a super model.
	 *	@param lodSettings the lod calculator.
	 *	@param superModel the model to test.
	 *	@param world world transform of model.
	 *	@param expectedLod the lod that should be calculated by lod settings.
	 *	@param expectedNextLodUp distance when lod will change,
	 *		getting closer.
	 *	@return true if test passes.
	 */
	bool checkLodBehindCamera( LodSettings& lodSettings,
		SuperModel& superModel,
		const Matrix& world,
		float expectedLod )
	{
		bool finalResult = true;

		// Check calculate lod works
		{
			const float result = lodSettings.calculateLod( world );
			finalResult &= almostEqual( result, expectedLod );
		}

		// Test on super model, give lod a negative number
		superModel.updateLodsOnly( world, -1.0f );

		const bool result = superModel.isLodVisible();

		// Never visible behind camera, unless the camera is within the model
		const bool expectedResult = true;

		finalResult &= (result == expectedResult);
		if (expectedResult)
		{
			finalResult &= almostEqual(
				superModel.getLodNextUp(), Model::LOD_HIDDEN );

			float expectedNextLodDown = Model::LOD_MAX;
			for (int i = 0; i < superModel.nModels(); ++i)
			{
				expectedNextLodDown = std::min( expectedNextLodDown,
					superModel.topModel( i )->extent() );
			}

			finalResult &= almostEqual(
				superModel.getLodNextDown(),
				expectedNextLodDown );
		}
		else
		{
			finalResult &= almostEqual(
				superModel.getLodNextUp(), Model::LOD_HIDDEN );
			finalResult &= almostEqual(
				superModel.getLodNextDown(), Model::LOD_MIN );
		}

		return finalResult;
	}


	/**
	 *	Test a lod on a super model.
	 *	@param lodSettings the lod calculator.
	 *	@param superModel the model to test.
	 *	@param world world transform of model.
	 *	@param expectedLod the lod that should be calculated by lod settings.
	 *	@param expectedNextLodUp distance when lod will change,
	 *		getting closer.
	 *	@return true if test passes.
	 */
	bool checkShellLodBehindCamera( LodSettings& lodSettings,
		SuperModel& superModel,
		const Matrix& world,
		bool expectedResult,
		float expectedLod,
		float expectedNextLodUp,
		float expectedNextLodDown )
	{
		bool finalResult = true;

		// Check calculate lod works
		{
			const float result = lodSettings.calculateLod( world );
			finalResult &= almostEqual( result, expectedLod );
		}

		// Test on super model, give lod a negative number
		superModel.updateLodsOnly( world, -1.0f );

		const bool result = superModel.isLodVisible();

		// Maybe visible behind camera, if camera is within the shell
		finalResult &= (result == expectedResult);
		finalResult &= almostEqual(
			superModel.getLodNextUp(), expectedNextLodUp );
		finalResult &= almostEqual(
			superModel.getLodNextDown(), expectedNextLodDown );

		return finalResult;
	}


	/**
	 *	Tests for lodding when object is behind the camera.
	 *	We have to use the world and LodSettings for this because giving
	 *	SuperModel::updateLodsOnly a negative lod will cause it to
	 *	re-calculate.
	 */
	bool testBehindCamera( SuperModel& superModel,
		LodSettings& lodSettings )
	{
		bool finalResult = true;

		// Just behind camera, -0.1f
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.1f );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		// Further behind camera, -20.0f
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -23.0f );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		// Right angles to camera, 0.1f to the right
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.1f, 0.0f, -3.0f );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		// Right angles to camera, 20.0f to the right
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 20.0f, 0.0f, -3.0f );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		// Special numbers, LOD_HIDDEN
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.0f + Model::LOD_HIDDEN );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		// Special numbers, LOD_MISSING
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.0f + Model::LOD_MISSING );
			finalResult &= checkLodBehindCamera( lodSettings,
				superModel,
				world,
				0.0f ); // expected lod
		}

		return finalResult;
	}


	/**
	 *	This test is for a model with one LOD.
	 *	Extents from 0-80.
	 */
	bool testUpdateLods1( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "sets/dungeon/props/honourstone.model" );
		SuperModel superModel( modelIDs );
		Matrix world( Matrix::identity );

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			80.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			80.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			79.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			80.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			80.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			80.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			80.1f, // test lod
			false, // expected visibility
			80.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		// Now lod it back in after it was lodded out to check if it's stuck
		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			80.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			80.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= testBehindCamera( superModel, lodSettings );

		return finalResult;
	}


	/**
	 *	This test is for a model with two LODs.
	 *	Extents from 0-5, 5-30.
	 */
	bool testUpdateLods2( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "sets/town/props/axe.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			5.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			5.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			5.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			5.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			29.0f, // test lod
			true, // expected visibility
			5.0f, // expected next lod up
			30.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			30.0f, // test lod
			true, // expected visibility
			5.0f, // expected next lod up
			30.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			30.1f, // test lod
			false, // expected visibility
			30.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		// Now lod it back in to the first lod after it was lodded out
		// to check if it's stuck
		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			5.0f ); // expected next lod down


		// Now lod back out
		finalResult &= checkLod( superModel,
			world,
			40.0f, // test lod
			false, // expected visibility
			30.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		// Now lod it back in to the second lod after it was lodded out
		// to check if it's stuck
		finalResult &= checkLod( superModel,
			world,
			25.0f, // test lod
			true, // expected visibility
			5.0f, // expected next lod up
			30.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			30.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= testBehindCamera( superModel, lodSettings );

		return finalResult;
	}


	/**
	 *	This test is for a model which does not LOD.
	 *	Extents from 0-infinity.
	 */
	bool testUpdateLods3( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "helpers/models/unit_cube.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			79.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= testBehindCamera( superModel, lodSettings );

		return finalResult;
	}


	/**
	 *	This test is for a shell model, one that you can be inside.
	 *	Extents from 0-infinity.
	 */
	bool testUpdateLods4( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "sets/minspec/shells/dungeon/dungeon_tunnel/model/dungeon_tunnel_basic.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			79.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		// Test behind camera, camera can be inside shell
		// Just behind camera, within shell, -0.1f
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.1f );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Behind camera, outside shell, -10.0f, but within "model radius" (~15)
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -13.0f );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Behind camera, outside shell, -17.0f
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -20.0f );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Right angles to camera, 0.1f to the right, within shell
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.1f, 0.0f, -3.0f );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Right angles to camera, 20.0f to the right, outside shell
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 20.0f, 0.0f, -3.0f );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Special numbers, LOD_HIDDEN, within shell
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.0f + Model::LOD_HIDDEN );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		// Special numbers, LOD_MISSING, within shell
		{
			Matrix world( Matrix::identity );
			world.setTranslate( 0.0f, 0.0f, -3.0f + Model::LOD_MISSING );
			finalResult &= checkShellLodBehindCamera( lodSettings,
				superModel,
				world,
				true, // expected visibility
				0.0f, // expected lod
				Model::LOD_HIDDEN, // expected next lod up
				Model::LOD_MAX ); // expected next lod down
		}

		return finalResult;
	}


	/**
	 *	This test is for a model which has multiple models.
	 *	SuperModel contains ranger_body and ranger_head.
	 *	ranger_body has base as a lower lod.
	 *	ranger_body extent 0-73, then base 73-400
	 *	ranger_head extent 0-73
	 */
	bool testUpdateLods5( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "characters/avatars/ranger/ranger_body.model" );
		modelIDs.push_back( "characters/avatars/ranger/ranger_head.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			73.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			73.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			73.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			73.0f ); // expected next lod down
			
		finalResult &= checkLod( superModel,
			world,
			73.1f, // test lod
			true, // expected visibility
			73.0f, // expected next lod up
			400.0f ); // expected next lod down
			
		finalResult &= checkLod( superModel,
			world,
			400.0f, // test lod
			true, // expected visibility
			73.0f, // expected next lod up
			400.0f ); // expected next lod down
			
		finalResult &= checkLod( superModel,
			world,
			400.1f, // test lod
			false, // expected visibility
			400.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down
			
		// Lod back in
		finalResult &= checkLod( superModel,
			world,
			20.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			73.0f ); // expected next lod down
			
		// Lod back out
		finalResult &= checkLod( superModel,
			world,
			500.0f, // test lod
			false, // expected visibility
			400.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down
			
		// Lod back in
		finalResult &= checkLod( superModel,
			world,
			100.0f, // test lod
			true, // expected visibility
			73.0f, // expected next lod up
			400.0f ); // expected next lod down

		// Max lod
		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			400.0f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= testBehindCamera( superModel, lodSettings );

		return finalResult;
	}

	
	/**
	 *	This test is for a model which does not LOD.
	 *	Extents from 0-infinity.
	 */
	bool testUpdateLods6( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "sets/minspec/props/tree/model/tree.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		// Test taking the model down the lod chain, then back up again.
		// This was broken recently because the lod boundaries were
		// calculated wrong when going down the chain.

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			19.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			19.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			79.0f, // test lod
			true, // expected visibility
			19.f, // expected next lod up
			117.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			120.f, // test lod
			true, // expected visibility
			117.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			79.0f, // test lod
			true, // expected visibility
			19.f, // expected next lod up
			117.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			19.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			19.0f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			true, // expected visibility
			117.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		return finalResult;
	}


	/**
	*	This test is for a model with 2 hidden LODs.
	*	Extents: 0-10, (hidden LOD), 10-20, (hidden LOD).
	 */
	bool testUpdateLods7( BWResource& bwresource,
		LodSettings& lodSettings )
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( "sets/minspec/props/barrel/models/barrel.model" );
		SuperModel superModel( modelIDs );

		Matrix world( Matrix::identity );

		// Test taking the model down the lod chain, then back up again.
		// This was broken when zooming out of LOD then back with hidden 
		// LOD model makes visible LOD model invisible (BWT-24629).

		bool finalResult = true;

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MIN, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			5.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			10.0f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			10.01f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			20.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			20.01f, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			100.f, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			20.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			15.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			10.f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			100.f, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			15.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			1.f, // test lod
			true, // expected visibility
			Model::LOD_HIDDEN, // expected next lod up
			10.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			Model::LOD_MAX, // test lod
			false, // expected visibility
			20.f, // expected next lod up
			Model::LOD_MAX ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			20.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		finalResult &= checkLod( superModel,
			world,
			20.f, // test lod
			true, // expected visibility
			10.f, // expected next lod up
			20.f ); // expected next lod down

		return finalResult;
	}
}


TEST( Moo_ModelLodding )
{
	BWResource& bwresource = BWResource::instance();
	LodSettings& lodSettings = LodSettings::instance();
	CHECK( TestModel::setupTests( bwresource, lodSettings ) );
	CHECK( TestModel::testLodSettingsCalculateLod1( bwresource, lodSettings ) );
	CHECK( TestModel::testLodSettingsCalculateLod2( bwresource, lodSettings ) );
	CHECK( TestModel::testLodSettingsCalculateLod3( bwresource, lodSettings ) );
	CHECK( TestModel::testLodSettingsCalculateLod4( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods1( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods2( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods3( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods4( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods5( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods6( bwresource, lodSettings ) );
	CHECK( TestModel::testUpdateLods7( bwresource, lodSettings ) );
};

BW_END_NAMESPACE
