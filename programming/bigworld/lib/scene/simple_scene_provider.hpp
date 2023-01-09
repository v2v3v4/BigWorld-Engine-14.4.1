#ifndef SIMPLE_SCENE_PROVIDER_HPP
#define SIMPLE_SCENE_PROVIDER_HPP

#include "forward_declarations.hpp"
#include "scene_provider.hpp"
#include "intersect_scene_view.hpp"
#include "collision_scene_view.hpp"
#include "scene_object.hpp"
#include "intersection_set.hpp"
#include "scene_intersect_context.hpp"

namespace BW
{
	template<class ObjectT>
	class SimpleSceneProvider :
		public SceneProvider,
		public IIntersectSceneViewProvider,
		public ICollisionSceneViewProvider
	{
	public:
		typedef BW::vector<ObjectT*> Objects;

	public:
		SimpleSceneProvider()
		{

		}

		virtual ~SimpleSceneProvider()
		{

		}

		void addObject( ObjectT & obj )
		{
			objects_.push_back( &obj );
		}
		
		void removeObject( ObjectT & obj )
		{
			for (Objects::iterator it = objects_.begin();
				it != objects_.end(); ++it)
			{
				if (*it == &obj)
				{
					std::swap( *it, objects_.back() );
					objects_.pop_back();
					return;
				}
			}
		}

		void tick( float dTime )
		{
			for (Objects::iterator it = objects_.begin();
				it != objects_.end(); ++it)
			{
				(*it)->doTick( dTime );
			}
		}

		void updateAnimations()
		{
			for (Objects::iterator it = objects_.begin();
				it != objects_.end(); ++it)
			{
				(*it)->doUpdateAnimations();
			}
		}

	private:
		// Scene view implementations
		virtual void * getView(
			const SceneTypeSystem::RuntimeTypeID & viewTypeID )
		{
			void * result = NULL;

			exposeView<ICollisionSceneViewProvider>(this, viewTypeID, result);
			exposeView<IIntersectSceneViewProvider>(this, viewTypeID, result);

			return result;
		}

		virtual size_t intersect( const SceneIntersectContext& context,
			const ConvexHull& hull, 
			IntersectionSet & intersection )
		{
			for (Objects::iterator it = objects_.begin();
				it != objects_.end(); ++it)
			{
				SceneObject obj( *it );
				intersection.insert( obj );
			}

			return objects_.size();
		}

		virtual bool collide( const Vector3 & source,
			const Vector3 & extent,
			const SweepParams& sp,
			CollisionState & state ) const
		{
			return false;
		}

		virtual bool collide( const WorldTriangle & source,
			const Vector3 & extent,
			const SweepParams& sp,
			CollisionState & state ) const
		{
			return false;
		}

	private:
		Objects objects_;
	};

} // namespace BW

#endif // SIMPLE_SCENE_PROVIDER_HPP