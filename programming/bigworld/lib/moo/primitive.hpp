#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include "moo_math.hpp"
#include "moo_dx.hpp"
#include "com_object_wrap.hpp"
#include "cstdmf/smartpointer.hpp"
#include "material.hpp"
#include "index_buffer.hpp"
#include "primitive_file_structs.hpp"
#include "reload.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

typedef SmartPointer< class Primitive > PrimitivePtr;
typedef SmartPointer< class Vertices > VerticesPtr;

/**
 * This class represents a collection of grouped indices. The file format for
 * this is a .primitive file.
 */
class Primitive : 
	public SafeReferenceCount,
/*
*	To support visual reloading, if class A is pulling info out from Primitive,
*	A needs be registered as a listener of this pPrimitive
*	and A::onReloaderReloaded(..) needs be implemented which re-pulls info
*	out from pPrimitive after pPrimitive is reloaded. A::onReloaderPreReload()
*	might need be implemented, if needed, which detaches info from 
*	the old pPrimitive before it's reloaded.
*/
	public Reloader
{
public:

	virtual HRESULT		setPrimitives();
	virtual HRESULT		drawPrimitiveGroup( uint32 groupIndex );
	virtual HRESULT		drawInstancedPrimitiveGroup( uint32 groupIndex, uint32 instanceCount );
	virtual HRESULT		release( );
	virtual HRESULT		load( );

	const IndexBuffer&		indexBuffer() const;

	uint32				nPrimGroups() const;
	const PrimitiveGroup&	primitiveGroup( uint32 i ) const;
	uint32				maxVertices() const;

	const BW::string&	resourceID() const;
	void				resourceID( const BW::string& resourceID );

	D3DPRIMITIVETYPE	primType() const;

	uint32				nIndices() const { return nIndices_; }
	D3DFORMAT			indicesFormat() const;
 
	void				indicesIB( IndicesHolder& indices );
	const IndicesHolder& indices();


	Primitive( const BW::string& resourceID );

	const Vector3& origin( uint32 i ) const;
	void calcGroupOrigins( const VerticesPtr verts );
	bool adoptGroupOrigins( BW::vector<Vector3>& origins );

#if ENABLE_RELOAD_MODEL
	bool				reload( bool doValidateCheck );
private:
	static bool			validateFile( const BW::string& resourceID );
#endif //ENABLE_RELOAD_MODEL

protected:
	virtual ~Primitive();
private:
	friend class PrimitiveManager;
	virtual void destroy() const;

	typedef BW::vector< PrimitiveGroup > PrimGroupVector;


	PrimGroupVector		primGroups_;

	BW::vector<Vector3>	groupOrigins_;

	uint32				nIndices_;
	uint32				maxVertices_;
	BW::string			resourceID_;
	D3DPRIMITIVETYPE	primType_;
	IndicesHolder		indices_;
	IndexBuffer			indexBuffer_;

#if ENABLE_RELOAD_MODEL
private:
	bool	isInPrimitiveManger_;
public:
	void isInPrimitiveManger( bool newVal ) { isInPrimitiveManger_ = newVal; }
#endif //ENABLE_RELOAD_MODEL

};

} // namespace Moo

#ifdef CODE_INLINE
#include "primitive.ipp"
#endif

BW_END_NAMESPACE

#endif
