#ifndef TEXTURE_STREAMING_MANAGER_HPP
#define TEXTURE_STREAMING_MANAGER_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/object_pool.hpp"

#include "streaming_manager.hpp"
#include "texture_manager.hpp"
#include "base_texture.hpp"
#include "texture_lock_wrapper.hpp"
#include "vertex_format.hpp"

BW_BEGIN_NAMESPACE

class MatrixProvider;
class SpaceManager;
class ClientSpace;
class Scene;

namespace Moo
{

class TextureStreamingSceneView;
class StreamingTexture;
class ModelTextureUsage;

typedef DX::BaseTexture DeviceTexture;
typedef DX::Texture DeviceTexture2D;

class DebugTextures
{
public:
	enum TextureSet
	{
		/// Display the original texture
		DEBUG_TS_NONE,

		/// Replace the current textures with textures 
		/// that have different colors at each mip level
		/// according to what the current texture's dims are
		DEBUG_TS_CURRENTWORKINGSET,

		/// Replace the current textures with textures
		/// that have different colors at each mip level
		/// according to if there were another mip level 
		/// available for that texture. 
		/// Red = the scene could use more detail
		DEBUG_TS_MISSINGDETAIL,

		/// Same as above, but only use the larger dimensions
		/// if those dimensions are available on disk.
		/// Red = The scene needs a mip from disk loaded.
		DEBUG_TS_MISSINGAVAILABLEDETAIL,

		/// Render all bound spheres being sent to texture
		/// streaming scene evaluation system. Requires
		/// enabling in debugStuff() function.
		DEBUG_TS_OBJECT_BOUNDS,

		/// Replace the current textures with textures
		/// that are different colors according to how
		/// many mips have been skipped from being loaded.
		/// Red = fully loaded.
		DEBUG_TS_NUM_MIPS_SKIPPED,

		/// Replace the current textures with textures
		/// that are different colors according to how
		/// many mips have been sacrificed from the ideal
		/// mip for this texture.
		/// Red = Many sacrificed.
		DEBUG_TS_SACRIFICED_DETAIL,

		/// Replace the current textures with textures
		/// that are different colors according to
		/// the dimensions of the texture on disk.
		DEBUG_TS_TEXTURE_SIZE,

		/// Replace the current textures with textures
		/// that are different colors according to
		/// the dimensions of the texture currently loaded.
		DEBUG_TS_CURRENT_TEXTURE_SIZE
	};

	struct DebugTextureSet_Key
	{
		TextureSet debugSet;
		uint32 width;
		uint32 height; 
		uint32 skipLevels;

		bool operator <(const DebugTextureSet_Key & other)const;
	};

	typedef BW::map<DebugTextureSet_Key, BaseTexturePtr> DebugTextureMap;
	DebugTextureMap debugTextureSet_;

	BaseTexture * getDebugTexture( TextureSet debugSet, BaseTexture * pTex );
	BaseTexture * getDebugTexture( TextureSet debugSet, uint32 width, 
		uint32 height, uint32 skipLevels );

	BaseTexture * createDebugTexture( 
		const DebugTextureSet_Key & debugRequest );

	void clear();
};

//-----------------------------------------------------------------------------

/// Triangle class for use by TexelDensityTriGenFunctor
template < class VertexType >
class Triangle
{
public:
	Triangle( const VertexType & a, const VertexType & b, 
		const VertexType & c ): 
			a_(a), 
			b_(b), 
			c_(c)
	{
	}

	float area() const
	{
		return 0.5f * length( (b_ - a_).crossProduct( c_ - a_ ) );
	}

	VertexType a_;
	VertexType b_;
	VertexType c_;

private:

	static float length( float scalar )
	{
		return abs( scalar );
	}

	static float length( const VertexType & comp )
	{
		return comp.length();
	}
};


//-----------------------------------------------------------------------------

/**
 * This class describes a functor intended to be passed to 
 * generateTrianglesFromIndices(...) so that it can calculate texel density 
 * over sets of triangles without worrying about the details of the triangle's 
 * indexing format or primitive/topology type. The main function is operator() 
 * which is invoked for each index buffer triplet that represents a triangle. 
 * It aggregates a min texel density over calls. Totals and skip thresholds 
 * are queried via the get functions.
 */
class UvDensityTriGenFunctor
{
public:
	typedef ProxyElementAccessor< VertexElement::SemanticType::POSITION > 
		PositionDataAccessor;
	typedef ProxyElementAccessor< VertexElement::SemanticType::TEXCOORD > 
		TexcoordDataAccessor;

	/// Default constructor that results in invalid state. Call to init() required.
	UvDensityTriGenFunctor(const PositionDataAccessor & positionDataAccessor,
		const TexcoordDataAccessor & texcoordDataAccessor);
	UvDensityTriGenFunctor(const VertexFormat & format, const void * buffer, 
		size_t bufferSize,  
		uint32 positionSemanticIndex, 
		uint32 texcoordSemanticIndex,
		uint32 streamIndex = 0);

	/**
	 * This method operates on index triplets, and generates the relevant
	 * triangles via the vertex buffer accessors provided in the constructor.
	 * With these triangles, it does the texel density calculations.
	 */
	bool operator()( uint32 index1, uint32 index2, uint32 index3 );

	/// This method returns the accumulated minimum texel density
	float getMinUvDensity() const;

	/// This method returns the total number of triangles successfully 
	/// processed (not skipped)
	uint32 getTrianglesProcessed() const;

	/// This method returns the total number of triangles skipped 
	/// (i.e. from not meeting consideration thresholds)
	uint32 getTrianglesSkipped() const;

	/// This method returns true only if all the vertex accessors are valid
	bool isValid() const;

	/// Get the minimum world area threshold. Triangles with a world area 
	/// lower than this are skipped
	float getMinWorldAreaThreshold() const;

	/// Get the minimum texel area threshold. Triangles with a texel area 
	/// lower than this are skipped
	float getMinUvAreaThreshold() const;

	/// Set the minimum world area threshold.
	void setMinWorldAreaThreshold( float minWorldAreaThreshold );

	/// Set the minimum texel area threshold.
	void setMinUvAreaThreshold( float minTexelAreaThreshold );

private:
	void initialise();
	
	PositionDataAccessor posData_;
	TexcoordDataAccessor uvData_;

	uint32 trianglesProcessed_;
	uint32 trianglesSkipped_;
	float minUvDensity_;

	float minWorldAreaThreshold_;
	float minUvAreaThreshold_;

	// default thresholds
	static const float DEFAULT_MIN_WORLD_AREA_THRESHOLD;
	static const float DEFAULT_MIN_TEXEL_AREA_THRESHOLD;

};


//------------------------------------------------------------------------------

class TextureStreamingManager
{
public:
	
	struct ViewpointData
	{
		ViewpointData();

		Matrix matrix;
		Vector3 velocity;
		float priorityBoost;
		float requirementBoost;
		float fov;
		float resolution;
	};

	class Viewpoint : public SafeReferenceCount
	{
	public:
		Viewpoint();
		virtual ~Viewpoint();

		enum QueryReturnCode
		{
			SUCCESS,
			INACTIVE,
			UNREGISTER
		};

		void manager( TextureStreamingManager * val );
		TextureStreamingManager * manager() const;

		virtual QueryReturnCode query( ViewpointData & data ) = 0;
		void unregister();

	protected:
		TextureStreamingManager * manager_;
	};

	typedef Handle< 13 > StreamingTextureHandle;
	typedef Handle< 20 > ModelTextureUsageHandle;
	static const uint32 INVALID_MIP = ~0u;

	enum StreamingMode
	{
		/// No streaming performed.
		/// All textures flagged as streamable are loaded to their highest
		/// detail.
		MODE_OFF, 

		/// Textures are loaded to the amounts they need to fullfill
		/// current scene requirements. Unrequired data is unloaded 
		/// immediately. (Default)
		MODE_CONSERVATIVE_NO_BUDGET,

		/// Same as MODE_CONSERVATIVE_NO_BUDGET except textures are
		/// prioritised if they won't all fit into the available
		/// memory budget.
		MODE_CONSERVATIVE,

		/// Same as MODE_CONSERVATIVE except data is only ever unloaded
		/// if the budget is required by another texture of higher 
		/// priority immediately.
		MODE_ON_DEMAND,

		/// Disable texture streaming, do not even allow
		/// streaming textures to be created.
		MODE_DISABLE
	};

	enum PrioritisationMode
	{
		/// Enable applying streaming priority during
		/// sacrifice heuristic calculations.
		PM_ENABLE_STREAMING_PRIORITY = 1 << 0,

		/// Enable the coherent sacrifice modification
		/// to the sacrifice heuristic. Atempts to stabilise
		/// the heuristic against small movements.
		PM_ENABLE_COHERENT_SACRIFICE = 1 << 1,

		/// Enable disabling sacrifice of textures that have been
		/// assigned a streaming priority. This mode behaves
		/// better with the COHERENT_SACRIFICE system
		/// while still allowing the developer to adjust which
		/// textures are going to be affected by sacrifices due
		/// to budget restrictions.
		PM_ENABLE_STREAMING_PRIORITY_DISABLE_SACRIFICE = 1 << 2
	};

	class TextureStreamingUsageCollector
	{
	public:
		void submitTextureRequirements( 
			BaseTexture* pTexture, 
			uint32 requiredMip,
			uint32 numPixelsAssignedOnScreen,
			float angleToFrustumRadians );

	private:
		friend TextureStreamingManager;
		TextureStreamingUsageCollector( TextureStreamingManager* pManager );
		
		TextureStreamingManager* pManager_;
	};

	TextureStreamingManager( TextureManager * pTextureManager );
	~TextureStreamingManager();

	void tryDestroy( const StreamingTexture * texture );

	bool enabled() const;
	bool isCommandLineDisabled() const;
	
	bool canStreamResource( const BW::StringRef & resourceID );
	BaseTexturePtr createStreamingTexture( TextureDetailLevel * detail,
		const BW::StringRef & resourceID, 
		const BaseTexture::FailurePolicy & notFoundPolicy,
		D3DFORMAT format,
		int mipSkip, bool noResize, bool noFilter,
		const BW::StringRef & allocator );
	void unregisterTexture( StreamingTexture * pTex );
	void reloadTexture( StreamingTexture * pTex );

	void generateReport( const BW::StringRef & filename );

	void registerScene( Scene * pScene );
	void unregisterScene( Scene * pScene );

	void registerViewpoint( Viewpoint * pViewpoint );
	void unregisterViewpoint( Viewpoint * pViewpoint );
	void clearViewpoints();

	void tick();
	void drawDebug();

	/// Configuration
	void streamingMode( uint32 mode );
	uint32 streamingMode() const;

	void prioritisationMode( uint32 mode );
	uint32 prioritisationMode() const;
	
	void debugMode( uint32 mode );
	uint32 debugMode() const;

	void minMipLevel( uint32 size );
	uint32 minMipLevel() const;

	void maxMipLevel( uint32 size );
	uint32 maxMipLevel() const;

	void maxMipsPerRequest( uint32 size );
	uint32 maxMipsPerRequest() const;

	void memoryBudget( uint32 budget );
	uint32 memoryBudget() const;

	void evaluationPeriod( float seconds );
	float evaluationPeriod() const;

	void initialLoadMipSkip( uint32 mipSkip );
	uint32 initialLoadMipSkip() const;

	/// Queries
	uint32 streamingTextureCount() const;
	uint32 streamingTextureUsageCount() const;
	uint32 pendingStreamingRequests() const;
	
	// Memory usage queries
	size_t memoryUsageGPUCache() const;
	size_t memoryUsageMainCache() const;
	size_t numTexturesInCache() const;
	size_t memoryUsageStreaming() const;
	size_t memoryUsageStreamingDisk() const;
	size_t memoryUsageManagedTextures() const;
	size_t memoryUsageAllTextures() const;
	size_t memoryUsageActiveTextures() const;
	
	BaseTexture * getDebugTexture( DebugTextures::TextureSet debugSet,
		BaseTexture * pTex );

	void forceTextureSkipMips( StreamingTexture * pTex, uint32 skipMips = 0 );

	// Texture queries:
	uint32 getTextureWidth( const StreamingTexture * pTex ) const;
	uint32 getTextureHeight( const StreamingTexture * pTex ) const;
	D3DFORMAT getTextureFormat( const StreamingTexture * pTex ) const;
	size_t getTextureMemoryUsed( const StreamingTexture * pTex ) const;
	uint32 getTextureMaxMipLevel( const StreamingTexture * pTex ) const;
	void updateTextureMipSkip( StreamingTexture * pTex , uint32 mipSkip );

private:

	typedef D3DFORMAT TextureFormat;

	class StreamingTextureLoadTask;
	struct StreamingTextureEntry
	{
		StreamingTextureEntry();

		// Texture object
		StreamingTexture * pTexture;
		
		// Current state
		uint32 currentMip;
		uint32 targetMip;
		uint32 requestedMip;
		bool streamingActive;
		bool decayingBiasActive;
		uint32 sacrificedDetail;

		// Static constraints
		uint32 width;
		uint32 height;
		uint32 minAvailableMip;
		uint32 minAvailableStreamingMip;
		uint32 maxAvailableMip;
		uint32 maxAvailableStreamingMip;
		TextureFormat format;
		uint32 streamingPriority;

		// Dynamic constraints
		uint32 idealAvailableMip;
		uint32 idealMip;
		uint32 forcedMip;

		// Prioritisation signals
		uint32 pixelsAssignedOnScreen;
		float minAngleToFrustum;
	};

	typedef StreamingTextureEntry StreamingTextureRequest;

	class StreamingTextureLoadTask : public BaseStreamingTask
	{
	public:
		StreamingTextureLoadTask( TextureStreamingManager * pManager,
			StreamingTexture * pTexture, size_t bandwidthEstimate );
		~StreamingTextureLoadTask();

		const StreamingTextureRequest & requestEntry() const;
		DX::Texture * targetTexture() const;

	protected:
		enum State
		{
			INITIALIZE,
			LOCK,
			STREAM,
			UNLOCK,
			COMPLETE
		};

		virtual bool doStreaming( QueueType currentQueue );
		virtual QueueType doQueue();
		virtual void doComplete();
		virtual size_t doEstimateBandwidth();

		void initialize();
		bool createTargetTexture();
		uint32 copySharedMips();
		bool lockSurfaces();
		void unlockSurfaces();
		uint32 loadMipsFromDisk();

		uint32 numMipsToLoad() const;

		State state_;
		SmartPointer< StreamingTexture > pTexture_;
		TextureStreamingManager * pManager_;
		StreamingTextureRequest requestEntry_;
		ComObjectWrap< DX::Texture > pTargetTexture_;
		TextureLockWrapper textureLock_;
		BW::vector< D3DLOCKED_RECT > lockedRects_;
	};

	class StreamingTextureEvaluationTask: public BackgroundTask
	{
	public:
		StreamingTextureEvaluationTask( TextureStreamingManager * pManager );

		bool queue();

		virtual void doBackgroundTask( TaskManager & mgr );
		void manager( TextureStreamingManager * pManager );

	protected:
		TextureStreamingManager * pManager_;
		mutable SimpleMutex unregisterManagerLock_;
		mutable SimpleMutex queuedLock_;
		bool queued_;
	};

	// Internal
	bool registerTexture( StreamingTexture * pTex,
		const TextureDetailLevel * detail );

	// Helpers
	static void setIdealMip( StreamingTextureEntry & entry, uint32 mipLevel );
	static uint32 calculateMaxMips( uint32 width, uint32 height );
	const StreamingTextureEntry* fetchEntry( const StreamingTexture * pTex ) 
		const;
	StreamingTextureEntry* fetchEntry( StreamingTexture * pTex );
	bool getStreamingRequest( StreamingTexture * pTex, 
		StreamingTextureRequest & request );

	// Streaming helpers
	void requestTextureUpdate( StreamingTextureEntry & entry, 
		uint32 targetMipLevel, bool immediate = false);
	void reportStreamingCompleted( StreamingTexture * pTex, 
		DX::BaseTexture * pNewTexture,
		const StreamingTextureRequest & requestEntry );
	void handleStreamingCompleted( StreamingTextureEntry & entry,
		StreamingTexture * pTex, DeviceTexture * pNewTexture,
		const StreamingTextureRequest & requestEntry );

	// Performance measurement
	void assignDebugTexture( StreamingTextureEntry & entry );
	void updatePerformanceMetrics();
	float calculateIdealSetErrorMetric();
	float calculateTargetSetErrorMetric();
	void calculateTotalTextureMemoryUsage( size_t * pDataInMemory, 
		size_t * pDataOnDisk ) const;
	size_t calculateTextureMemoryUsage( const StreamingTextureEntry & entry ) 
		const;
	static size_t calculateTextureMemoryUsage( uint32 width, uint32 height, 
		uint32 skipMips, uint32 mipCount, 
		TextureFormat format );

	// Algorithm helpers
	void evaluateStreaming();
	void updateViewpoints( BW::vector< ViewpointData > & activePoints );
	void calculateIdealMips();
	void calculateTargetMips();
	void generateStreamingRequests();

	void executeOverSubscriptionPolicy( size_t proposedMemoryUsage );
	void executeUnderSubscriptionPolicy( size_t proposedMemoryUsage );

	void onSpaceCreated( SpaceManager * pManager, ClientSpace * pSpace );
	void onSpaceDestroyed( SpaceManager * pManager, ClientSpace * pSpace );

	// Comparision helpers
	float sacrificePriority( const StreamingTextureEntry & entry );
	float loadPriority( const StreamingTextureEntry & entry );
	bool canSacrificeDetail( const StreamingTextureEntry & entry );

	TextureManager * pTextureManager_;
	std::auto_ptr< StreamingManager > pStreamingManager_;
	SmartPointer< StreamingTextureEvaluationTask > pEvaluationTask_;

	typedef PackedObjectPool< StreamingTextureEntry, StreamingTextureHandle > 
		StreamingTextureEntryVector;
	StreamingTextureEntryVector streamingTextures_;
	
	typedef BW::vector< SmartPointer< Viewpoint > > ViewpointList;
	ViewpointList viewpoints_;

	typedef BW::set<Scene*> SceneSet;
	typedef BW::set<TextureStreamingSceneView*> SceneViewSet;
	SceneSet scenes_;
	SceneViewSet sceneViews_;

	mutable SimpleMutex sceneLock_;
	mutable SimpleMutex textureLock_;
	mutable SimpleMutex viewpointLock_;
	DebugTextures::TextureSet debugMode_;
	DebugTextures debugTextures_;
	uint32 outstandingStreamingRequests_;
	double nextEvaluationTime_;

	// Global constraints
	uint32 minMipLevel_;
	uint32 maxMipLevel_;
	uint32 maxMipsPerRequest_;
	uint32 memoryBudget_;
	float evaluationPeriod_;
	uint32 streamingMode_;
	uint32 prioritisationMode_;
	uint32 initialLoadMipSkip_;

	// Watchers
	float idealSetError_;
	float targetSetError_;
	size_t textureDataOnDisk_;
	size_t textureDataInMemory_;
	size_t textureDataPercentInMemory_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_STREAMING_MANAGER_HPP
