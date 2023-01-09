#ifndef VISUAL_HPP
#define VISUAL_HPP

#include "moo_math.hpp"
#include "managed_texture.hpp"
#include "node.hpp"
#include "node_catalogue.hpp"
#include "vertices.hpp"
#include "primitive.hpp"
#include "material.hpp"
#include "animation.hpp"
#include "math/boundbox.hpp"
#include "math/vector3.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"
#include "vertex_formats.hpp"
#include "effect_material.hpp"
#include "reload.hpp"
#include "complex_effect_material.hpp"

BW_BEGIN_NAMESPACE

class BSPTree;
class BSP;
class WorldTriangle;
typedef BW::vector<WorldTriangle> RealWTriangleSet;
typedef AVectorNoDestructor< Matrix >	DrawVector;

namespace Moo
{
	class DrawContext;

/**
 *	Data defining the perimeter of an exported portal
 */
class PortalData : public BW::vector<Vector3>
{
public:
	PortalData() : flags_( 0 )	{ }

	int flags() const			{ return flags_; }
	void flags( int f )			{ flags_ = f; }

	bool isInvasive() const		{ return !!(flags_ & 1); }

private:
	int	flags_;
};

/**
 *	A proxy to a boolean value
 */
class BoolProxy : public ReferenceCount
{
public:
	BoolProxy() : state_( true ) { }
	operator bool() { return state_; }
	bool state_;
};

typedef SmartPointer< BoolProxy > BoolProxyPointer;

/**
 *	A helper class to provide indirect references to BSPs
 */
class BSPProxy : public SafeReferenceCount
{
public:
	BSPProxy( BSPTree * pTree );
	~BSPProxy();

	operator BSPTree * () const { return pTree_; }
	BSPTree * pTree() const	{ return pTree_; }

private:
	BSPProxy( const BSPProxy & );
	BSPProxy & operator =( const BSPProxy & );
	BSPTree * pTree_;
};

typedef SmartPointer< BSPProxy > BSPProxyPtr;

typedef SmartPointer< class Visual > VisualPtr;

/**
 *	This class is a helper class for the visual draw methods to set
 *	up common properties before drawing.
 */
class VisualHelper
{
public:
	VisualHelper();
	bool shouldDraw( bool ignoreBoundingBox, const BoundingBox& bb );
	void start( const BoundingBox& bb, bool rendering = true );
	void fini( bool rendering = true );
	const BoundingBox& worldSpaceBB() { return wsBB_; }
	const Matrix& worldViewProjection() { return worldViewProjection_; }

private:
	LightContainerPtr pRCLC_;
	LightContainerPtr pRCSLC_;
	LightContainerPtr pLC_;
	LightContainerPtr pSLC_;
	BoundingBox wsBB_;
	Matrix worldViewProjection_;
};


/**
 *	This class stores data for a constraint stored in the visual.
 *	@see IKConstraintSystem for an example of how this data could be used.
 */
class ConstraintData
{
public:
	///Enumerates the possible types of constraint.
	enum ConstraintType
	{
		CT_AIM, CT_ORIENT, CT_POINT, CT_PARENT, CT_POLE_VECTOR
	};

	///Enumerates the possible types of constraint target.
	enum TargetType
	{
		TT_MODEL_NODE, TT_IK_HANDLE
	};

	///Enumerates the possible types of constraint source.
	enum SourceType
	{
		ST_MODEL_NODE, ST_CONSTRAINT
	};


	/**
	 *	Creates a ConstraintData object describing a constraint in the
	 *	visual data. See below for a description of the members.
	 */
	ConstraintData(
		ConstraintType type, const BW::string & identifier,
		TargetType targetType, const BW::string & targetIdentifier,
		SourceType sourceType, const BW::string & sourceIdentifier,
		float weight,
		const Vector3 & rotationOffset,
		const Vector3 & translationOffset,
		const Vector3 & aimVector = Vector3::K,
		const Vector3 & upVector = Vector3::J,
		const Vector3 & worldUpVector = Vector3::J )
		:
		type_( type ),
		identifier_( identifier ),
		targetType_( targetType ),
		targetIdentifier_( targetIdentifier ),
		sourceType_( sourceType ),
		sourceIdentifier_( sourceIdentifier ),
		weight_( weight ),
		rotationOffset_( rotationOffset ),
		translationOffset_( translationOffset ),
		aimVector_( aimVector ),
		upVector_( upVector ),
		worldUpVector_( worldUpVector )
	{
	}
	
	///The type of this constraint.
	ConstraintType type_;
	///The name identifying this constraint.
	BW::string identifier_;

	///The type of target for this constraint (what it's modifying).
	TargetType targetType_;
	///The name identifying the target.
	BW::string targetIdentifier_;

	///The type of source for this constraint (provides input transform).
	SourceType sourceType_;
	///The name identifying the source.
	BW::string sourceIdentifier_;

	///The weighting for this constraint among those modifying the same
	///data on the same target.
	float weight_;

	///Rotation offsets (x, y, then z rotations in radians), where applicable.
	Vector3 rotationOffset_;
	///Translation offsets (x, y, then z), where applicable.
	Vector3 translationOffset_;

	///Only applicable for CT_AIM, this is the aiming vector in model space.
	Vector3 aimVector_;
	///Only applicable for CT_AIM, this is the up direction in model space.
	Vector3 upVector_;
	///Only applicable for CT_AIM, this is the up direction in world space.
	Vector3 worldUpVector_;
};


/**
 *	This class stores data for an IK handle stored in the visual.
 *	@see IKConstraintSystem for an example of how this data could be used.
 */
class IKHandleData
{
public:
	/**
	 *	Creates a IKHandleData object describing an IK Handle in the
	 *	visual data. See below for a description of the members.
	 */
	IKHandleData(
		const BW::string & identifier,
		const BW::string & startNodeId, const BW::string & endNodeId,
		const Vector3 & poleVector, const Vector3 & targetPosition )
		:
		identifier_( identifier ),
		startNodeId_( startNodeId ),
		endNodeId_( endNodeId ),
		poleVector_( poleVector ),
		targetPosition_( targetPosition )
	{
	}
	
	///The name identifying this IK handle.
	BW::string identifier_;

	///The start node for the IK handle.
	BW::string startNodeId_;
	///The end node for the IK handle (that will ideally move onto the target).
	BW::string endNodeId_;
	
	///Pole vector controlling the IK solver rotation.
	Vector3 poleVector_;
	///Initial target position in model space.
	Vector3 targetPosition_;
};


/**
 *	This class is the basic drawable shaded mesh object in Moo.
 *
 *	It collects indices and vertices into Geometry structures,
 *	and organises those with common nodes into RenderSets,
 *	such that all the Geometry in a RenderSet may be drawn with the
 *	the same node matrices in the GPU's vertex shader constants.
 *
 *	It also maintains a number of related ancillary structures,
 *	which are derived from the mesh and used by other BigWorld systems.
 */
class Visual :
	public SafeReferenceCount,
/*
*	To support visual reloading, if class A is pulling info out from Visual,
*	A needs be registered as a listener of this pVisual
*	and A::onReloaderReloaded(..) needs be implemented which re-pulls info
*	out from pVisual after pVisual is reloaded. A::onReloaderPreReload() 
*	might need be implemented, if needed, which detaches info from 
*	the old pVisual before it's reloaded.
*	Also when A is initialising from Model, you need call pVisual->beginRead()
*	and pVisual->endRead() to avoid concurrency issues.
*/
	public Reloader
{
public:

	/*
	 * Types Section
	 */

	/*
	 *	Loading flags to allow partial loading (geometry-only for flora for
	 *	example).
	 */
	enum
	{
		LOAD_ALL = 0,
		LOAD_GEOMETRY_ONLY = 1,
	};

	/*
	 * Typedefs needed by VisualLoader.
	 */
	typedef ComplexEffectMaterial 	Material;

	/**
	* Class needed by VisualLoader. Caches BSP trees.
	*/
	class BSPCache
	{
	public:
		void add( const BW::string& , BSPProxyPtr& pBSP );
		void del( const BW::string& );
		BSPProxyPtr& find( const BW::string& );
		static BSPCache& instance();
	};

	/**
	* Class needed by VisualLoader. A bit of a dodgy class to
	* automatically convert a BSPTree* to a BSPProxyPtr.
	*/
	class BSPTreePtr : public BSPProxyPtr
	{
	public:
		BSPTreePtr( const BSPProxyPtr & other ) : BSPProxyPtr( other ) {}
		BSPTreePtr( BSPTree * pTree = NULL ) :
			BSPProxyPtr( pTree ? new BSPProxy( pTree ) : NULL )
		{}
	};

	/**
	* Class needed by VisualLoader. Stores EffectMaterial.
	*/
	class Materials : public BW::vector<ComplexEffectMaterialPtr>
	{
	public:
		const ComplexEffectMaterialPtr find( const BW::string& identifier ) const;
	};

	/**
	* This structure holds an index to a Primitive::PrimGroup structure that 
	* and a pointer to material common to the triangles in there.
	*/
	struct PrimitiveGroup
	{
		uint32				groupIndex_;
		ComplexEffectMaterialPtr	material_;
	};

	typedef BW::vector< PrimitiveGroup > PrimitiveGroupVector;

	/**
	* This structure holds groups of primitives, each with a different material.
	*/
	struct Geometry
	{
		VerticesPtr				vertices_;
		PrimitivePtr			primitives_;
		PrimitiveGroupVector	primitiveGroups_;

		uint32 nTriangles() const;
	};

	typedef BW::vector< Geometry > GeometryVector;

	/**
	* This class holds nodes and their corresponding geometry. The sets
	* may be treated as being in world space or object space.
	*/
	class RenderSet
	{
	public:
		bool				treatAsWorldSpaceObject_;
		NodePtrVector		transformNodes_;
		GeometryVector		geometry_;
		Matrix				firstNodeStaticWorldTransform_;
	};

	typedef BW::vector< RenderSet > RenderSetVector;
	typedef BW::vector< AnimationPtr > AnimationVector;


	struct ReadGuard
	{
		ReadGuard( Visual* visual );
		~ReadGuard();
	private:
		Visual* visual;
	};

	/*
	 * Function Section
	 */

	explicit Visual( const BW::StringRef& resourceID, bool validateNodes = true, uint32 loadingFlags = LOAD_ALL );
	~Visual();

	void isInVisualManager( bool newVal ) { isInVisualManager_ = newVal; }

	HRESULT	draw( Moo::DrawContext& drawContext, bool ignoreBoundingBox = false,
				bool useDefaultPose = true );

	//-- To use instancing we have to have sm 3.0 support.
	void						justDrawPrimitivesInstanced(uint offset, uint numInstances);
	void						justDrawPrimitives();

	bool						createCopy( 
									const VertexFormat& retFormat, 
									void*&				retPVertices, 
									IndicesHolder&		retPIndices,
									uint32&				retNumVertices, 
									uint32&				retNumIndices, 
									EffectMaterialPtr &	material );

	uint32						nVertices() const;
	uint32						nTriangles() const;
	uint32						nVerticesAsFlora()	const;
	uint32						vertexBufferMemory() const;
	void						getVertices(BW::map<void*, int>& vertices);

	void						clampVerticesToBound( float bound );

	NodePtr						rootNode( ) const;

	AnimationPtr				animation( uint32 i ) const;
	uint32						nAnimations( ) const;
	void						addAnimation( AnimationPtr animation );
	AnimationPtr				addAnimation( const BW::string& animResourceID );
	AnimationPtr				findAnimation( const BW::string& identifier ) const;

	const BoundingBox&			boundingBox() const;
	void						boundingBox( const BoundingBox& bb );

	uint32						nPortals( void ) const;
	const PortalData&			portal( uint32 i ) const;
	const BSPTree *				pBSPTree() const { return pBSP_ ? (BSPTree*)(*pBSP_) : NULL; }
	
	uint32						nConstraints() const;
	const ConstraintData &		constraint( uint32 i ) const;
	uint32						nIKHandles() const;
	const IKHandleData &		ikHandle( uint32 i ) const;
	

	//-- ToDo: reconsider this section.
	void						overrideMaterial( const BW::string& materialIdentifier, const ComplexEffectMaterial& mat );
	int							gatherMaterials( const BW::string & materialIdentifier, BW::vector< PrimitiveGroup * > & primGroups, ConstComplexEffectMaterialPtr * ppOriginal = NULL );
	int							collateOriginalMaterials( BW::vector< ComplexEffectMaterialPtr > & rppMaterial );
	Materials&					getMaterials()	{ return ownMaterials_; }
	float						uvSpaceDensity();
	float						calculateUVSpaceDensity();
	void						generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );

	void						useExistingNodes( NodeCatalogue & nodeCatalogue );

	void						instancingStream(VertexBuffer* stream) { instancingStream_ = stream; }

	const BW::string &			resourceID() const		{ return resourceID_; }
#ifdef EDITOR_ENABLED
	bool						updateBSP();
#endif

	bool						primGroup0(
									Moo::VerticesPtr & retVerts,
									Moo::PrimitivePtr & retPrim,
									Moo::Visual::PrimitiveGroup* &retPPrimGroup );

	RenderSetVector&			renderSets() { return renderSets_; }

	bool						isOK() const { return isOK_; }

	static BSPProxyPtr			loadBSPVisual( const BW::string& resourceID );

	void						beginRead();
	void						endRead();

#if ENABLE_RELOAD_MODEL
private:

	ReadWriteLock				reloadRWLock_;
	static bool					validateFile( const BW::string& resourceID );
public:
	bool						reload( bool doValidateCheck );
#endif//ENABLE_RELOAD_MODEL

private:
	void						addLightsInModelSpace( const Matrix& invWorld );
	void						addLightsInWorldSpace( );

	void						load( uint32 loadingFlags = LOAD_ALL );
	void						release();
	bool						isInVisualManager_;

	AnimationVector				animations_;
	NodePtr						rootNode_;
	RenderSetVector				renderSets_;
	BW::vector<PortalData>		portals_;
	BoundingBox					bb_;	
	float						uvSpaceDensity_;
	Materials					ownMaterials_;

	VertexBuffer*				instancingStream_;

	///Stores all constraints defined for this visual.
	BW::vector<ConstraintData>	constraints_;
	///Stores all IK handles defined for this visual.
	BW::vector<IKHandleData>	ikHandles_;

	BSPProxyPtr					pBSP_;
	bool						validateNodes_;
    BW::string					resourceID_;
	bool						isOK_;

	float						clampedToBound_;

	Visual( const Visual & );
	Visual & operator=( const Visual & );
};

/**
 * Helper class for pushing visual instance data to effect visual context
 * Calculates matrix palette out of transform nodes if required
*/
class EffectVisualContextSetter
{
public:
	EffectVisualContextSetter( Visual::RenderSet* pRenderSet );
	~EffectVisualContextSetter();

private:
	// each palette entry is represented by 3 Vector4
	static const uint32 NUM_VECTOR4_PER_PALETTE_MATRIX = 3;
	static const uint32 MAX_WORLD_PALETTE_SIZE = 256 * NUM_VECTOR4_PER_PALETTE_MATRIX;

	Vector4			matrixPalette_[MAX_WORLD_PALETTE_SIZE];
};

/**
* Holder for Moo/Physics BSP code
*/
namespace BSPTreeHelper
{
	void createVertexList( 
		const BSPTree &				sourceTree,
		BW::vector< Moo::VertexXYZL >& verts, 
		uint32							colour = 0xFFFFFFFF );
};

} // namespace Moo

#ifdef CODE_INLINE
#include "visual.ipp"
#endif

BW_END_NAMESPACE


#endif // VISUAL_HPP
