#ifndef TICK_SCENE_VIEW_HPP
#define TICK_SCENE_VIEW_HPP

#include "forward_declarations.hpp"
#include "scene_provider.hpp"
#include "scene_type_system.hpp"
#include "scene_object.hpp"

namespace BW
{
	class ITickSceneProvider
	{
	public:

		virtual void tick( float dTime ) = 0;
		virtual void updateAnimations( float dTime ) = 0;

	};

	class ITickSceneListener
	{
	public:
		virtual void onPreTick() = 0;
		virtual void onPostTick() = 0;
	};

	class TickSceneView : public 
		SceneView<ITickSceneProvider>
	{
	public:
		
		void tick( float dTime );
		void updateAnimations( float dTime );

		void addListener( ITickSceneListener* pListener );
		void removeListener( ITickSceneListener* pListener );

	private:

		typedef BW::vector<ITickSceneListener*> Listeners;
		Listeners listeners_;
	};

	class ITickOperationTypeHandler
	{
	public:
		virtual ~ITickOperationTypeHandler(){}

		virtual void doTick(
			SceneObject* pObjects,
			size_t objectCount, float dTime ) = 0;

		virtual void doUpdateAnimations( 
			SceneObject* pObjects, 
			size_t objectCount, float dTime ) = 0;
	};

	class TickOperation : public
		SceneObjectOperation<ITickOperationTypeHandler>
	{
	public:

		void tick( SceneObject* pObjects, size_t objectCount, float dTime );
		void tick( SceneTypeSystem::RuntimeTypeID typeID,
			SceneObject* pObjects, size_t objectCount, float dTime );

		void updateAnimations( SceneObject* pObjects, size_t objectCount, float dTime );
		void updateAnimations( SceneTypeSystem::RuntimeTypeID typeID,
			SceneObject* pObjects, size_t objectCount, float dTime );
	};

} // namespace BW

#endif
