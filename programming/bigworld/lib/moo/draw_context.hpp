#ifndef DRAW_CONTEXT_HPP
#define DRAW_CONTEXT_HPP

#include "resmgr\resource_modification_listener.hpp"
#include "moo\device_callback.hpp"
#include "cstdmf\static_array.hpp"
#include "cstdmf\grow_only_pod_allocator.hpp"

namespace BW
{
class Matrix;
class Vector4;
class BoundingBox;

namespace Moo
{

class ManagedEffect;
class VertexBuffer;
class VertexDeclaration;
class IndexBuffer;
class StreamContainer;
class ComplexEffectMaterial;
class EffectProperty;
class Vertices;
class Primitive;

//-- Current rendering pass type.
enum ERenderingPassType
{
	RENDERING_PASS_COLOR = 0,
	RENDERING_PASS_REFLECTION,
	RENDERING_PASS_SHADOWS,
	RENDERING_PASS_DEPTH,
	RENDERING_PASS_COUNT
};
/**
*  This class is a base class for all user controlled global state overrides.
*  Game code is expected to create an object of this class for each global state override block
*  and push/pop it when doing rendering via DrawContext's render queues
*  global state includes: semantics, render states, direct shader constant
*  manipulation and everything else which may affect delayed rendering.
*  Derived classes have to support apply, undo and change effect modes.
*  Change effect mode support is only required if constant block is
*  directly setting up shader constants
*  Child classes need to implement isResourceModified only if they store
*  and use shader parameter handles to support runtime shader reloading.
*/
class GlobalStateBlock : public ResourceModificationListener
{
public:
	/**
	*  Three apply modes:
	*  Both APPLY_MODE and UNDO_MODE mean constant block has been changed,
	*  the only difference is
	*  for the APPLY_MODE 'new' block need to apply its global state modification
	*  while for the UNDO_MODE 'old' block need to revert its global state modification
	*  CHANGE_EFFECT mode means current constant block is still active, but
	*  pixel/vertex shader has changed and constant block needs to re-apply
	*  all shader specific setup.
	*/
	enum ApplyMode
	{
		APPLY_MODE,
		UNDO_MODE,
		CHANGE_EFFECT_MODE
	};

	virtual	void	apply( ManagedEffect* effect, ApplyMode mode ) = 0;

	virtual void	onResourceModified( const BW::StringRef& basePath,
						const BW::StringRef& resourceID, Action modType ) {}
};


struct RenderOp;
struct WrapperOp;
struct DistanceSortedOp;
class GlobalStateRecorder;

/**
 *  This class represents a draw context which should be passed to every single
 *  draw operation performed by the engine.
 *  It provides an interface for a scene wide delayed rendering for opaque, transparent and shimmering geometry.
 *  Delayed sorted rendering means we record render operations instead of
 *  submitting them directly to the GPU and flush everything later.
 *  It supports immediate mode for rendering objects which rely
 *  on global state we cannot capture yet.
 *  Immediate mode is usually interleaved with the normal delayed mode.
 *  Example of using immediate mode is rendering everything with attached fashions which do not support delayed rendering.
 *  Supports skinned and non skinned geometry.
 *  When doing flush it sorts opaque render ops by material/shader/vb/ib to speed up rendering
 *  by avoiding API overhead doing redundant material switches
 *
 *  Game code may override flush function and setup/tear down global state in it
 *  while calling parent's flush in-between.
 *  If global state has to be set per sets of render calls it can be done via GlobalStateBlocks and
 *  GlobalStateRecorder pushing custom user defined block before calling draw and popping it afterwards
 *  OverrideBlock allows do to override material on all render ops passed to the draw context.
 */
class DrawContext
{
public:
	enum ChannelMask
	{
		OPAQUE_CHANNEL_MASK			= 0x00000001,
		TRANSPARENT_CHANNEL_MASK	= 0x00000002,
		SHIMMER_CHANNEL_MASK		= 0x00000004,
		NUM_CHANNEL_MASKS				= 3,
		ALL_CHANNELS_MASK = ((1 << NUM_CHANNEL_MASKS) - 1)
	};
	class UserDrawItem
	{
	public:
		virtual ~UserDrawItem() {}
		virtual void draw() = 0;
		virtual void fini() {} // some user draw items might be doing self-destruct
	};

	class OverrideBlock
	{
	public:
		OverrideBlock( const BW::StringRef& shaderPrefix,
			bool isDoubleSided = false );

		void	process( ComplexEffectMaterial*& material,
						bool isSkinned ) const;

		bool	isValid() const;
	private:
		// rigid and skinned override effects
		SmartPointer<ComplexEffectMaterial> materials_[2];	
	};
	/**
	 *  Instance data which is necessary to be able to issue a draw call to GPU
	 *  We can separate skinned and non-skinned render ops by sorting render ops by palette size
	 *  However sorting by managed effect and technique should be enough to separate those
	 *  Make this structure 16bytes aligned if we need to use 16bytes alignment for Matrix or Vector4
	 *  This structure is a header followed by payload
	 *  Payload depends on palette size and either a single matrix representing world transform
	 *  or an array of Vector4 representing a skin matrix palette
	 *  Payload may be extended to hold any sort of additional information if required
	 */
	struct InstanceData
	{
		// each skinned matrix is represented by 3 Vector4s
		static const uint32 NUM_VECTOR4_PER_SKIN_MATRIX = 3;

		uint32			paletteSize_;	// if 0 use a single world transform matrix
		inline Matrix*		matrix()	{ MF_ASSERT_DEV(paletteSize_ == 0 ); return (Matrix*)(this + 1);	}
		inline Vector4*		palette()	{ MF_ASSERT_DEV(paletteSize_ != 0 ); return (Vector4*)(this + 1);	}
	};

public:
	DrawContext( ERenderingPassType renderingPassType );
	virtual ~DrawContext();

	void			pushGlobalStateBlock( GlobalStateBlock* userConstants );
	void			popGlobalStateBlock( GlobalStateBlock* userConstants );
	// change immediate mode on all render queues
	void			pushImmediateMode();
	void			popImmediateMode();
	// set active material override
	// does not support more than one override block at time
	void			pushOverrideBlock( OverrideBlock* overrideBlock );
	void			popOverrideBlock( OverrideBlock* overrideBlock );
	// begin/end all active render queues
	virtual void	begin( uint32 channelMask );
	virtual void	end( uint32 channelMask );
	virtual void	flush( uint32 channelMask, bool isClearing = true );
	uint32			collectChannelMask() const { return collectChannelMask_; }

	void			attachWatchers( const char* name, bool autoResetStatistics = true );
	// explicitly reset collected statistic
	void			resetStatistics();

	void			drawRenderOp( ComplexEffectMaterial* material,
		Vertices* vertices, Primitive* primitives, uint32 groupIndex,
		InstanceData* instanceData, const BoundingBox& worldBB );

	void			drawUserItem( UserDrawItem* userItem,
		ChannelMask channelType, float distance = 0.0f );

	ERenderingPassType	renderingPassType() const	{ return renderingPassType_; }

	InstanceData*	allocInstanceData( uint32 numPalletteEntries );

private:
	class RenderOpProcessor;
	DrawContext( const DrawContext& );		// disallow copy
	void operator=( const DrawContext& );	// disallow assign

	void	flushOpaqueChannel();
	void	flushTransparentChannel( bool isClearing );
	void	flushShimmerChannel( bool isClearing );
	bool	hwInstancingAvailable() const;

	// opaque render ops, sorted by shader / material / vb / ib etc
	DynamicArray<RenderOp>			opaqueOps_;
	// distance sorted render ops
	DynamicArray<DistanceSortedOp>	transparentOps_;
	// shimmer ops, not sorted by distance, flushed after the transparent channel
	DynamicArray<WrapperOp>			shimmerOps_;

	GlobalStateRecorder*	globalStateRecorder_;
	uint32					immediateModeCounter_;
	OverrideBlock*			activeOverride_;
	ERenderingPassType		renderingPassType_;
	uint32					collectChannelMask_;
	RenderOpProcessor*		ropProcessor_;
	// general purpose custom allocator to capture world matrices, matrix palettes,
	// non opaque render ops which require sorting by distance and other channel draw items
	// gets cleared on flush
	GrowOnlyPodAllocator	instanceDataAllocator_;
	const bool				hwInstancingAvailable_;

	struct DebugRenderingData
	{
		size_t		numOpaqueMaterialSwitches_;
		size_t		numOpaqueShaderSwitches_;
		size_t		numCollectedOpaqueOps_;
		size_t		numCollectedTransparentOps_;
		size_t		numCollectedShimmerOps_;
		bool		isSortingOpaqueChannel_;
		bool		isSortingTransparentChannel_;
		bool		isRenderingOpaqueChannel_;
		bool		isRenderingTransparentChannel_;
		bool		isRenderingShimmerChannel_;
	};

	// debug only
	DebugRenderingData		debugData_;
	bool					autoResetStatistics_;
#if ENABLE_WATCHERS
	BW::string				watcherPath_;
#endif // ENABLE_WATCHERS
};

} // namespace Moo

} // namespace BW

#endif // DRAW_CONTEXT_HPP
