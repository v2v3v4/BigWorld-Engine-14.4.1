#ifndef VERTICES_HPP
#define VERTICES_HPP

#include "moo_math.hpp"
#include "moo_dx.hpp"
#include "vertex_buffer.hpp"
//#include "dxcontext.hpp"
#include "com_object_wrap.hpp"
#include "cstdmf/smartpointer.hpp"
#include "vertex_declaration.hpp"
#include "reload.hpp"
#include "vertex_streams.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class StreamContainer;

typedef SmartPointer< class Vertices > VerticesPtr;
typedef SmartPointer< class BaseSoftwareSkinner > BaseSoftwareSkinnerPtr;
typedef SmartPointer< class Node > NodePtr;
typedef BW::vector< NodePtr > NodePtrVector;

class VertexFormat;

// enable this to enable debugging of mesh normals
//#define EDITOR_ENABLED
// NOTE: it's not a robust solution for drawing normals
// so use it at your own risk :P
// also needs to be defined in : "romp\super_model.cpp"

/**
 *  Vertices is the class used to load and access a vertex buffer, with one of 
 *  several Vertex formats. A Vertices object is created from a resourceID. This 
 *	in general is a reference to a subsection of a .primitives file ( for 
 *	example "objects/MyBipedObject.primitive/FeetVertices" ).
 *
 *  A Vertices object is created using the VerticesManager::get function.
 *
 *  Before rendering a primitive based on this set of Vertices, call the 
 *	function setVertices, which will load the verts from the file if necessary, 
 *	and present them to Direct3D as the current StreamSource.
 *
 *  The string description of format of the vertices is specified in the 
 *	resource header.
 *
 *  See vertex_formats.hpp for a list of valid formats.
 *
 */
class Vertices  : 
	public SafeReferenceCount,
/*
*	To support visual reloading, if class A is pulling info out from Vertices,
*	A needs be registered as a listener of this pVertices
*	and A::onReloaderReloaded(..) needs be implemented which re-pulls info
*	out from pVertices after pVertices is reloaded. A::onReloaderPreReload() 
*	might need be implemented, if needed, which detaches info from 
*	the old pVertices before it's reloaded.
*/
	public Reloader
{
public:
	Vertices( const BW::string& resourceID, int numNodes );

	virtual HRESULT		load( );
	virtual HRESULT		release( );
	virtual HRESULT		setVertices( bool software,	bool instanced = false );

	void				pullInternals( const bool instanced,
									const VertexDeclaration*& outputVertexDecl,
									const VertexBuffer*& outputDefaultVertexBuffer,
									uint32&	outputDefaultVertexBufferOffset,
									uint32& outputDefaultVertexStride,
									const StreamContainer*& outputStreamContainer);

	virtual HRESULT		setTransformedVertices( bool tb,
		const NodePtrVector& nodes );


	const BW::string&	resourceID( ) const;
	void				resourceID( const BW::string& resourceID );
	uint32				nVertices( ) const;

	const BW::string&	sourceFormat( ) const;	

	uint32				vertexStride() const { return vertexStride_; };
	uint32				vertexStride( uint32 streamIndex ) const;
	uint32				vertexStreamIndex( uint32 streamIndex ) const;
	Moo::VertexBuffer	vertexBuffer( ) const;
	Moo::VertexBuffer	vertexBuffer( uint32 streamIndex ) const;
	uint32 numVertexBuffers() const;
	uint32 vertexBufferOffset( uint32 streamIndex ) const;

	void initFromValues( uint32 numVerts, const char* sourceFormat );
	void addStream( uint32 streamIndex, const VertexBuffer& vb, 
		uint32 offset, uint32 stride, uint32 size );

	HRESULT bindStreams( bool instanced, bool softwareSkinned = false,
		bool bumpMapped = false );

	typedef BW::vector< uint32 > VertexNormals;
	typedef BW::vector< Vector3 > VertexPositions;

	virtual void vertexPositionsVB( VertexPositions& vPositions );
	virtual const VertexPositions & vertexPositions();

	#ifdef EDITOR_ENABLED
	virtual const VertexNormals & vertexNormals();
	virtual const VertexPositions & vertexNormals2();
	virtual const VertexNormals & vertexNormals3();
	#endif //EDITOR_ENABLED
	const VertexDeclaration* pDecl() const {return pDecl_; };
	void				clearSoftwareSkinner() { pSkinnerVertexBuffer_.release(); }
	BaseSoftwareSkinner*	softwareSkinner();

	bool resaveHardskinnedVertices();

	/// This function returns true if the provided format has 'bumped' info.
	static bool isBumpedFormat( const VertexFormat & vertexFormat );

	/// This method returns true if the contained format has 'bumped' info.
	bool hasBumpedFormat() const;

	// indicates if this class has vertex streams
	bool hasStreams() const { return streams_ != NULL; }
	virtual void	resetStream( const uint32 streamNumber ) { VertexBuffer::reset( streamNumber ); }

#if ENABLE_RELOAD_MODEL
	bool reload( bool doValidateCheck );
private:
	static bool	validateFile( const BW::string& resourceID, int numNodes );
#endif //ENABLE_RELOAD_MODEL

protected:
	friend class VerticesManager;
	virtual void destroy() const;
	virtual ~Vertices();

	bool openSourceFiles( DataSectionPtr& pPrimFile, BinaryPtr& vertices, BW::string& partName );

	//TODO: move the default vertexBuffer_ into it's own stream.
	StreamContainer* 	streams_;

	Moo::VertexBuffer  vertexBuffer_;
	uint32 vertexBufferOffset_;
	uint32 vertexBufferSize_;
	VertexDeclaration* pDecl_;
	VertexDeclaration* pInstancedDecl_;
	uint32			nVertices_;
	BW::string		resourceID_;

	BW::string		sourceFormat_;
	uint32			vertexStride_;
	mutable VertexPositions vertexPositions_;
#ifdef EDITOR_ENABLED	
	mutable VertexNormals vertexNormals_;
	mutable VertexPositions vertexNormals2_;
	mutable VertexNormals vertexNormals3_;
#endif //EDITOR_ENABLED
	BaseSoftwareSkinnerPtr pSoftwareSkinner_;
	bool formatSupportsSoftwareSkinner_;
	Moo::VertexBuffer	pSkinnerVertexBuffer_;
	bool				vbBumped_;

	//  This parameter is used to verify bone indices against the number of
	//  bones. A value less or equal to zero means no verification is done.
	int					numNodes_;

public:
	/**
	 * This class holds an array of vertices on the heap, destroying them when
	 * object goes out of scope.
	 */
	template<typename VertexType>
	class ScopedVertexArray
	{
	public:
		ScopedVertexArray() : ptr_( NULL )
		{
		}
		~ScopedVertexArray()
		{
			bw_safe_delete_array(ptr_);
			ptr_ = NULL;
		}
		VertexType* init( const VertexType* src, int count )
		{
			ptr_ = new VertexType[ count ];
			memcpy( ptr_, src, sizeof(*ptr_)*count );
			return ptr_;
		}
	private:
		VertexType* ptr_;
	};
protected:

#ifdef EDITOR_ENABLED
	/**
	 * This function stores vertex normals using VertexFormat
	 */
	static void copyVertexNormals( const VertexFormat & vertexFormat, 
		VertexNormals & vertexNormals, const void * pVerts, uint32 nVertices );

	static void copyVertexNormals( const VertexFormat & vertexFormat, 
		VertexPositions & vertexNormals, const void * pVerts, uint32 nVertices );

	static void copyTangentSpace( const VertexFormat & vertexFormat, 
		VertexNormals & vertexNormals, const void * pVerts, uint32 nVertices );

	/**
	 * This method caches the vertex normals from the vertices
	 */
	bool cacheVertexNormals();

#endif

	/**
	 * This function makes sure all blend indices are within a sensible bounds
	 * with respect to this instance.
	 *	@return	False if a fixup (modification) was required. 
	 */
	static bool verifyFixupBlendIndices( const VertexFormat& vertexFormat, 
		const void* vertices, uint32 nVertices, uint32 nIndices, 
		uint32 nBoneNodes, uint32 streamIndex, uint32 semanticIndex );

	/**
	 * This method performs a fix up of contained blend indices
	 *	@return	False if skipped or nothing changed, True if fixup occurred.
	 */
	bool fixBlendIndices();

	template< typename VertexType >
	bool verifyIndices1( VertexType* vertices, int numVertices )
	{
		BW_GUARD;
		if ( numNodes_ <= 0 )
			return true;

		bool ok = true;
		for ( int i = 0; i < numVertices; ++i )
		{
			if ( vertices[i].index_ < 0 || vertices[i].index_ >= numNodes_ )
			{
				vertices[i].index_ = 0;
				ok = false;
			}
		}
		return ok;
	}

	template< typename VertexType >
	bool verifyIndices3( VertexType * vertices, int numVertices )
	{
		BW_GUARD;
		if ( numNodes_ <= 0 )
			return true;

		bool ok = true;
		for ( int i = 0; i < numVertices; ++i )
		{
			if ( vertices[i].index_ < 0 || vertices[i].index_ >= numNodes_ )
			{
				vertices[i].index_ = 0;
				ok = false;
			}

			if ( vertices[i].index2_ < 0 || vertices[i].index2_ >= numNodes_ )
			{
				vertices[i].index2_ = 0;
				ok = false;
			}

			if ( vertices[i].index3_ < 0 || vertices[i].index3_ >= numNodes_ )
			{
				vertices[i].index3_ = 0;
				ok = false;
			}
		}
		return ok;
	}

	/// check vertex format is in the list we expect
	static bool inFormatNameList( const BW::StringRef & formatName, 
		const char ** formatNamesArray, uint32 nameCount, 
		bool emitErrorMsgs );

private:
	class Detail;

#if ENABLE_RELOAD_MODEL
private:
	bool	isInVerticesManger_;
public:
	void isInVerticeManger( bool newVal ) { isInVerticesManger_ = newVal; }
#endif //ENABLE_RELOAD_MODEL

};

} // namespace Moo

#ifdef CODE_INLINE
#include "vertices.ipp"
#endif

BW_END_NAMESPACE

#endif
/*vertices.hpp*/
