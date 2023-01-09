#ifndef SOLID_SHAPE_MESH_HPP
#define SOLID_SHAPE_MESH_HPP

#include "moo/vertex_formats.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer<class Picker> PickerPtr;

namespace Moo
{
	class RenderContext;
}

/**
 * This class is the baseclass for the picker objects.
 */
class Picker : public ReferenceCount
{
public:
	virtual bool		intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL ) const = 0;
protected:
};


/**
 * This class contains information to collide with a sphere.
 */
class SpherePicker : public Picker
{
public:
	SpherePicker( const Vector3& position, float radius, const Matrix& transform = Matrix::identity );
	bool intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL  ) const;

	const Vector3& position() const { return position_; };
	float radius() const { return radius_; };
	float radiusSquared() const { return radiusSquared_; };
	const Matrix& transform() const { return transform_; };
	const Matrix& invTransform() const { return invTransform_; };

protected:
	Vector3 position_;
	float	radius_;
	float	radiusSquared_;
	Matrix	transform_;
	Matrix  invTransform_;
};

/**
 * This class contains information to collide with a box.
 */
class BoxPicker : public Picker
{
public:
	BoxPicker( const Vector3& minValues, const Vector3& maxValues, const Matrix& transform = Matrix::identity );
	bool intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL  ) const;

	const Vector3& minValues() const { return min_; };
	const Vector3 maxValues() const { return max_; };
	const Matrix& transform() const { return transform_; };
	const Matrix& invTransform() const { return invTransform_; };

protected:
	Vector3 min_;
	Vector3 max_;
	Matrix	transform_;
	Matrix  invTransform_;
};

/**
 * This class contains information to collide with a cylinder.
 */
class CylinderPicker : public Picker
{
public:
	CylinderPicker( const Vector3& position, float radius, float length, bool capFront = true, bool capBack = true, const Matrix& transform = Matrix::identity );
	bool intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL  ) const;

	const Matrix& transform() const { return transform_; };
	const Matrix& invTransform() const { return invTransform_; };

protected:

	bool intersectsDisc( const Vector3& discPosition, float discSign, const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL ) const;

	Vector3	position_;
	float	radius_;
	float	radiusSquared_;
	float	length_;
	bool	capFront_;
	bool	capBack_;
	Matrix	transform_;
	Matrix  invTransform_;
};

/**
 * This class contains information to collide with a cone.
 */
class ConePicker : public Picker
{
public:
	ConePicker( const Vector3& position, float radius, float length, bool cap = true, const Matrix& transform = Matrix::identity );
	bool intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL  ) const;

	const Matrix& transform() const { return transform_; };
	const Matrix& invTransform() const { return invTransform_; };

protected:

	bool intersectsDisc( const Vector3& discPosition, float discSign, const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL ) const;

	Vector3	position_;
	float	radius_;
//	float	radiusSquared_;
	float	length_;
	bool	cap_;
	Matrix	transform_;
	Matrix  invTransform_;
};

/**
 * This class contains information to collide with a disc.
 */
class DiscPicker : public Picker
{
public:
	DiscPicker( const Vector3& position, float innerRadius, float outerRadius, const Matrix& transform = Matrix::identity );
	bool intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL  ) const;

	const Matrix& transform() const { return transform_; };
	const Matrix& invTransform() const { return invTransform_; };

protected:

	bool intersectsDisc( const Vector3& discPosition, float discSign, const Vector3& origin, const Vector3& direction, Vector3* intersection = NULL ) const;

	Vector3	position_;
	float	innerRadius_;
	float	outerRadius_;
	float	innerRadiusSquared_;
	float	outerRadiusSquared_;
	Matrix	transform_;
	Matrix  invTransform_;
};

/**
 * This class is the base class of the collision callback.
 */
class  ShapePart : public ReferenceCount
{
public:
	virtual ~ShapePart(){};
};

typedef SmartPointer<ShapePart> ShapePartPtr;

/**
 * This class helps create visualisations of geometric gizmos, it also provides 
 * a way of colliding with them.
 */
class SolidShapeMesh : public ReferenceCount
{
public:
	SolidShapeMesh();
	void addSphere( const Vector3& pos, float radius, uint32 colour = 0xffffffff, ShapePartPtr callback = NULL );
	void addCone( const Vector3& pos, float radius, float length, bool cap = true, uint32 colour = 0xffffffff, ShapePartPtr callback = NULL );
	void addCylinder( const Vector3& pos, float radius, float length, bool capFront = true, bool capBack = true, uint32 colour = 0xffffffff, ShapePartPtr callback = NULL );
	void addBox( const Vector3& minValues, const Vector3& maxValues, uint32 colour = 0xffffffff, ShapePartPtr callback = NULL );
	void addDisc( const Vector3& position, float innerRadius, float outerRadius, uint32 colour = 0xffffffff, ShapePartPtr callback = NULL );

	BW::vector< Moo::VertexXYZND >& verts() { return vertices_; };
	BW::vector< uint16 >& indices() { return indices_; };
	
	const Matrix&	transform() const { return transform_; };
	void			transform( const Matrix& transform );
	ShapePart*		intersects( const Vector3& origin, const Vector3& direction, float* distance = NULL );

    void draw(Moo::RenderContext &rc);

    void clear();

private:
	void			addVertex( const Moo::VertexXYZND& vert );

	BW::vector< Moo::VertexXYZND >		vertices_;
	BW::vector< uint16 >				indices_;
	Matrix								transform_;
	Matrix								normalTransform_;
	BW::vector< PickerPtr >			pickers_;
	BW::vector< ShapePartPtr >         callbacks_;
};

typedef SmartPointer<SolidShapeMesh> SolidShapeMeshPtr;


BW_END_NAMESPACE
#endif // SOLID_SHAPE_MESH_HPP
