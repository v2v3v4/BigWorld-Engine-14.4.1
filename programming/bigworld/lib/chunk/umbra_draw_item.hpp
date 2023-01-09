#ifndef UMBRA_DRAW_ITEM_HPP
#define UMBRA_DRAW_ITEM_HPP

#include "umbra_config.hpp"

#if UMBRA_ENABLE

#include "umbra_proxies.hpp"

namespace Umbra 
{
	namespace OB 
	{
		class Cell;
	}
}


BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}

class Chunk;

/**
 *	This class defines the interface the 
 *	Umbra implementation uses to render objects.
 *	Any object that wants to be rendered through Umbra will need to create a 
 *	derived class and set up the umbra objects.
 *	The class contains three pure virtual methods that need to be implemented
 *	- draw()
 *	- drawDepth()
 *	- drawReflection()
 */
class UmbraDrawItem 
{
public:
	virtual Chunk*	draw( Moo::DrawContext& drawContext, Chunk* pChunkContext ) = 0;
	virtual Chunk*	drawDepth( Moo::DrawContext& drawContext, Chunk* pChunkContext) = 0;
	virtual ~UmbraDrawItem();

	UmbraObjectProxyPtr pUmbraObject() { return pObject_; }

	void updateCell( Umbra::OB::Cell* pNewCell );
	virtual void updateTransform( const Matrix& newTransform );
	virtual void update() {};

protected:
	UmbraObjectProxyPtr	pObject_;
};

class UmbraDrawShadowItem : public UmbraDrawItem
{
public:
	
	UmbraDrawShadowItem() 
		: isDynamicObject_( false ) 
	{}

	const Matrix & transform() const { return objectTr_; }
	const BoundingBox & bbox() const { return objectBB_; }
	bool isDynamic() const { return isDynamicObject_; }

protected:

	Matrix		objectTr_;
	BoundingBox objectBB_;
	bool		isDynamicObject_;
};

BW_END_NAMESPACE

#endif // UMBRA_ENABLE
#endif // UMBRA_DRAW_ITEM_HPP
