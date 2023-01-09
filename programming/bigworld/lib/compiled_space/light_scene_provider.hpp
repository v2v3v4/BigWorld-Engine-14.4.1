#ifndef _LIGHT_SCENE_PROVIDER_HPP
#define _LIGHT_SCENE_PROVIDER_HPP

#include <cstdmf/stdmf.hpp>
#include <cstdmf/lookup_table.hpp>
#include <resmgr/datasection.hpp>

#include <math/matrix.hpp>
#include <math/boundbox.hpp>
#include <math/loose_octree.hpp>

#include <scene/scene_provider.hpp>
#include <scene/light_scene_view.hpp>
#include <scene/tick_scene_view.hpp>

#include <compiled_space/binary_format.hpp>
#include <compiled_space/light_scene_types.hpp>
#include <compiled_space/loader.hpp>

#include <moo/omni_light.hpp>
#include <moo/spot_light.hpp>
#include <moo/pulse_light.hpp>

namespace BW {
namespace CompiledSpace {

	class StringTable;

	class LightSceneProvider :
		public ILoader,
		public SceneProvider,
		public ILightSceneViewProvider,
		public ITickSceneProvider
	{
	public:
		typedef BW::vector<Moo::OmniLightPtr> MooOmniLightPtrArray;
		typedef BW::vector<Moo::SpotLightPtr> MooSpotLightPtrArray;
		typedef BW::vector<Moo::PulseLightPtr> MooPulseLightPtrArray;
		
		static void registerHandlers( Scene & scene );

	public:
		LightSceneProvider();
		virtual ~LightSceneProvider();

		// ILoader interface
		bool doLoadFromSpace( ClientSpace * pSpace,
			BinaryFormat& reader,
			const DataSectionPtr& pSpaceSettings,
			const Matrix& transform,
			const StringTable& strings );

		bool doBind();
		void doUnload();
		float percentLoaded() const;

		bool isValid() const;

		void forEachObject( const ConstSceneObjectCallback & function ) const;
		void forEachObject( const SceneObjectCallback & function );

		//ILightSceneViewProvider interfaces...
		size_t intersect( const ConvexHull & hull,
			Moo::LightContainer & lightContainer ) const;
		size_t intersect( const AABB & bbox,
			Moo::LightContainer & lightContainer ) const;
		void debugDrawLights() const;

		//ITickSceneProvider interfaces...
		void tick( float dTime );
		void updateAnimations( float dTime );

	private:
		// Scene view implementations
		virtual void * getView(
			const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

		BinaryFormat* pReader_;
		BinaryFormat::Stream* pStream_;

		MooOmniLightPtrArray omniLights_;
		MooSpotLightPtrArray spotLights_;
		MooPulseLightPtrArray pulseLights_;
	};

} // namespace CompiledSpace
} // namespace BW



#endif // _LIGHT_SCENE_PROVIDER_HPP
