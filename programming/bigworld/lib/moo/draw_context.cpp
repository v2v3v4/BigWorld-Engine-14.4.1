#include "pch.hpp"
#include "draw_context.hpp"
#include "moo\vertex_buffer.hpp"
#include "moo\complex_effect_material.hpp"
#include "moo\vertex_streams.hpp"
#include "moo\vertices.hpp"
#include "moo\primitive.hpp"
#include "moo\visual.hpp"
#include "moo\effect_visual_context.hpp"
#include "moo\dynamic_index_buffer.hpp"

namespace BW
{

namespace Moo
{

/**
*
*  This class records runtime global state overrides
*/
class GlobalStateRecorder
{
public:
	static const uint32 NULL_BUFFER_INDEX = 0xFFFFFFFF;

	GlobalStateRecorder()
	{
		capturedBlocks_.reserve( INITIAL_BUFFER_SIZE );
		this->reset();
	}


	uint32			captureChain()
	{
		if (currentChainSize_ == 0)
		{
			// handle the most common case when there is no recorded constant chain
			return NULL_BUFFER_INDEX;
		}
		// handle the case if this is the first capture call on the existing constant chain
		if (!didCapture_)
		{
			capturedBlocks_.push_back( NULL );
			didCapture_ = true;
		}
		// need to subtract 1 to handle chain terminator
		return (uint32)capturedBlocks_.size() - currentChainSize_ - 1;
	}

	void			applyChain( uint32 startIndex, GlobalStateBlock::ApplyMode mode,
								ManagedEffect* effect )
	{
		if (startIndex == NULL_BUFFER_INDEX)
		{
			return;
		}
		uint32 index = startIndex;
		GlobalStateBlock* block = capturedBlocks_[index];
		MF_ASSERT( block );

		while (block)
		{
			block->apply( effect, mode);
			index++;
			MF_ASSERT( index < capturedBlocks_.size() );
			block = capturedBlocks_[index];
		}
	}

	void			reset()
	{
		currentChainSize_ = 0;
		didCapture_ = false;
		capturedBlocks_.clear();
	}

	void			pushBlock( GlobalStateBlock* userConstant )
	{
		// no reason to record the same constant block twice in a row
		// if this is happening it usually indicates a logic problem in the codebase
		MF_ASSERT( currentChainSize_ == 0 || capturedBlocks_.back() != userConstant );
		if (didCapture_)
		{
			this->handleCaptureState();
		}
		capturedBlocks_.push_back( userConstant );
		currentChainSize_++;
	}

	void			popBlock( GlobalStateBlock* userConstant )
	{
		MF_ASSERT( currentChainSize_ > 0 );
		// decrease current chain size before making a copy of an existing chain
		// this way we do not copy an element which will be discarded straight away
		currentChainSize_--;
		if (didCapture_)
		{
			this->handleCaptureState();
		}
		else
		{
			// we only need to pop back an element if capture state was not on
			// otherwise it will be handled in handle capture state function
			capturedBlocks_.pop_back();
		}
	}
private:
	static const uint32 INITIAL_BUFFER_SIZE = 256;

	void			handleCaptureState()
	{
		// make a copy of an existing chain
		// need to subtract 1 to handle chain terminator
		uint32 iStart = (uint32)capturedBlocks_.size() - currentChainSize_ - 1;
		// however we do not copy terminator
		for (uint32 i = 0 ; i < currentChainSize_; i++)
		{
			capturedBlocks_.push_back( capturedBlocks_[iStart + i] );
		}
		didCapture_ = false;
	}	// on frame captured user constant blocks
	// this array is grow only and gets cleared on reset function call
	// external systems can keep indices to it
	// identical indices mean identical constant chains
	BW::vector<GlobalStateBlock*>	capturedBlocks_;
	uint32							currentChainSize_;
	bool							didCapture_;
};

// TODO: change material properties to something less insane
// detect identical material property setups
typedef BW::map< D3DXHANDLE, SmartPointer<EffectProperty> > MaterialProperties;

/**
 *	Render op represents a collection of data required to issue a draw call to the GPU
 *  All fields are pointers or POD types which can be used during the sorting phase
 *  The most important data to sort by are effect and effect's technique to avoid expensive shader switches
 */
struct RenderOp
{
	static const uint32 PRIMTYPE_NUMBITS = 3;
	static const uint32 PRIMTYPE_MASK		= (1 << PRIMTYPE_NUMBITS) - 1;
	static const uint32 FLAG_HW_INSTANCING	= 1 << PRIMTYPE_NUMBITS;
	// Material setup is represented by the effect and technique handle pair
	// which is resolved to the vertex and pixel shader pair
	ManagedEffect*			effect_;
	ManagedEffect::Handle	technique_;
	// Per instance material properties which include float/int/bool constants and textures
	// There is a limited support of detecting identical materials.
	// Some ( but not all ) identical material setups will have an identical pointer
	MaterialProperties*		materialProperties_;
	// Global render state overrides or NULL_BUFFER_INDEX if there are none
	// Game code can set those anytime during delayed rendering mode
	// user constant block can override any aspect of render op
	// it's a constant block responsibility to correctly handle apply, undo and shader change operations
	uint32					globalStateBlockIndex_;
	// render op data includes vertex buffer, index buffer and the index of group of primitives
	// identical vertex buffers have identical pointers
	// identical index buffers are also have identical pointers
	const VertexDeclaration*	vertexDeclaration_;
	const VertexBuffer*		vertexBuffer_;
	const IndexBuffer*		indexBuffer_;
	// extra vertex streams, could be NULL if there is only 1 default stream
	const StreamContainer*	vertexStreams_; 
	// default vertex stream information
	uint32					defaultVertexBufferOffset_;
	uint32					defaultVertexStride_;
	// information about primitive group needed to submit render operation
	PrimitiveGroup			primitiveGroupInfo_;
	// Instance data for this render op.
	// Contains world matrix or world matrix palette.
	DrawContext::InstanceData*	instanceData_;
	// flags ( primitive type and instanced flag )
	uint32					flags_;
};

struct WrapperOp
{
	enum Type
	{
		RENDEROP_NORMAL,
		RENDEROP_INTERNAL_SORT,
		USER_DRAW_ITEM
	};
	Type		type_;
	void*		payload_;
};

struct DistanceSortedOp : WrapperOp
{
	float		cameraDistance_;
	bool operator<( const DistanceSortedOp& rhs )
	{
		return cameraDistance_ > rhs.cameraDistance_;
	}
};

struct RenderOpInternalSort : RenderOp
{
public:
	const IndicesHolder*	pInputIndices_;
	const Vector3*			inputVertexPositions_;
};

/**
 *  Utility structures and functions for sorting and submitting data to GPU
 */
namespace 
{

// modifies input data
void	sortTrianglesByCameraDistance( RenderOp& renderOp,
										const IndicesHolder& inputIndices,
										const Vector3* inputVertexPositions )
{
	/**
	 * This structure is used to store the distance to a particular triangle in a
	 * Primitive::PrimGroup structure.
	 */
	struct TriangleDistance
	{
		TriangleDistance( float d1, float d2, float d3, int index )
		{
			dist_ = max( d1, max( d2, d3 ));
			index_ = index;
		}
		bool operator < (const TriangleDistance& d )
		{
			return this->dist_ < d.dist_;
		}

		float dist_;
		int index_;
	};

	// skinned internal sorted is not supported yet
	MF_ASSERT( renderOp.instanceData_->paletteSize_ == 0 );

	const PrimitiveGroup& pg = renderOp.primitiveGroupInfo_;

	static BW::vector<float> vertexDistances;

	// Resize the vertex distances to fit all our vertices
	vertexDistances.reserve( pg.nVertices_ );
	vertexDistances.clear();

	static BW::vector<TriangleDistance> triangleDistances;

	Matrix worldViewProj;
	worldViewProj.multiply( *renderOp.instanceData_->matrix(), Moo::rc().viewProjection() );

	Vector3 vec( worldViewProj.row(0).w, worldViewProj.row(1).w, 
		worldViewProj.row(2).w );
	float d = worldViewProj.row(3).w;

	for (uint32 i = 0; i < (uint32)pg.nVertices_; i++)
	{
		vertexDistances.push_back( vec.dotProduct(inputVertexPositions[i]) + d );
	}

	// Get the depth of the vertices in the vertex snapshot
	triangleDistances.reserve( pg.nPrimitives_ );
	triangleDistances.clear();
	uint32 index = pg.startIndex_;
	uint32 end = pg.startIndex_ + (pg.nPrimitives_ * 3);

	// Store the distance of the triangles in this primitive group
	uint32 currentTriangleIndex = 0;
	while (index != end)
	{
		float dist1 = vertexDistances[inputIndices[index] - pg.startVertex_];
		++index;
		float dist2 = vertexDistances[inputIndices[index] - pg.startVertex_];
		++index;
		float dist3 = vertexDistances[inputIndices[index] - pg.startVertex_];
		++index;
		triangleDistances.push_back( TriangleDistance( dist1, dist2, dist3, currentTriangleIndex ) );
		currentTriangleIndex += 3;
	}

	// No point continuing if there are no triangles to render
	if (triangleDistances.size() == 0)
	{
		// a_radchenko: why do we have vertex buffers without triangles ? 
		MF_ASSERT( 0 );
	}

	// Sort triangle distances back to front (using reverse iterator as +z is into the screen)
	std::sort( triangleDistances.rbegin(), triangleDistances.rend() );

	// Set the vertices and store the vertex offset
	static const uint32 MAX_16_BIT_INDEX = 0xffff; 
	
	uint32 maxIndex = pg.startVertex_ + pg.nVertices_;

	const D3DFORMAT format = (maxIndex <= MAX_16_BIT_INDEX) ? D3DFMT_INDEX16 : D3DFMT_INDEX32; 

	MF_ASSERT( maxIndex < Moo::rc().maxVertexIndex() );

	// Lock the indices. 
	DynamicIndexBufferBase& dib = rc().dynamicIndexBufferInterface().get( format ); 
	Moo::IndicesReference ind = 
		dib.lock2( static_cast<uint32>(triangleDistances.size() * 3 ));

	MF_ASSERT( ind.valid() );
	// Iterate over our sorted triangles and copy the indices into the new index 
	// buffer for rendering back to front. Also fix up the indices so that we do not 
	// have to give a negative BaseVertexIndex 
	VectorNoDestructor<TriangleDistance>::iterator sit = triangleDistances.begin(); 
	VectorNoDestructor<TriangleDistance>::iterator send = triangleDistances.end(); 

	int offset = 0; 

	while (sit != send) 
	{ 
		uint32 triangleIndex = (sit++)->index_; 
		triangleIndex += pg.startIndex_; 
		ind.set( offset++, inputIndices[triangleIndex++] ); 
		ind.set( offset++, inputIndices[triangleIndex++] ); 
		ind.set( offset++, inputIndices[triangleIndex++] ); 
	} 

	// Unlock the indices and set them on the device 
	dib.unlock(); 
	
	renderOp.indexBuffer_ = &dib.indexBuffer();
	renderOp.flags_ = (uint32)(D3DPT_TRIANGLELIST);
	renderOp.primitiveGroupInfo_.startIndex_ = dib.lockIndex();
}

RenderOp* fillWrapperOp( WrapperOp& wrop,  GrowOnlyPodAllocator& allocator,
		ChannelType channelType, Vertices* vertices, Primitive* primitives,
		const bool isSkinned)
{
	MF_ASSERT( channelType != OPAQUE_CHANNEL );
	MF_ASSERT( channelType != NOT_SUPPORTED_CHANNEL );

	RenderOp* renderOp;
	if (channelType == TRANSPARENT_CHANNEL || channelType == SHIMMER_CHANNEL || isSkinned)
	{
		wrop.type_ = WrapperOp::RENDEROP_NORMAL;
		renderOp  = (RenderOp*)allocator.allocate( sizeof (RenderOp) );
	}
	else
	{
		wrop.type_ = WrapperOp::RENDEROP_INTERNAL_SORT;
		RenderOpInternalSort* sortedItem = 
			(RenderOpInternalSort*)allocator.allocate(
			sizeof (RenderOpInternalSort) );
		renderOp = sortedItem;
		sortedItem->pInputIndices_ = &primitives->indices();
		MF_ASSERT( sortedItem->pInputIndices_ );
		sortedItem->inputVertexPositions_ = vertices->vertexPositions().data();
	}
	wrop.payload_ = renderOp;
	return renderOp;
}

void	fillRenderOp( RenderOp& rop,
					 ManagedEffect* effect,
					 ManagedEffect::Handle hTechnique,
					 MaterialProperties* materialProperties,
					 Vertices* vertices,
					 Primitive* primitives,
					 uint32 groupIndex,
					 uint32 globalStateBlockIndex,
					 DrawContext::InstanceData* instanceData,
					 bool isInstanced )
{
	rop.effect_ = effect;
	rop.technique_ = hTechnique;
	rop.materialProperties_ = materialProperties;
	vertices->pullInternals( isInstanced, rop.vertexDeclaration_,
		rop.vertexBuffer_, rop.defaultVertexBufferOffset_,
		rop.defaultVertexStride_, rop.vertexStreams_);

	rop.indexBuffer_ = &primitives->indexBuffer();
	rop.primitiveGroupInfo_ = primitives->primitiveGroup( groupIndex );
	rop.flags_ = (uint32)primitives->primType();

	MF_ASSERT( rop.flags_ & ((1 << (RenderOp::PRIMTYPE_NUMBITS + 1))- 1));
	if (isInstanced)
	{
		rop.flags_ |= RenderOp::FLAG_HW_INSTANCING;
	}

	MF_ASSERT( ((rop.flags_ & RenderOp::FLAG_HW_INSTANCING) && isInstanced) ||
		((rop.flags_ & RenderOp::FLAG_HW_INSTANCING) == 0 ));

	rop.instanceData_ = instanceData;
	rop.globalStateBlockIndex_ = globalStateBlockIndex;
}

inline bool RenderOpCompare( const RenderOp& rop0, const RenderOp& rop1 )
{
	if (rop0.effect_ == rop1.effect_)
	{
		if (rop0.technique_ == rop1.technique_)
		{
			if (rop0.materialProperties_ == rop1.materialProperties_)
			{
				if (rop0.globalStateBlockIndex_ == rop1.globalStateBlockIndex_)
				{
					if (rop0.vertexDeclaration_ == rop1.vertexDeclaration_)
					{
						if (rop0.vertexBuffer_ == rop1.vertexBuffer_)
						{
							if (rop0.indexBuffer_ == rop1.indexBuffer_)
							{
								return rop0.primitiveGroupInfo_.startIndex_ < rop1.primitiveGroupInfo_.startIndex_;
							}
							return rop0.indexBuffer_ < rop1.indexBuffer_;
						}
						return rop0.vertexBuffer_ < rop1.vertexBuffer_;
					}
					return rop0.vertexDeclaration_ < rop1.vertexDeclaration_;
				}
				return rop0.globalStateBlockIndex_ < rop1.globalStateBlockIndex_;
			}
			return rop0.materialProperties_ < rop1.materialProperties_;
		}
		return rop0.technique_ < rop1.technique_ ;
	}
	return rop0.effect_ < rop1.effect_;
}

} //  anonymous namespace


class DrawContext::RenderOpProcessor : public DeviceCallback
{
private:
	struct InstancedRenderOp
	{
		const RenderOp*	reference_;
		uint32	numInstances_;
	};
	typedef DynamicEmbeddedArray<InstancedRenderOp, 1024 > InstancedRopVector;

	struct ActiveState
	{
		ManagedEffect*				effect_;
		ManagedEffect::Handle		technique_;
		MaterialProperties*			materialProperties_;
		const VertexBuffer*			vertexBuffer_;
		const VertexDeclaration*	vertexDeclaration_;
		const IndexBuffer*			indexBuffer_;
		uint32						globalStateBlockIndex_;
		uint32						curActivePass_;
		uint32						numActivePasses_;

	};

	static const uint32	MAX_NUM_INSTANCED_MATRICES	=	4096;
	static const uint32 NON_INSTANCED_ROP = 0xFFFFFFFF;

	GlobalStateRecorder&	globalStateRecorder_;
	VertexBuffer			instancedVB_;
	EffectVisualContext&	effectVisualContext_;
	InstancedRopVector		instancedRenderOps_;
	const RenderOp*			instancingChainRef_;
	Matrix*					instanceMatrixArray_;
	Matrix*					startInstanceMatrix_;
	Matrix*					nextInstanceMatrix_;
	ActiveState				active_;

public:
	RenderOpProcessor( GlobalStateRecorder& globalStateRecorder):
		globalStateRecorder_( globalStateRecorder ),
		effectVisualContext_( Moo::rc().effectVisualContext() )
	{
		nullifyActiveState();
		instancingChainRef_ = NULL;
		instanceMatrixArray_ = NULL;
		startInstanceMatrix_ = NULL;
		nextInstanceMatrix_ = NULL;
	}

	void start()
	{
		MF_ASSERT( instancedRenderOps_.size() == 0 );
		effectVisualContext_.initConstants();
		createUnmanagedObjects();
		nullifyActiveState();
	}

	void stop()
	{
		closeActiveEffect();
		flushInstancedRenderOps();
		effectVisualContext_.worldMatrix( NULL );
	}

	void processRenderOp( const RenderOp* rop )
	{
		if (rop->flags_ & RenderOp::FLAG_HW_INSTANCING)
		{
			MF_ASSERT( instancedVB_.valid() );

			// record instanced render operation
			bool isInstancedBufferOutOfSpace = 
				nextInstanceMatrix_ - instanceMatrixArray_ == MAX_NUM_INSTANCED_MATRICES;

			if (instancingChainRef_ && notIdenticalRenderOps( rop, instancingChainRef_ ))
			{
				InstancedRenderOp& irop = instancedRenderOps_.push_back();
				irop.reference_ = instancingChainRef_;
				irop.numInstances_ = (uint32)(nextInstanceMatrix_ - startInstanceMatrix_);
				instancingChainRef_ = NULL;	
			}
			if (isInstancedBufferOutOfSpace)
			{
				closeActiveEffect();
				// and flush instanced render ops
				flushInstancedRenderOps();
			}

			if (!startInstanceMatrix_)
			{
				// open / reopen instancing buffer to write a new data
				openInstancingBuffer();
			}
			// start a new chain if it didn't exist or was closed
			if (!instancingChainRef_)
			{
				instancingChainRef_ = rop;
				startInstanceMatrix_ = nextInstanceMatrix_;
			}
			// record instance data
			MF_ASSERT( rop->instanceData_->paletteSize_ == 0 );
			*nextInstanceMatrix_++ = *rop->instanceData_->matrix();
		}
		else
		{
			processRop( rop, 1, NON_INSTANCED_ROP );
		}
	}

	void	createUnmanagedObjects()
	{
		BW_GUARD;
		if (!instancedVB_.valid() && rc().isHWInstancingAvailable())
		{
			bool success = SUCCEEDED(instancedVB_.create(
				InstancingStream::ELEMENT_SIZE * MAX_NUM_INSTANCED_MATRICES,
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
				"vertex buffer/visual batcher VB"
				));
			MF_ASSERT(success);
		}
	}

	void	deleteUnmanagedObjects()
	{
		BW_GUARD;
		if (instancedVB_.valid())
		{
			instancedVB_.release();
		}
	}

	// process one render op right now
	void	immediateRender( const RenderOp& rop )
	{
		applyInstanceData( rop.instanceData_ );
		rop.effect_->currentTechnique( rop.technique_, true );
		// set vertex declaration and vertex buffer
		rc().setVertexDeclaration( rop.vertexDeclaration_->declaration() );
		rop.vertexBuffer_->set( 0, rop.defaultVertexBufferOffset_, rop.defaultVertexStride_ );
		// reset additional vertex streams if they were active
		if (rop.vertexStreams_)
		{
			rop.vertexStreams_->set();
		}
		else
		{
			VertexBuffer::reset( UV2Stream::STREAM_NUMBER );
			VertexBuffer::reset( ColourStream::STREAM_NUMBER );
		}
		// set index buffer
		rop.indexBuffer_->set();

		if (rop.effect_->begin( false ))
		{
			for (uint32 pass = 0; pass < rop.effect_->numPasses(); ++pass)
			{
				rop.effect_->beginPass( pass );
				// apply material properties
				setEffectMaterialProperties( rop.effect_, rop.materialProperties_ );
				// apply global state which we recorded before
				globalStateRecorder_.applyChain( globalStateRecorder_.captureChain(),
					GlobalStateBlock::APPLY_MODE, rop.effect_ );
				// need to set auto constants after we applied user constant chain because constant chain may override
				// semantics which require set auto constant call to actually write data to the shader constants
				flushRop( &rop, 1, NON_INSTANCED_ROP );
				// finish default 0 pass
				rop.effect_->endPass();
			}
			rop.effect_->end();
		}
		effectVisualContext_.worldMatrix( NULL );
	}

	void flushSingleWrappedOp( WrapperOp& wrapOp, bool isClearing )
	{
		switch (wrapOp.type_)
		{
		case DistanceSortedOp::RENDEROP_NORMAL:
			{
				RenderOp* renderOp = (RenderOp*)wrapOp.payload_;
				processRenderOp( renderOp );
			}; break;
		case DistanceSortedOp::RENDEROP_INTERNAL_SORT:
			{
				RenderOpInternalSort* payload = (RenderOpInternalSort*)wrapOp.payload_;
				RenderOp renderOp = *payload;
				sortTrianglesByCameraDistance( renderOp,
					*payload->pInputIndices_, payload->inputVertexPositions_ );
				processRenderOp( &renderOp );
			}; break;
		case DistanceSortedOp::USER_DRAW_ITEM:
			{
				stop();
				DrawContext::UserDrawItem* payload = (DrawContext::UserDrawItem*)wrapOp.payload_;
				payload->draw();
				if (isClearing)
				{
					payload->fini();
				}
				start();
			}; break;
		}
	}

private:
	void nullifyActiveState()
	{
		active_.effect_ = NULL;
		active_.technique_ = NULL;
		active_.materialProperties_ = NULL;
		active_.vertexBuffer_ = NULL;
		active_.vertexDeclaration_ = NULL;
		active_.indexBuffer_ = NULL;
		active_.globalStateBlockIndex_ = GlobalStateRecorder::NULL_BUFFER_INDEX;
		active_.curActivePass_ = 0;
		active_.numActivePasses_ = 0;
	}

	// set material properties to given effect
	inline void setEffectMaterialProperties( ManagedEffect* effect,
		MaterialProperties* properties )
	{
		if (properties)
		{
			ID3DXEffect*	pEffect = effect->pEffect();
			MaterialProperties::iterator	it	= properties->begin();
			MaterialProperties::iterator	end	= properties->end();

			while (it != end)
			{
				it->second->apply( pEffect, it->first );
				it++;
			}
		}
	}

	// resolve instance data and apply it to the visual context
	inline void applyInstanceData( DrawContext::InstanceData* instanceData )
	{
		if (instanceData->paletteSize_ > 0)
		{
			effectVisualContext_.worldMatrixPalette( instanceData->palette(),
				instanceData->paletteSize_ );
		}
		else
		{
			effectVisualContext_.worldMatrix( instanceData->matrix() );
		}
	}

	void flushRop( const RenderOp* rop, uint32 numInstances, uint32 offset )
	{
		if (offset == NON_INSTANCED_ROP)
		{
			// TODO, change all instance data to directly populate shader constants
			// set world matrix / matrix palette for use as a vertex shader constant
			// there could be multiple vertex shader constants which are derived from world transform matrix
			applyInstanceData( rop->instanceData_ );
		}
		else
		{
			instancedVB_.set( InstancingStream::STREAM_NUMBER,
				offset * InstancingStream::ELEMENT_SIZE,
				InstancingStream::ELEMENT_SIZE );

			rc().device()->SetStreamSourceFreq( 0, 
				D3DSTREAMSOURCE_INDEXEDDATA | numInstances );
		}
		// need to set auto constants after we applied constant chain because
		// constant chain may contain semantics which require set
		// auto constant call to actually write data to the shader constants
		rop->effect_->setAutoConstants();
		rop->effect_->commitChanges();
		// dip
		D3DPRIMITIVETYPE primType = (D3DPRIMITIVETYPE)(rop->flags_ & RenderOp::PRIMTYPE_MASK);
		if (rop->primitiveGroupInfo_.nVertices_ && rop->primitiveGroupInfo_.nPrimitives_)
		{
			Moo::rc().drawIndexedInstancedPrimitive( primType,
				0,
				rop->primitiveGroupInfo_.startVertex_,
				rop->primitiveGroupInfo_.nVertices_,
				rop->primitiveGroupInfo_.startIndex_,
				rop->primitiveGroupInfo_.nPrimitives_, numInstances );
		}
	}

	void setMaterial(const RenderOp* rop, bool isEffectChanged )
	{
		// apply new material
		if (active_.materialProperties_ != rop->materialProperties_)
		{
			active_.materialProperties_ = rop->materialProperties_;
			setEffectMaterialProperties( active_.effect_, active_.materialProperties_ );
		}
		// we need to re-apply captured global state if captured state changes
		// or active shader changes
		// process global constant block change
		if (rop->globalStateBlockIndex_ != GlobalStateRecorder::NULL_BUFFER_INDEX &&
			active_.globalStateBlockIndex_ != rop->globalStateBlockIndex_)
		{
			// undo previous user block
			globalStateRecorder_.applyChain( active_.globalStateBlockIndex_,
				GlobalStateBlock::UNDO_MODE, active_.effect_ );
			// set new active constant block
			active_.globalStateBlockIndex_ = rop->globalStateBlockIndex_;
			// apply new user block
			globalStateRecorder_.applyChain( active_.globalStateBlockIndex_,
				GlobalStateBlock::APPLY_MODE, active_.effect_ );
		}
		else if (isEffectChanged)
		{
			// if user constants are the same we only need to handle shader change
			// so user block can do direct shader constant setup
			globalStateRecorder_.applyChain( active_.globalStateBlockIndex_,
				GlobalStateBlock::CHANGE_EFFECT_MODE, active_.effect_ );
		}
	}

	void setPrimitiveBuffers( const RenderOp* rop )
	{
		// set vertex decl
		if (active_.vertexDeclaration_ != rop->vertexDeclaration_)
		{
			active_.vertexDeclaration_ = rop->vertexDeclaration_;
			Moo::rc().setVertexDeclaration( rop->vertexDeclaration_->declaration() );
		}
		if (active_.vertexBuffer_ != rop->vertexBuffer_)
		{
			active_.vertexBuffer_ = rop->vertexBuffer_;
			// default vertex buffer is always index 0
			active_.vertexBuffer_->set( 0, rop->defaultVertexBufferOffset_,
				rop->defaultVertexStride_ );
		}
		// reset additional vertex streams if they were active
		if (rop->vertexStreams_)
		{
			rop->vertexStreams_->set();
		}
		else
		{
			VertexBuffer::reset( UV2Stream::STREAM_NUMBER );
			VertexBuffer::reset( ColourStream::STREAM_NUMBER );
		}

		// set indices
		if (active_.indexBuffer_ != rop->indexBuffer_)
		{
			active_.indexBuffer_ = rop->indexBuffer_;
			rop->indexBuffer_->set();
		}
	}

	void closeActiveEffect( bool isFullClose = true )
	{
		if (active_.effect_)
		{
			active_.effect_->endPass();
			active_.effect_->end();
		}

		if (isFullClose)
		{
			globalStateRecorder_.applyChain( active_.globalStateBlockIndex_,
				GlobalStateBlock::UNDO_MODE,  active_.effect_ );
			nullifyActiveState();
		}
	}

	void setEffect( const RenderOp* rop )
	{
		// apply new shader and close previously opened shader pass if required
		active_.effect_ = rop->effect_;
		active_.technique_ = rop->technique_;
		active_.effect_->currentTechnique( active_.technique_, true );
		active_.effect_->begin( false );
		active_.numActivePasses_ = active_.effect_->numPasses();
		active_.curActivePass_ = 0;
		if (active_.curActivePass_ < active_.numActivePasses_)
		{
			active_.effect_->beginPass( active_.curActivePass_++ );
		}
	}

	void processRop( const RenderOp* rop, uint32 numInstances, uint32 offset )
	{
		bool isEffectChanged = active_.effect_ != rop->effect_ ||
			active_.technique_ != rop->technique_;

		if (isEffectChanged)
		{
			closeActiveEffect( false );
			setEffect( rop );
		}
		// non instanced technique, flush a single rop pass 0
		setMaterial( rop, isEffectChanged );
		setPrimitiveBuffers( rop );
		flushRop( rop, numInstances, offset );
		// process multi-pass if we have more than 1 pass
		while (active_.curActivePass_ < active_.numActivePasses_)
		{
			// multi pass, proceed to the next pass
			active_.effect_->endPass();
			active_.effect_->beginPass( active_.curActivePass_++ );
			// re-render all render ops from the beginning of the pass marker
			setMaterial( rop, false );
			flushRop( rop, numInstances, offset );
		}
	}

	inline bool notIdenticalRenderOps( const RenderOp* rop0, const RenderOp* rop1 )
	{
		return rop0->effect_ != rop1->effect_ ||
			rop0->technique_ != rop1->technique_ ||
			rop0->materialProperties_ != rop1->materialProperties_ ||
			rop0->vertexDeclaration_ != rop1->vertexDeclaration_ ||
			rop0->vertexBuffer_ != rop1->vertexBuffer_ ||
			rop0->indexBuffer_ != rop1->indexBuffer_ ||
			rop0->primitiveGroupInfo_.startIndex_ != rop1->primitiveGroupInfo_.startIndex_ ||
			rop0->globalStateBlockIndex_ != rop1->globalStateBlockIndex_;
	}

	void openInstancingBuffer()
	{
		if (instancedVB_.valid())
		{
			instancedVB_.lock( 0, MAX_NUM_INSTANCED_MATRICES * InstancingStream::ELEMENT_SIZE,
				(void**)&instanceMatrixArray_, D3DLOCK_DISCARD );
			startInstanceMatrix_ = instanceMatrixArray_;
			nextInstanceMatrix_ = instanceMatrixArray_;
			instancingChainRef_ = NULL;
		}
	}

	void flushInstancedRenderOps()
	{
		if (!instanceMatrixArray_)
		{
			return;
		}
		// close the last open instancing chain
		if ( instancingChainRef_ )
		{
			InstancedRenderOp& irop = instancedRenderOps_.push_back();
			irop.reference_ = instancingChainRef_;
			irop.numInstances_ = (uint32)(nextInstanceMatrix_ - startInstanceMatrix_);
			instancingChainRef_ = NULL;
		}
		if (startInstanceMatrix_)
		{
			// close instancing vb
			instancedVB_.unlock();
			instanceMatrixArray_ = NULL;
			startInstanceMatrix_ = NULL;
			nextInstanceMatrix_ = NULL;
		}
		// flush instancing chains
		MF_ASSERT ( instancedRenderOps_.size() );
		nullifyActiveState();

		rc().device()->SetStreamSourceFreq( InstancingStream::STREAM_NUMBER,
			(D3DSTREAMSOURCE_INSTANCEDATA | 1) );

		const InstancedRenderOp* iropEnd = instancedRenderOps_.data() + instancedRenderOps_.size();

		uint32 offset = 0;
		for (const InstancedRenderOp* irop = instancedRenderOps_.data();
			irop != iropEnd; ++irop)
		{
			processRop( irop->reference_, irop->numInstances_, offset );
			offset += irop->numInstances_;
		}
		// close render pass for the last active instanced effect
		closeActiveEffect();

		// clear instanced renderops array
		instancedRenderOps_.clear();

		rc().device()->SetStreamSourceFreq( 0, 1 );
		rc().device()->SetStreamSourceFreq( InstancingStream::STREAM_NUMBER, 1 );
		VertexBuffer::reset( InstancingStream::STREAM_NUMBER );
	}
}; // RenderOpProcessor

// debug only
#if ENABLE_WATCHERS
static bool	s_HWInstancingEnabledOverride = true;
static bool	s_enabledStatisticsCollection = false;
#else
static const bool	s_HWInstancingEnabledOverride = true;
static const bool	s_enabledStatisticsCollection = false;
#endif // ENABLE_WATCHERS

static const uint32 INITIAL_OPAQUE_RENDEROP_BUFFER_SIZE = 1024;
static const size_t INSTANCE_DATA_PAGE_SIZE = 4096;

/**
 *  Draw context implementation
 */
DrawContext::OverrideBlock::OverrideBlock( const BW::StringRef& shaderPrefix,
										  bool isDoubleSided )
{
	materials_[0] = new ComplexEffectMaterial();
	materials_[0]->initFromEffect(shaderPrefix + ".fx", "", isDoubleSided ? 1 : -1 );
	materials_[1] = new ComplexEffectMaterial();
	materials_[1]->initFromEffect( shaderPrefix + "_skinned.fx", "", isDoubleSided ? 1 : -1 );
}

void DrawContext::OverrideBlock::process( ComplexEffectMaterial*& material,
										 bool isSkinned ) const
{
	material = isSkinned ? materials_[1].get(): materials_[0].get();
	MF_ASSERT( material->baseMaterial()->pEffect() );
}


bool DrawContext::OverrideBlock::isValid() const
{
	return materials_[0].hasObject() || materials_[1].hasObject();
}

DrawContext::DrawContext( ERenderingPassType renderingPassType ):
	immediateModeCounter_( 0 ),
	renderingPassType_( renderingPassType ),
	hwInstancingAvailable_( rc().isHWInstancingAvailable() ),
	instanceDataAllocator_( INSTANCE_DATA_PAGE_SIZE ),
	activeOverride_( NULL ),
	collectChannelMask_( 0 )

{
	globalStateRecorder_ = new GlobalStateRecorder;
	autoResetStatistics_ = true;
	// debug only
	debugData_.isSortingOpaqueChannel_ = true;
	debugData_.isSortingTransparentChannel_ = true;
	debugData_.isRenderingOpaqueChannel_ = true;
	debugData_.isRenderingTransparentChannel_ = true;
	debugData_.isRenderingShimmerChannel_ = true;

	this->resetStatistics();

	opaqueOps_.reserve( INITIAL_OPAQUE_RENDEROP_BUFFER_SIZE );
	ropProcessor_ = new RenderOpProcessor( *globalStateRecorder_ );
}

DrawContext::~DrawContext()
{
#if ENABLE_WATCHERS
	if (watcherPath_.length() > 0)
	{
		if (Watcher::hasRootWatcher())
		{
			MF_ASSERT( Watcher::hasRootWatcher() &&
				Watcher::rootWatcher().getChild( watcherPath_.c_str() ) != NULL );

			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Opaque Channel Sorting").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Transparent Channel Sorting").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Opaque Channel Rendering").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Transparent Channel Rendering").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Shimmer Channel Rendering").c_str() );

			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Opaque Material Switches").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Opaque Shader Switches").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Opaque Ops").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Transparent Ops").c_str() );
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Shimmer Ops").c_str() );

			Watcher::rootWatcher().removeChild( watcherPath_.c_str() );
		}

		if (Watcher::hasRootWatcher())
		{
			Watcher::rootWatcher().removeChild( (watcherPath_ + "/Enabled").c_str() );
			Watcher::rootWatcher().removeChild( watcherPath_.c_str() );
		}
	}
#endif // ENABLE_WATCHERS

	bw_safe_delete( ropProcessor_ );
	bw_safe_delete( globalStateRecorder_ );

	MF_ASSERT( immediateModeCounter_ == 0 );
	MF_ASSERT( opaqueOps_.size() == 0 );
	MF_ASSERT( transparentOps_.size() == 0 );
	MF_ASSERT( shimmerOps_.size() == 0) ;
}

void	DrawContext::begin( uint32 channelMask )
{
	MF_ASSERT( (collectChannelMask_ & channelMask) == 0 );
	collectChannelMask_ |= channelMask;
	if (autoResetStatistics_)
	{
		this->resetStatistics();
	}
}

void	DrawContext::end( uint32 channelMask )
{
	MF_ASSERT( (collectChannelMask_ & channelMask) == channelMask );
	collectChannelMask_ &= ~channelMask;
}

void	DrawContext::flush( uint32 channelMask, bool isClearing )
{
	MF_ASSERT( (collectChannelMask_ & channelMask ) == 0 );

	BW_GUARD_PROFILER(DrawContext_flush);
	PROFILER_SCOPED_DYNAMIC_STRING( watcherPath_.c_str() );

	if (channelMask & OPAQUE_CHANNEL_MASK)
	{
		this->flushOpaqueChannel();
		if (isClearing)
		{
			opaqueOps_.clear();
		}
	}
	if (channelMask & TRANSPARENT_CHANNEL_MASK)
	{
		this->flushTransparentChannel( isClearing );
		if (isClearing)
		{
			transparentOps_.clear();
		}
	}

	if (channelMask & SHIMMER_CHANNEL_MASK)
	{
		this->flushShimmerChannel( isClearing );
		if (isClearing)
		{
			shimmerOps_.clear();
		}
	}

	if (opaqueOps_.size() == 0 && transparentOps_.size() == 0 && shimmerOps_.size() == 0)
	{
		instanceDataAllocator_.releaseAll();
		globalStateRecorder_->reset();
	}
}

void	DrawContext::flushShimmerChannel( bool isClearing )
{
	if (shimmerOps_.size() == 0 || !debugData_.isRenderingShimmerChannel_)
	{
		return;
	}
	BW_GUARD_PROFILER( DrawContext_flushShimmerChannel );
	if (s_enabledStatisticsCollection)
	{
		debugData_.numCollectedShimmerOps_ = shimmerOps_.size();
	}
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );

	ropProcessor_->start();
	// flush distance sorted ops
	for (DynamicArray<WrapperOp>::iterator it = shimmerOps_.begin();
		it != shimmerOps_.end(); ++it)
	{
		ropProcessor_->flushSingleWrappedOp( *it, isClearing );
	}
	ropProcessor_->stop();

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE );

}

void	DrawContext::flushTransparentChannel( bool isClearing )
{
	if (transparentOps_.size() == 0 || !debugData_.isRenderingTransparentChannel_)
	{
		return;
	}
	BW_GUARD_PROFILER( DrawContext_flushTransparentChannel );
	if (debugData_.isSortingTransparentChannel_)
	{
		std::sort( transparentOps_.begin(), transparentOps_.end() );
	}
	if (s_enabledStatisticsCollection)
	{
		debugData_.numCollectedTransparentOps_ = transparentOps_.size();
	}

	ropProcessor_->start();
	// flush distance sorted ops
	for (DynamicArray<DistanceSortedOp>::iterator it = transparentOps_.begin();
		it != transparentOps_.end(); ++it)
	{
		ropProcessor_->flushSingleWrappedOp( *it, isClearing );
	}
	ropProcessor_->stop();
}

void	DrawContext::flushOpaqueChannel()
{
	if (opaqueOps_.size() == 0 || !debugData_.isRenderingOpaqueChannel_)
	{
		return;
	}
	// sort opaque render ops first
	if (debugData_.isSortingOpaqueChannel_)
	{
		BW_GUARD_PROFILER( DrawContext_sortOpaqueChannel );
		std::sort( opaqueOps_.begin(), opaqueOps_.end(), RenderOpCompare );
	}
	const RenderOp* renderOps = opaqueOps_.data();
	size_t numRenderOps = opaqueOps_.size();
	// flush opaque render ops to GPU
	{
		BW_GUARD_PROFILER( DrawContext_flushOpaqueChannel );
		ropProcessor_->start();
		const RenderOp* ropEnd = renderOps + numRenderOps;
		for (const RenderOp* rop = renderOps; rop != ropEnd; ++rop)
		{
			ropProcessor_->processRenderOp( rop );
		}
		ropProcessor_->stop();
	}

	// collect debug statistics to display in runtime
	if (s_enabledStatisticsCollection)
	{
		BW_GUARD_PROFILER( DrawContext_debugStats );

		const ManagedEffect* curEffect = NULL;
		const MaterialProperties* curMaterialProperties = NULL;

		for (size_t i  = 0; i < numRenderOps; ++i)
		{
			const RenderOp& rop = renderOps[i];
			if (curEffect != rop.effect_)
			{
				curEffect = rop.effect_;
				debugData_.numOpaqueShaderSwitches_++;
			}
			if (curMaterialProperties != rop.materialProperties_)
			{
				curMaterialProperties = rop.materialProperties_;
				debugData_.numOpaqueMaterialSwitches_++;
			}
		}
		debugData_.numCollectedOpaqueOps_ += numRenderOps;
	}
}

void	DrawContext::attachWatchers( const char* name,
									bool autoResetStatistics )
{
#if ENABLE_WATCHERS
	autoResetStatistics_ =  autoResetStatistics; 
	MF_ASSERT( strlen(name) );
	watcherPath_ = BW::string("DrawContext/") + name;

	// check that multiple draw contexts aren't using the same name
	MF_ASSERT( Watcher::hasRootWatcher() &&
		Watcher::rootWatcher().getChild( watcherPath_.c_str() ) == NULL );

	static bool firstTime = true;
	if (firstTime)
	{
		firstTime = false;

		MF_WATCH( "DrawContext/HWInstancingEnabled", s_HWInstancingEnabledOverride,
			Watcher::WT_READ_WRITE, "Force enabling/disabling hardware instancing" );

		MF_WATCH( "DrawContext/EnabledStatisticsCollection", s_enabledStatisticsCollection,
			Watcher::WT_READ_WRITE, "Force enabling/disabling statistics collections" );
	}

	// check that multiple draw contexts aren't using the same name
	MF_ASSERT( Watcher::hasRootWatcher() &&
		Watcher::rootWatcher().getChild( (watcherPath_ + "/Opaque Sorting Mode").c_str() ) == NULL );

	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Opaque Channel Sorting").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Transparent Channel Sorting").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Opaque Channel Rendering").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Transparent Channel Rendering").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Shimmer Channel Rendering").c_str() );

	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Opaque Material Switches").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Opaque Shader Switches").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Opaque Ops").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Transparent Ops").c_str() );
	Watcher::rootWatcher().removeChild( (watcherPath_ + "/Num Collected Shimmer Ops").c_str() );

	MF_WATCH( (watcherPath_ + "/Opaque Channel Sorting").c_str(),
		*(bool*)&debugData_.isSortingOpaqueChannel_, Watcher::WT_READ_WRITE, "" );

	MF_WATCH( (watcherPath_ + "/Transparent Channel Sorting").c_str(),
		*(bool*)&debugData_.isSortingTransparentChannel_, Watcher::WT_READ_WRITE, "" );

	MF_WATCH( (watcherPath_ + "/Opaque Channel Rendering").c_str(),
		*(bool*)&debugData_.isRenderingOpaqueChannel_, Watcher::WT_READ_WRITE, "" );

	MF_WATCH( (watcherPath_ + "/Transparent Channel Rendering" ).c_str(),
		*(bool*)&debugData_.isRenderingTransparentChannel_,	Watcher::WT_READ_WRITE, "" );

	MF_WATCH( (watcherPath_ + "/Shimmer Channel Rendering" ).c_str(),
		*(bool*)&debugData_.isRenderingShimmerChannel_,	Watcher::WT_READ_WRITE, "" );

	MF_WATCH( (watcherPath_ + "/Num Opaque Material Switches" ).c_str(),
		*(uint*)&debugData_.numOpaqueMaterialSwitches_,	Watcher::WT_READ_ONLY, "" );

	MF_WATCH( (watcherPath_ + "/Num Opaque Shader Switches" ).c_str(),
		*(uint*)&debugData_.numOpaqueShaderSwitches_,	Watcher::WT_READ_ONLY, "" );

	MF_WATCH( (watcherPath_ + "/Num Collected Opaque Ops" ).c_str(),
		*(uint*)&debugData_.numCollectedOpaqueOps_,	Watcher::WT_READ_ONLY, "" );

	MF_WATCH( (watcherPath_ + "/Num Collected Transparent Ops" ).c_str(),
		*(uint*)&debugData_.numCollectedTransparentOps_,	Watcher::WT_READ_ONLY, "" );

	MF_WATCH( (watcherPath_ + "/Num Collected Shimmer Ops" ).c_str(),
		*(uint*)&debugData_.numCollectedShimmerOps_,	Watcher::WT_READ_ONLY, "" );
#endif // ENABLE_WATCHERS
}

void	DrawContext::resetStatistics()
{
#if ENABLE_WATCHERS
	debugData_.numOpaqueMaterialSwitches_ = 0;
	debugData_.numOpaqueShaderSwitches_ = 0;
	debugData_.numCollectedOpaqueOps_ = 0;
	debugData_.numCollectedTransparentOps_ = 0;
	debugData_.numCollectedShimmerOps_ = 0;
#endif
}

// change immediate mode on all render queues
void	DrawContext::pushImmediateMode()
{
	immediateModeCounter_++;
}

void	DrawContext::popImmediateMode()
{
	MF_ASSERT(immediateModeCounter_ > 0);
	immediateModeCounter_--;
}

void	DrawContext::pushOverrideBlock( OverrideBlock* overrideBlock )
{
	MF_ASSERT( activeOverride_ == NULL );
	activeOverride_ = overrideBlock;
}
void	DrawContext::popOverrideBlock( OverrideBlock* overrideBlock )
{
	MF_ASSERT( activeOverride_ == overrideBlock );
	activeOverride_ = NULL;
}


void	DrawContext::pushGlobalStateBlock( GlobalStateBlock* userConstants )
{
	globalStateRecorder_->pushBlock( userConstants );
}

void	DrawContext::popGlobalStateBlock( GlobalStateBlock* userConstants )
{
	globalStateRecorder_->popBlock( userConstants );
}

void	DrawContext::drawRenderOp( ComplexEffectMaterial* material,
								  Vertices* vertices,
								  Primitive* primitives,
								  uint32 primitiveGroupIndex,
								  InstanceData* instanceData,
								  const BoundingBox& worldBB )
{
	const bool canUseHWInstancing = this->hwInstancingAvailable() &&
									immediateModeCounter_ == 0;
	const bool isSkinned = instanceData->paletteSize_ > 0;
	// apply material override if needed
	if (activeOverride_)
	{
		activeOverride_->process( material, isSkinned );
	}

	ChannelType channelType = material->channelType();
	MF_ASSERT( channelType != NOT_SUPPORTED_CHANNEL );

	// pull data required for rendering from complex effect material
	// hw instancing for transparent models is not supported
	bool isInstanced = canUseHWInstancing && channelType == OPAQUE_CHANNEL && !isSkinned;
	ManagedEffect* effect = NULL;
	ManagedEffect::Handle technique = NULL;
	MaterialProperties* materialProperties = NULL;
	material->pass( renderingPassType_, effect,
		technique, materialProperties, isInstanced );
	
	// it's possible for technique to be NULL if effect does not support given rendering pass type
	// if this is the case, we'll just skip it
	// like some effects might not support shadow pass if they don't want to draw shadows
	if (effect == NULL || technique == NULL)
	{
		return;
	}
	// material properties must always exist
	MF_ASSERT( materialProperties );
	// skinned data does not support hw instancing yet
	MF_ASSERT( (isSkinned && !isInstanced) || !isSkinned);
	RenderOp* renderOp = NULL;
	if (channelType == OPAQUE_CHANNEL &&
		(collectChannelMask_ & OPAQUE_CHANNEL_MASK))
	{
		renderOp = &opaqueOps_.push_back();
	}
	else if ((channelType == TRANSPARENT_CHANNEL ||
		channelType == TRANSPARENT_INTERNAL_SORT_CHANNEL)
		&& (collectChannelMask_ & TRANSPARENT_CHANNEL_MASK))
	{
		DistanceSortedOp& sortOp = transparentOps_.push_back();
		sortOp.cameraDistance_ =
			rc().view().applyPoint( worldBB.centre() + 
			primitives->origin( primitiveGroupIndex )).z;
		renderOp = fillWrapperOp( sortOp, instanceDataAllocator_,
			channelType, vertices, primitives, isSkinned );
		isInstanced = false;
	}
	else if ((channelType == SHIMMER_CHANNEL ||
		channelType == SHIMMER_INTERNAL_SORT_CHANNEL)
		&& (collectChannelMask_ & SHIMMER_CHANNEL_MASK))
	{
		WrapperOp& sortOp = shimmerOps_.push_back();
		renderOp = fillWrapperOp( sortOp, instanceDataAllocator_,
			channelType, vertices, primitives, isSkinned );
		isInstanced = false;
	}

	if (renderOp)
	{
		fillRenderOp( *renderOp, effect, technique, materialProperties,
			vertices, primitives, primitiveGroupIndex,
			globalStateRecorder_->captureChain(), instanceData, isInstanced );

		if (immediateModeCounter_ && channelType == OPAQUE_CHANNEL)
		{
			if (debugData_.isRenderingOpaqueChannel_)
			{
				ropProcessor_->immediateRender( opaqueOps_.back() );
			}
			opaqueOps_.pop_back();
		}
	}

}

DrawContext::InstanceData*	DrawContext::allocInstanceData( uint32 numPalletteEntries )
{
	InstanceData* data;
	if (numPalletteEntries)
	{
		data = (InstanceData*)instanceDataAllocator_.allocate(
			sizeof(InstanceData) + 
			sizeof(Vector4) * InstanceData::NUM_VECTOR4_PER_SKIN_MATRIX * numPalletteEntries);
	}
	else
	{
		data = (InstanceData*)instanceDataAllocator_.allocate(
			sizeof(InstanceData) + sizeof(Matrix));

	}
	data->paletteSize_ = numPalletteEntries;
	return data;
}

void	DrawContext::drawUserItem( UserDrawItem* userItem, ChannelMask channelMask,
								  float distance )
{
	MF_ASSERT( channelMask == TRANSPARENT_CHANNEL_MASK || channelMask == SHIMMER_CHANNEL_MASK );
	MF_ASSERT( userItem );
	if (channelMask == TRANSPARENT_CHANNEL_MASK && (collectChannelMask_ & TRANSPARENT_CHANNEL_MASK))
	{
		DistanceSortedOp& op = transparentOps_.push_back();
		op.cameraDistance_ = distance;
		op.type_ = WrapperOp::USER_DRAW_ITEM;
		op.payload_ = userItem;
	}
	else if (channelMask == SHIMMER_CHANNEL_MASK && (collectChannelMask_ & SHIMMER_CHANNEL_MASK))
	{
		WrapperOp& op = shimmerOps_.push_back();
		op.type_ = WrapperOp::USER_DRAW_ITEM;
		op.payload_ = userItem;
	}
	else
	{
		userItem->fini();
	}
}

bool	DrawContext::hwInstancingAvailable() const
{
	return hwInstancingAvailable_ && s_HWInstancingEnabledOverride;
}

} // namespace Moo

} // namespace BW
