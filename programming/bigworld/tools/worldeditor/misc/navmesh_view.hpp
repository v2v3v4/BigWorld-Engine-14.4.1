#ifndef NAVMESH_VIEW_HPP
#define NAVMESH_VIEW_HPP

#include "moo/draw_context.hpp"

BW_BEGIN_NAMESPACE

class Chunk;


class NavmeshView : public Moo::DrawContext::UserDrawItem
{
	bool active_;
	float currentGirth_;

	struct Mesh
	{
		float minHeight_;
		float maxHeight_;
		int edgeCount_;
	};

	struct Edge
	{
		float x_;
		float z_;
		int connection_;
	};

	struct RenderSet
	{
		Moo::VertexBuffer edges_;
		UINT edgeCount_;
		Moo::VertexBuffer triangles_;
		UINT triangleCount_;
	};

	typedef BW::vector<RenderSet> RenderSets;
	BW::map<float, RenderSets> girthRenderSets_;

	void preDraw( float size );
	void postDraw();

	void populateGirth( const unsigned char* navmesh, int* offset, float yBias );

public:
	NavmeshView( const BW::vector<unsigned char>& navmesh, float yBias );

	void addToChannel( Moo::DrawContext& drawContext, const Vector3& centre, float girth );

	virtual void draw();
	virtual void fini();
};


class RecastDebugView : public SafeReferenceCount
{
	BW::map<Chunk*, Moo::VertexBuffer> chunkVBs_;

public:
	void render();

	void add( Chunk* chunk );
	void remove( Chunk* chunk );
};


typedef SmartPointer<RecastDebugView> RecastDebugViewPtr;

BW_END_NAMESPACE

#endif //NAVMESH_VIEW_HPP
