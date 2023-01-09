#ifndef GIZMO_MANAGER_HPP
#define GIZMO_MANAGER_HPP

#include <iostream>
#include "moo/visual.hpp"
#include "pyscript/script.hpp"
#include "cstdmf/bw_vector.hpp"
#include "general_properties.hpp"

BW_BEGIN_NAMESPACE

class Tool;
typedef SmartPointer<Tool>	ToolPtr;
namespace Moo
{
	class DrawContext;
}

class Gizmo : public ReferenceCount
{
public:
	const static int ALWAYS_ENABLED = -1;

	virtual ~Gizmo(){};
	virtual void drawZBufferedStuff( bool force )	{};
	virtual bool draw( Moo::DrawContext& drawContext, bool force ) = 0;
	virtual bool intersects( const Vector3& origin, const Vector3& direction,
													float& t, bool force ) = 0;
	virtual void click( const Vector3& origin, const Vector3& direction ){};
	virtual void rollOver( const Vector3& origin, const Vector3& direction ){};
	virtual void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix ){};
protected:
	virtual Matrix objectTransform() const = 0;
	virtual Matrix gizmoTransform() const;
	Matrix getCoordModifier() const;
	virtual Matrix objectCoord() const;
	virtual void pushTool( ToolPtr pTool );
};

typedef SmartPointer< Gizmo > GizmoPtr;
typedef BW::vector< GizmoPtr > GizmoVector;


/**
 *	This class implements a named set of gizmos. Used by the GizmoManager
 *  to force a particular set of gizmos at a particular time.
 */
class GizmoSet : public ReferenceCount
{
public:
	GizmoSet();

	void clear();
	void add( GizmoPtr gizmo );

	const GizmoVector& vector();

private:
	GizmoVector gizmos_;
};

typedef SmartPointer< GizmoSet > GizmoSetPtr;


/**
 *	This class manages a dynamic set of gizmos, and
 *	provides hit-testing given the world ray of the mouse
 *	cursor.
 *
 *	Clicks are routed through to the gizmo underneath the
 *	cursor.
 *
 *	All gizmos are drawn every frame, unless an active GizmoSet is active, in
 *  which case only the gizmos in the set will be drawn, with the force flag
 *  set to true.
 */
class GizmoManager
{
	typedef GizmoManager This;
public:
	~GizmoManager();

	static GizmoManager & instance();

	void	draw( Moo::DrawContext& drawContext );
	bool	update( const Vector3& worldRay );
	bool	click();
	bool	rollOver();

	void	addGizmo( GizmoPtr pGizmo );
	void	removeGizmo( GizmoPtr pGizmo );
	void	removeAllGizmo();

	void		forceGizmoSet( GizmoSetPtr gizmoSet );
	GizmoSetPtr	forceGizmoSet();

	const Vector3& getLastCameraPosition();

	PY_MODULE_STATIC_METHOD_DECLARE( py_gizmoUpdate )
	PY_MODULE_STATIC_METHOD_DECLARE( py_gizmoClick )

private:
	GizmoVector	gizmos_;

	Vector3		lastWorldRay_;
	Vector3		lastWorldOrigin_;
	GizmoPtr	intersectedGizmo_;

	GizmoSetPtr forcedGizmoSet_;

	GizmoManager();
	GizmoManager( const GizmoManager& );
	GizmoManager& operator=( const GizmoManager& );
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "gizmo_manager.ipp"
#endif

#endif // GIZMO_MANAGER_HPP
