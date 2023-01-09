#ifndef TOOL_LOCATOR_HPP
#define TOOL_LOCATOR_HPP

#include "cstdmf/named_object.hpp"
#include "math/matrix.hpp"
/**
 *	This class keeps the factory methods for all types of Tool Locators
 *	It is right after #include named_object.hpp because borland
 *	dislikes it after some other include files.
 */

#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "math/planeeq.hpp"

BW_BEGIN_NAMESPACE

class ToolLocator;
typedef NamedObject<ToolLocator * (*)()> LocatorFactory;

class Tool;

#define LOCATOR_FACTORY_DECLARE( CONSTRUCT )								\
	static LocatorFactory s_factory;										\
	virtual LocatorFactory & factory() { return s_factory; }				\
	static ToolLocator * s_create() { return new CONSTRUCT; }				\

#define LOCATOR_FACTORY( CLASS )											\
	LocatorFactory CLASS::s_factory( #CLASS, CLASS::s_create );				\

/**
 *	The ToolLocator class positions a tool in the world.
 */
class ToolLocator : public PyObjectPlus 
{
	Py_Header( ToolLocator, PyObjectPlus )
public:
	ToolLocator( PyTypeObject * pType = &s_type_ );
	virtual ~ToolLocator();
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool ) = 0;
	virtual bool positionValid() const { return true; }
	const Matrix& transform() const	{ return transform_; }
	void transform( const Matrix& m )	{ transform_ = m; }
	virtual Vector3 direction() const { return Vector3( 0.f, 1.f, 0.f ); };
protected:
	Matrix transform_;
};

typedef SmartPointer<ToolLocator>	ToolLocatorPtr;

PY_SCRIPT_CONVERTERS_DECLARE( ToolLocator )


/**
 *	This class implements a tool locator that sits at the origin.
 */
class OriginLocator : public ToolLocator
{
	Py_Header( OriginLocator, ToolLocator )
public:
	OriginLocator( PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_FACTORY_DECLARE()
private:
	LOCATOR_FACTORY_DECLARE( OriginLocator() )
};


/**
 *	This class implements a tool locator that sits on a plane.
 */
class PlaneToolLocator : public ToolLocator
{
	Py_Header( PlaneToolLocator, ToolLocator )
public:
	PlaneToolLocator( PlaneEq * pPlane = NULL, PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_FACTORY_DECLARE()
private:
	PlaneEq	planeEq_;
	LOCATOR_FACTORY_DECLARE( PlaneToolLocator() )
};


/**
 *	This class implements a tool locator that sits on the most orthogonal
 *	plane of the 3 major axes to the camera.
 */
class TriPlaneToolLocator : public ToolLocator
{
	Py_Header( TriPlaneToolLocator, ToolLocator )
public:
	TriPlaneToolLocator( const Vector3& pos = s_lastPos, PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_FACTORY_DECLARE()
private:
	PlaneEq planeEq_[3];
	Vector3 fulcrum_;
	static Vector3 s_lastPos;
};


/**
 *	This class implements a tool locator that sits on the most orthogonal
 *	plane to the camera, and takes a position for the planes' fulcra.
 */
class AdaptivePlaneToolLocator : public ToolLocator
{
	Py_Header( AdaptivePlaneToolLocator, ToolLocator )
public:
	AdaptivePlaneToolLocator( const Vector3& pos, PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_FACTORY_DECLARE()
private:
	Vector3 fulcrum_;
};


/**
 *	This class implements a tool locator that sits on a line.
 */
class LineToolLocator : public ToolLocator
{
	Py_Header( LineToolLocator, ToolLocator )
public:
	LineToolLocator( const Vector3& origin = Vector3( 0, 0, 0), const Vector3& direction = Vector3( 0, 0, 1), PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );
	virtual Vector3 direction() const { return direction_; }

	PY_FACTORY_DECLARE()
private:
	Vector3 origin_;
	Vector3 direction_;
	LOCATOR_FACTORY_DECLARE( LineToolLocator() )
};


class ChunkToolLocator : public ToolLocator
{
	Py_Header( ChunkToolLocator, ToolLocator )
public:
	ChunkToolLocator( PyTypeObject * pType = &s_type_ );
};


/**
 *	This class implements a tool locator that finds the chunk a tool is in.
 */
class PlaneChunkLocator : public ChunkToolLocator
{
	Py_Header( PlaneChunkLocator, ChunkToolLocator )
public:
	PlaneChunkLocator( PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_FACTORY_DECLARE()
private:
	LOCATOR_FACTORY_DECLARE( PlaneChunkLocator() )
};

BW_END_NAMESPACE

#endif