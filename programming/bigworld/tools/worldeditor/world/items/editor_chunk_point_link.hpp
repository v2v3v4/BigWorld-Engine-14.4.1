#ifndef EDITOR_CHUNK_POINT_LINK
#define EDITOR_CHUNK_POINT_LINK


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a link from an item to an arbitrary point.
 */
class EditorChunkPointLink : public EditorChunkLink
{
	DECLARE_EDITOR_CHUNK_ITEM(EditorChunkPointLink)

public:
	EditorChunkPointLink();

	virtual void endPoint( const Vector3& endPoint, const BW::string& chunkId );

	virtual bool getEndPoints(
		Vector3 &s, Vector3 &e, bool absoluteCoords ) const;

	virtual void draw( Moo::DrawContext& drawContext );

	virtual void drawInternal();

	virtual float collide(
		const Vector3& source, const Vector3& dir, WorldTriangle& wt ) const;

private:
	Vector3 endPoint_;
	BW::string chunkId_;
    Moo::BaseTexturePtr texture_;
};

typedef SmartPointer<EditorChunkPointLink>   EditorChunkPointLinkPtr;

BW_END_NAMESPACE

#endif // _EDITOR_CHUNK_POINT_LINK_