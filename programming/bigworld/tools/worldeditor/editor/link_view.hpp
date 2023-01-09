#ifndef LINK_VIEW_HPP
#define LINK_VIEW_HPP


#include "gizmo/tool_view.hpp"
#include "gizmo/solid_shape_mesh.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class provides a view a draggable object when trying to link two
 *  objects.
 */
class LinkView : public ToolView
{
public:
    LinkView(LinkProxyPtr linkProxy, Matrix const &startPos);

	void updateAnimations( const Tool& tool );
    /*virtual*/ void render( Moo::DrawContext& drawContext, Tool const &tool);

protected:
    void buildMesh();

private:
    LinkView(LinkView const &);
    LinkView &operator=(LinkView const &);	

protected:
    LinkProxyPtr        linkProxy_;
    SolidShapeMesh      linkMesh_;
	Moo::VisualPtr		linkDrawMesh_;    
    Matrix              startPos_;
};

BW_END_NAMESPACE

#endif // LINK_VIEW_HPP
