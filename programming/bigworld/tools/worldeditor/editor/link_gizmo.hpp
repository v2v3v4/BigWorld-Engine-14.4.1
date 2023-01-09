#ifndef LINK_GIZMO_HPP
#define LINK_GIZMO_HPP

#include "gizmo/genprop_gizmoviews.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/solid_shape_mesh.hpp"
#include "input/input.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

struct  LinkGizmoColourPart;


/**
 *  This class represents a Gizmo that allows items to be interactively
 *  linked together.
 */
class LinkGizmo : public Gizmo
{
public:
    LinkGizmo
    (
        LinkProxyPtr    linkProxy,
        MatrixProxyPtr  center, 
        uint32          enablerModifier     = MODIFIER_ALT
    );

    /*virtual*/ ~LinkGizmo();

	void update( LinkProxyPtr linkProxy, MatrixProxyPtr center );

    /*virtual*/ bool draw( Moo::DrawContext& drawContext, bool force );

    /*virtual*/ bool intersects( Vector3 const &origin,
							Vector3 const &direction, float &t, bool force );

    void click(Vector3 const &origin, Vector3 const &direction);

    void rollOver(Vector3 const &origin, Vector3 const &direction);

    static Vector3  rightLinkCenter;
    static Vector3  leftLinkCenter;
    static float    linkRadius;
    static DWORD    linkColour;

    static Vector3  centerAddUL;
    static Vector3  centerAddLR;
    static float    heightAdd;
    static float    widthAdd;
    static float    breadthAdd;
    static DWORD    addColour;

    static DWORD    unlitColour;

protected:
    void rebuildMeshes(bool force);

    Matrix objectTransform() const;
	Matrix gizmoTransform() const;

    void clickAdd(Vector3 const &origin, Vector3 const &direction, bool upper);

    void clickLink(Vector3 const &origin, Vector3 const &direction, bool left);

private:
    LinkGizmo(LinkGizmo const &);               // not permitted
    LinkGizmo &operator=(LinkGizmo const &);    // not permitted

protected:
	void init();

    LinkProxyPtr        linkProxy_;
    MatrixProxyPtr      center_;
	bool				active_;
	bool				inited_;
	uint32              enablerModifier_;
    SolidShapeMesh      linkSelectionMesh_;
	Moo::VisualPtr		linkDrawMesh_;
    SolidShapeMesh      addSelectionMesh_;
	Moo::VisualPtr		addDrawMesh_;
    Moo::Colour         hilightColour_;
    LinkGizmoColourPart *currentPart_;
    Vector3             currentPos_;
};


typedef SmartPointer< LinkGizmo > LinkGizmoPtr;

BW_END_NAMESPACE

#endif // LINK_GIZMO_HPP
