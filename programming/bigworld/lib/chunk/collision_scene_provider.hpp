#ifndef COLLISION_SCENE_PROVIDER_HPP__
#define COLLISION_SCENE_PROVIDER_HPP__


#include "cstdmf/bw_vector.hpp"
#include "physics2/bsp.hpp"


BW_BEGIN_NAMESPACE

class CollisionSceneConsumer
{
public:
	virtual ~CollisionSceneConsumer() {}

	virtual void consume( const Vector3& v ) = 0;
	virtual void consumePortal( const Vector3& v ) = 0;
	virtual void flush() = 0;
	virtual bool stopped() const = 0;
};


class CollisionSceneProvider : public SafeReferenceCount
{
public:
	virtual ~CollisionSceneProvider() {}

	virtual void appendCollisionTriangleList( BW::vector<Vector3>& triangleList ) const = 0;
	virtual size_t collisionTriangleCount() const = 0;
	virtual void feed( CollisionSceneConsumer* consumer ) const = 0;
	virtual void feedPortals( CollisionSceneConsumer* consumer ) const { };
};

typedef SmartPointer<CollisionSceneProvider> CollisionSceneProviderPtr;


class BSPTreeCollisionSceneProvider : public CollisionSceneProvider
{
	RealWTriangleSet triangles_;
	RealWTriangleSet portalTriangles_;
	Matrix transform_;

public:
	BSPTreeCollisionSceneProvider( const BSPTree* bsp, const Matrix& transform,
		const RealWTriangleSet& portalTriangles = RealWTriangleSet() );
	virtual void appendCollisionTriangleList( BW::vector<Vector3>& triangleList ) const;
	virtual size_t collisionTriangleCount() const;
	virtual void feed( CollisionSceneConsumer* consumer ) const;
	virtual void feedPortals( CollisionSceneConsumer* consumer ) const;
};


class CollisionSceneProviders
{
	typedef BW::vector<CollisionSceneProviderPtr> Providers;
	Providers providers_;

public:
	void append( CollisionSceneProviderPtr provider );
	void appendCollisionTriangleList( BW::vector<Vector3>& triangleList ) const;
	bool feed( CollisionSceneConsumer* consumer ) const;
	bool feedPortals( CollisionSceneConsumer* consumer ) const;
	bool dumpOBJ( const BW::string& filename ) const;
};

BW_END_NAMESPACE

#endif//COLLISION_SCENE_PROVIDER_HPP__
