#ifndef CHUNK_SPACE_ADAPTER_HPP
#define CHUNK_SPACE_ADAPTER_HPP

#include "scene/intersect_scene_view.hpp"

#include "cstdmf/bw_namespace.hpp"

#include "space/client_space.hpp"
#include "space/client_space_factory.hpp"
#include "space/space_interfaces.hpp"
#include "space/entity_embodiment.hpp"

#include "moo/semi_dynamic_shadow_scene_view.hpp"

#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

class ChunkSpace;
class ChunkEmbodiment;

class WaterSceneRenderer;

class PyOmniLight;
class PySpotLight;

typedef SmartPointer< ChunkSpace > ChunkSpacePtr;
typedef SmartPointer< ChunkEmbodiment > ChunkEmbodimentPtr;

class ClientChunkSpaceFactory :
	public IClientSpaceFactory
{
public:
	ClientChunkSpaceFactory();

	virtual ClientSpace * createSpace( SpaceID spaceID ) const;
	virtual IEntityEmbodimentPtr createEntityEmbodiment( 
		const ScriptObject& object ) const;
	virtual IOmniLightEmbodiment * createOmniLightEmbodiment(
		const PyOmniLight & pyOmniLight ) const;
	virtual ISpotLightEmbodiment * createSpotLightEmbodiment(
		const PySpotLight & pySpotLight ) const;
};

class ClientChunkSpaceAdapter : 
	public ClientSpace
{
public:

	ClientChunkSpaceAdapter( ChunkSpace * pChunkSpace );
	virtual ~ClientChunkSpaceAdapter();
	
	static ChunkSpacePtr getChunkSpace( const ClientSpacePtr& space );
	static ClientSpacePtr getSpace( const ChunkSpacePtr& space );
	static void init();
	class EntityEmbodiment :
		public IEntityEmbodiment
	{
	public:
		static void registerHandlers( Scene & scene );

	public:
		EntityEmbodiment( const ChunkEmbodimentPtr& object );
		virtual ~EntityEmbodiment();

	protected:
		virtual void doMove( float dTime );

		virtual void doWorldTransform( const Matrix & transform );
		virtual const Matrix & doWorldTransform() const;
		virtual const AABB& doLocalBoundingBox() const;

		virtual bool doIsOutside() const;
		virtual bool doIsRegionLoaded( Vector3 testPos, float radius ) const;

		virtual void doDraw( Moo::DrawContext & drawContext );

		virtual void doEnterSpace( ClientSpacePtr pSpace, bool transient = false );
		virtual void doLeaveSpace( bool transient = false );

	private:
		ChunkEmbodimentPtr embodiment_;
		mutable BoundingBox localBB_;
	};

protected:

	virtual bool doAddMapping( GeometryMappingID mappingID,
		Matrix& transform, const BW::string & path, 
		const SmartPointer< DataSection >& pSettings );
	virtual void doDelMapping( GeometryMappingID mappingID );

	virtual float doGetLoadStatus( float distance ) const;

	virtual AABB doGetBounds() const;
	virtual Vector3 doClampToBounds( const Vector3& position ) const;
	virtual float doCollide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc ) const;
	virtual float doCollide( const WorldTriangle & triangle, 
		const Vector3 & translation, 
		CollisionCallback & cc = CollisionCallback_s_default ) const;
	virtual float doFindTerrainHeight( const Vector3 & position ) const;

	virtual void doClear();
	virtual EnviroMinder & doEnviro();
	virtual void doTick( float dTime );
	virtual void doUpdateAnimations( float dTime );

	virtual const Moo::DirectionalLightPtr & doGetSunLight() const;
	virtual const Moo::Colour & doGetAmbientLight() const;
	virtual const Moo::LightContainerPtr & doGetLights() const;
	virtual bool doGetLightsInArea( const AABB& worldBB, 
		Moo::LightContainerPtr* allLights );

	class SceneProvider : 
		public BW::SceneProvider,
		public IIntersectSceneViewProvider,
		public SpaceInterfaces::IPortalSceneViewProvider,
		public BW::Moo::ISemiDynamicShadowSceneViewProvider
	{
	public:
		SceneProvider( ChunkSpace* pChunkSpace );

		virtual bool setClosestPortalState( const Vector3 & point,
			bool isPermissive, CollisionFlags collisionFlags );

		virtual size_t intersect( const SceneIntersectContext& context,
			const ConvexHull& hull, 
			IntersectionSet & intersection) const;

		virtual void * getView( 
			const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID );

		virtual void updateDivisionConfig( Moo::IntBoundBox2& bounds, 
			float& divisionSize );

		virtual void addDivisionBounds( const Moo::IntVector2& divisionCoords, 
			AABB& divisionBounds );

	private:
		class Detail;

		ChunkSpace * pChunkSpace_;
	};

private:

	SceneProvider provider_;
	SmartPointer< ChunkSpace > pChunkSpace_;

};

BW_END_NAMESPACE

#endif // CHUNK_SPACE_ADAPTER_HPP
