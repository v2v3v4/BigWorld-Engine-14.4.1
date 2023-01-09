#ifndef PY_MODEL_OBSTACLE_HPP
#define PY_MODEL_OBSTACLE_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script_math.hpp"
#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}

class PyDye;
class SuperModel;
class ChunkItem;

/**
 *	This class is the python representation of an obstacle made out of a model.
 *	It can be either static or dynamic.
 */
class PyModelObstacle : public PyObjectPlus
{
	Py_Header( PyModelObstacle, PyObjectPlus )

public:
	PyModelObstacle( SuperModel * pSuperModel, MatrixProviderPtr pMatrix = NULL,
		bool dynamic = false, PyTypeObject * pType = &s_type_ );
	virtual ~PyModelObstacle();

	void attach();
	void detach();

	void enterWorld();
	void leaveWorld();

	SuperModel* superModel() const { return pSuperModel_; }

	void tick( float dTime );
	void updateAnimations( const Matrix & worldTransform );
	void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );

	const Matrix & worldTransform() const;

	void localBoundingBox( BoundingBox& bb, bool skinny = false ) const;
	void worldBoundingBox( BoundingBox& bb, const Matrix& world, bool skinny = false ) const;

	bool attached() const							{ return attached_; }
	bool dynamic() const							{ return dynamic_; }
	bool inWorld() const							{ return inWorld_; }

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	PyObject * pyGet_sources();
	PY_RO_ATTRIBUTE_SET( sources )

	PY_RW_ATTRIBUTE_DECLARE( pMatrix_, matrix );

	PY_READABLE_ATTRIBUTE_GET( dynamic_, dynamic );
	int pySet_dynamic( PyObject * val );

	PyObject * pyGet_bounds();
	PY_RO_ATTRIBUTE_SET( bounds )

	PY_RO_ATTRIBUTE_DECLARE( attached_, attached )

	PY_RW_ATTRIBUTE_DECLARE( vehicleID_, vehicleID )

	PyObject * node( const BW::string & nodeName );
	PY_AUTO_METHOD_DECLARE( RETOWN, node, ARG( BW::string, END ) );

	PY_FACTORY_DECLARE()

private:
	SuperModel			* pSuperModel_;
	MatrixProviderPtr	pMatrix_;
	bool				dynamic_;
	bool				attached_;
	bool				inWorld_;
	uint32				vehicleID_;

	BoundingBox			localBoundingBox_;

	typedef BW::map< BW::string, SmartPointer<PyDye> > Dyes;
	Dyes				dyes_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyModelObstacle )

typedef ScriptObjectPtr<PyModelObstacle> PyModelObstaclePtr;

BW_END_NAMESPACE

#endif // PY_MODEL_OBSTACLE_HPP
