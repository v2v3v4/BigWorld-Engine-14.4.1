#include "pch.hpp"

#include "cstdmf/grow_only_pod_allocator.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"
#include "moo/vertices.hpp"
#include "moo/primitive.hpp"
#include "moo/managed_effect.hpp"


using namespace BW;
using namespace Moo;

#if 0

class MockEffectVisualContext : public EffectVisualContext
{
public:
	virtual void	init() {}
};

class OpaqueRenderQueueSimpleFixture1
{
public:
	OpaqueRenderQueueSimpleFixture1():drawContext_( RENDERING_PASS_COLOR )
	{

	}
public:
	MockEffectVisualContext	effectVisualContext_;
	GlobalStateRecorder		stateRecorder_;
	DrawContext		drawContext_;

};

class MockUserConstantBlock;

class MockUserContstantBlockContext
{
public:
	class RecordedApply
	{
	public:
		RecordedApply(MockUserConstantBlock* constantBlock,
			ManagedEffect* effect, GlobalStateBlock::ApplyMode	mode):
			constantBlock_( constantBlock ), effect_( effect), mode_( mode ) {}

		MockUserConstantBlock* constantBlock_;
		ManagedEffect*	effect_;
		GlobalStateBlock::ApplyMode	mode_;
	};

	void	record( MockUserConstantBlock* mockUserConstant,
					ManagedEffect* effect,
					GlobalStateBlock::ApplyMode mode)
	{ 
		recordedApply_.push_back(RecordedApply(mockUserConstant, effect, mode));
	}

	void	clear() { recordedApply_.clear(); }

	BW::vector<RecordedApply>	recordedApply_;
};
class MockUserConstantBlock : public GlobalStateBlock
{
public:
	MockUserConstantBlock( MockUserContstantBlockContext& context ):context_( context ) {}

	virtual	void	apply( ManagedEffect* effect,
		GlobalStateBlock::ApplyMode mode )
	{
		context_.record( this, effect, mode );
	}
private:
	MockUserContstantBlockContext& context_;
};

TEST( GlobalStateRecorder_userBlocksImmediateMode_One )
{
	GlobalStateRecorder stateRecoreder;

	MockUserContstantBlockContext blockContext;
	MockUserConstantBlock block0( blockContext );
	// test applying one constant chain
	stateRecoreder.pushBlock( &block0 );
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );
	CHECK_EQUAL( 1, blockContext.recordedApply_.size() );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[0].constantBlock_ );
	CHECK(NULL == blockContext.recordedApply_[0].effect_ );
	CHECK_EQUAL(GlobalStateBlock::APPLY_MODE, blockContext.recordedApply_[0].mode_ );
	stateRecoreder.popBlock( &block0 );
	// test applying current constant chain where there are no recorded blocks
	blockContext.clear();
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );
	CHECK_EQUAL( 0, blockContext.recordedApply_.size() );
}

TEST( GlobalStateRecorder_userBlocksImmediateMode_Two )
{
	GlobalStateRecorder stateRecoreder;

	MockUserContstantBlockContext blockContext;
	MockUserConstantBlock block0( blockContext );
	MockUserConstantBlock block1( blockContext );

	// test applying two constant blocks, block0 followed by block1
	blockContext.clear();
	stateRecoreder.pushBlock( &block0 );
	stateRecoreder.pushBlock( &block1 );
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );

	CHECK_EQUAL( 2, blockContext.recordedApply_.size() );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[0].constantBlock_ );
	CHECK(NULL == blockContext.recordedApply_[0].effect_ );
	CHECK_EQUAL( GlobalStateBlock::APPLY_MODE, blockContext.recordedApply_[0].mode_ );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[1].constantBlock_ );
	CHECK(NULL == blockContext.recordedApply_[1].effect_ );
	CHECK_EQUAL( GlobalStateBlock::APPLY_MODE, blockContext.recordedApply_[1].mode_ );
	stateRecoreder.popBlock( &block1 );
	stateRecoreder.popBlock( &block0 );

	// test applying two constant blocks, block1 followed by block0
	blockContext.clear();
	stateRecoreder.pushBlock( &block1 );
	stateRecoreder.pushBlock( &block0 );
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );
	CHECK_EQUAL( 2, blockContext.recordedApply_.size() );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[0].constantBlock_ );
	CHECK(NULL == blockContext.recordedApply_[0].effect_ );
	CHECK_EQUAL( GlobalStateBlock::APPLY_MODE, blockContext.recordedApply_[0].mode_ );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[1].constantBlock_ );
	CHECK(NULL == blockContext.recordedApply_[1].effect_ );
	CHECK_EQUAL( GlobalStateBlock::APPLY_MODE, blockContext.recordedApply_[1].mode_ );
	stateRecoreder.popBlock( &block0 );
	stateRecoreder.popBlock( &block1 );
}

TEST( GlobalStateRecorder_userBlocksImmediateMode_Three )
{
	GlobalStateRecorder stateRecoreder;

	MockUserContstantBlockContext blockContext;
	MockUserConstantBlock block0( blockContext );
	MockUserConstantBlock block1( blockContext );

	// test applying three constant blocks, block1 followed by block0
	stateRecoreder.pushBlock( &block1 );
	stateRecoreder.pushBlock( &block0 );
	// test multiple applies where first apply uses two blocks
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );

	CHECK_EQUAL( 2, blockContext.recordedApply_.size() );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[0].constantBlock_ );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[1].constantBlock_ );
	// push the third block and do another apply
	// note that at this stage all three blocks must re-apply to the current global state
	stateRecoreder.pushBlock( &block1 );
	stateRecoreder.applyChain( stateRecoreder.captureChain(),
		GlobalStateBlock::APPLY_MODE, NULL );

	CHECK_EQUAL( 5, blockContext.recordedApply_.size() );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[0].constantBlock_ );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[1].constantBlock_ );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[2].constantBlock_ );
	CHECK_EQUAL( &block0, blockContext.recordedApply_[3].constantBlock_ );
	CHECK_EQUAL( &block1, blockContext.recordedApply_[4].constantBlock_ );
	stateRecoreder.popBlock( &block1 );
	stateRecoreder.popBlock( &block0 );
	stateRecoreder.popBlock( &block1 );

}

TEST_F( OpaqueRenderQueueSimpleFixture1, DrawContext_allocInstanceData )
{
	// confirm that memory layout is linear
	drawContext_.begin();

	// allocate matrix, then 4 matrix palette and then another matrix
	InstanceData* matrixInstance0 = drawContext_.allocInstanceData( 0 );
	InstanceData* paletteInstance0 = drawContext_.allocInstanceData( 4 );
	InstanceData* matrixInstance1 = drawContext_.allocInstanceData( 0 );
	CHECK( matrixInstance0);
	CHECK( matrixInstance1);
	CHECK( paletteInstance0);
	CHECK_EQUAL( 0, matrixInstance0->paletteSize_);
	CHECK_EQUAL( 0, matrixInstance1->paletteSize_);
	CHECK_EQUAL( 4, paletteInstance0->paletteSize_);

	uchar* expectedPaletteInstance = (uchar*)matrixInstance0 + sizeof(Matrix) + sizeof(InstanceData);
	CHECK_EQUAL( expectedPaletteInstance, (uchar*)paletteInstance0 );

	uchar* expectedMatrix0Payload = (uchar*)matrixInstance0 + sizeof(InstanceData);
	CHECK_EQUAL( expectedMatrix0Payload, (uchar*)matrixInstance0->matrix() );

	uchar* expectedPalette0Payload = (uchar*)paletteInstance0 + sizeof(InstanceData);
	CHECK_EQUAL( expectedPalette0Payload, (uchar*)paletteInstance0->palette() );

	uchar* expectedMatrix1Instance = (uchar*)matrixInstance0 + 
		sizeof(InstanceData) * 2 + sizeof(Matrix) +
		sizeof(Vector4) * InstanceData::NUM_VECTOR4_PER_SKIN_MATRIX * 4;
	CHECK_EQUAL( expectedMatrix1Instance, (uchar*)matrixInstance1 );

	uchar* expectedMatrix1Payload = (uchar*)matrixInstance0 +
		sizeof(InstanceData) * 3 + sizeof(Matrix) +
		sizeof(Vector4) * InstanceData::NUM_VECTOR4_PER_SKIN_MATRIX * 4;
	CHECK_EQUAL( expectedMatrix1Payload, (uchar*)matrixInstance1->matrix() );

	drawContext_.end();
	// reset and allocate palette and matrix
	drawContext_.flush();

	drawContext_.begin();

	InstanceData* paletteInstance1 = drawContext_.allocInstanceData( 11 );
	InstanceData* matrixInstance2 = drawContext_.allocInstanceData( 0 );
	CHECK( paletteInstance1);
	CHECK( matrixInstance2);
	CHECK_EQUAL( 0, matrixInstance2->paletteSize_);
	CHECK_EQUAL( 11, paletteInstance1->paletteSize_);

	// check that we actually reset pointers
	CHECK_EQUAL( matrixInstance0, paletteInstance1 );
	// verify memory layout
	uchar* expectedPalette1Payload = (uchar*)paletteInstance1 + sizeof(InstanceData);
	CHECK_EQUAL( expectedPalette1Payload, (uchar*)paletteInstance1->palette() );

	uchar* expectedMatrix2Instance = (uchar*)paletteInstance1 + 
		sizeof(InstanceData) + 
		sizeof(Vector4) * InstanceData::NUM_VECTOR4_PER_SKIN_MATRIX * 11;
	CHECK_EQUAL( expectedMatrix2Instance, (uchar*)matrixInstance2 );

	drawContext_.end();
	drawContext_.flush();
}

class MockManagedEffect : public ManagedEffect
{
public:
	class CapturedInstanceData
	{
	public:
		CapturedInstanceData(): paletteSize_( 0 ), palette_(NULL) {}
		~CapturedInstanceData()
		{
		}

		void		capture( MockEffectVisualContext& context )
		{
			paletteSize_ = context.worldMatrixPaletteSize();
			if (paletteSize_)
			{
				uint32 numVectors = paletteSize_ * EffectVisualContext::NUM_VECTOR4_PER_PALETTE_ENTRY;
				palette_.resize(numVectors);
				memcpy( palette_.data(), context.worldMatrixPalette(), sizeof(Vector4) * numVectors );
			}
			else
			{
				matrix_ = *context.worldMatrix();
			}
		}

		Matrix		matrix_;
		BW::vector<Vector4>	palette_;
		uint32		paletteSize_;
	};

	MockManagedEffect(MockEffectVisualContext& context ): 
		context_( context ),
		setAutoConstantsCalled_(0),
		beginCalled_(0),
		beginPassCalled_( 0 ),
		endPassCalled_( 0 ),
		endCalled_( 0 ),
		currentTechniqueSetCalled_ ( 0 ),
		commitChangesCalled_( 0 ),
		mockNumPasses_(1)
	{
	}

	virtual void setAutoConstants()
	{ 
		CapturedInstanceData data;
		data.capture( context_ );
		instanceDataUsed_.push_back( data );

		setAutoConstantsCalled_++;
	}
	virtual bool begin( bool setAutoProps ) { beginCalled_++; return true; }
	virtual bool beginPass( uint32 pass ) { beginPassCalled_++; return true; }
	virtual bool endPass() { endPassCalled_++; return true; }
	virtual bool end() { endCalled_++; return true; }
	virtual bool currentTechnique( Handle hTec, bool setExplicit )
	{ currentTechniqueSetCalled_.push_back( hTec); return true; }
	virtual bool commitChanges() { commitChangesCalled_++; return true; }
	// Render queue does not support more than 1 pass and asserts on it
	virtual uint32 numPasses() const
	{ 
		if (beginCalled_ == endCalled_)
		{
			return 0;
		}
		return mockNumPasses_;
	}

	uint32 setAutoConstantsCalled_;
	uint32 beginCalled_;
	uint32 beginPassCalled_;
	uint32 endPassCalled_;
	uint32 endCalled_;
	uint32 mockNumPasses_;
	BW::vector<Handle> currentTechniqueSetCalled_;
	uint32 commitChangesCalled_;
	BW::vector<CapturedInstanceData>	instanceDataUsed_;
	MockEffectVisualContext&	context_;
};

class MockEffectProperty: public EffectProperty
{
public:
	MockEffectProperty( const BW::string& name ): EffectProperty( name ) {}
	virtual bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{ 
		ApplyStruct s;
		s.pEffect_ = pEffect;
		s.hProperty_ = hProperty;
		applyCalled_.push_back(s);
		return true;
	}
	virtual void save( DataSectionPtr pDS ) {}
	virtual EffectProperty* clone() const { return NULL; }

	struct ApplyStruct
	{
		ID3DXEffect* pEffect_;
		D3DXHANDLE hProperty_;
	};
	BW::vector< ApplyStruct> applyCalled_;
};

class MockVertices : public Vertices
{
public:
	MockVertices( const BW::string& resourceID, int numNodes ): setVerticesCalled_(0), Vertices( resourceID, numNodes ) {}

	virtual HRESULT		setVertices( bool software, 
		bool instanced = false ) 
		{ setVerticesCalled_++; return S_OK; }

	virtual void	resetStream( const uint32 streamNumber ) {  }

	uint32 setVerticesCalled_;
};

class MockPrimitives : public Primitive
{
public:
	MockPrimitives( const BW::string& resourceID ):
		setPrimitivesCalled_(0),
		Primitive( resourceID ) {}

	virtual HRESULT		drawPrimitiveGroup( uint32 groupIndex ) { drawPrimitiveGroupRecorded_.push_back( groupIndex ); return S_OK; }
	virtual HRESULT		setPrimitives() { setPrimitivesCalled_++; return S_OK; }

	BW::vector<uint32>	drawPrimitiveGroupRecorded_;
	uint32	setPrimitivesCalled_;
};

class OpaqueRenderQueueSimpleFixture2
{
public:
	OpaqueRenderQueueSimpleFixture2():
		effectVisualContext(),
		vb("bob", 0),
		ib("bob"),
		effect(effectVisualContext),
		allocator( 1024 )
	{
		technique = (EffectTechnique)0xFF00FF00;

		effectProperty = new MockEffectProperty("bob");
		materialParamHandle = (EffectTechnique)0x00000004;
		material[materialParamHandle] = effectProperty;
	}
	~OpaqueRenderQueueSimpleFixture2()
	{

	}

	MockEffectVisualContext effectVisualContext;
	GlobalStateRecorder		globalStateRecorder;

	BW::vector<RenderOp> renderOps_;
	MockManagedEffect effect;
	EffectTechnique technique;
	MaterialProperties material;
	SmartPointer<MockEffectProperty>  effectProperty;
	EffectTechnique materialParamHandle ;

	MockVertices vb;
	MockPrimitives ib;
	GrowOnlyPodAllocator allocator;
};
TEST_F( OpaqueRenderQueueSimpleFixture2, RenderQueue_addAndFlushOneNonSkinnedRenderOp )
{
	uint32 groupIndex = 11;

	// prepare data for the render op and record it
	// allocate and populate instance data
	InstanceData* instanceData = (InstanceData*)allocator.allocate( sizeof(InstanceData) + sizeof(Matrix));
	instanceData->paletteSize_ = 0;
	// generate random world transform matrix
	Matrix&	worldTransform = *instanceData->matrix();
	worldTransform.setTranslate( 1.0f, 2.0f, 3.0f );
	worldTransform.preRotateY( 1.5f );
	worldTransform.preRotateX( -1.0f );

	RenderOp rop;
	rop.effect_ = &effect;
	rop.technique_ = technique;
	rop.materialProperties_ = &material;
	rop.vertices_ = &vb;
	rop.primitives_ = &ib;
	rop.primGroupIndex_ = groupIndex;
	rop.instanceData_ = instanceData;
	rop.globalStateBlockIndex_ = GlobalStateRecorder::NULL_BUFFER_INDEX;
	rop.flags_ = 0;

	RenderQueue::deepTestRender( &rop, 1, globalStateRecorder, effectVisualContext, NULL );
	

	CHECK_EQUAL( 1, effect.beginCalled_);
	CHECK_EQUAL( 1, effect.beginPassCalled_ );
	CHECK_EQUAL( 1, effect.commitChangesCalled_ );
	CHECK_EQUAL( 1, effect.endPassCalled_ );
	CHECK_EQUAL( 1, effect.endCalled_ );
	CHECK_EQUAL( 1, effect.setAutoConstantsCalled_ );
	CHECK_EQUAL( 1, effect.currentTechniqueSetCalled_.size() );
	CHECK_EQUAL( technique, effect.currentTechniqueSetCalled_[0] );

	CHECK_EQUAL( 1, vb.setVerticesCalled_ );
	CHECK_EQUAL( 1, ib.setPrimitivesCalled_ );
	CHECK_EQUAL( 1, ib.drawPrimitiveGroupRecorded_.size() );
	CHECK_EQUAL( groupIndex, ib.drawPrimitiveGroupRecorded_[0] );
	CHECK_EQUAL( 1, effectProperty->applyCalled_.size() );
	CHECK_EQUAL( materialParamHandle, effectProperty->applyCalled_[0].hProperty_ );

	CHECK_EQUAL( 1, effect.instanceDataUsed_.size() );
	CHECK_EQUAL( 0, effect.instanceDataUsed_[0].paletteSize_ );

	CHECK( almostEqual(worldTransform, effect.instanceDataUsed_[0].matrix_) );
}


TEST_F( OpaqueRenderQueueSimpleFixture2, RenderQueue_addAndFlushOneSkinnedRenderOp )
{
	uint32 groupIndex = 1;

	// prepare data for one skinnd render op and record it
	// allocate and populate instance data
	static const uint32 TEST_PALETTE_SIZE = 3;
	static const uint32 NUM_VECTOR4 = TEST_PALETTE_SIZE * EffectVisualContext::NUM_VECTOR4_PER_PALETTE_ENTRY;
	Vector4	palette[NUM_VECTOR4];
	for (uint32 i = 0; i < NUM_VECTOR4; ++i)
		for (uint32 j = 0; j < 4; j++)
			palette[i][j] = (i + 1.0f) / (j + 1.0f);

	InstanceData* instanceData = (InstanceData*)allocator.allocate( sizeof(InstanceData) +
		sizeof(Vector4) * NUM_VECTOR4);
	instanceData->paletteSize_ = 3;
	memcpy(instanceData->palette(), palette, sizeof(Vector4) * NUM_VECTOR4);

	RenderOp rop;
	rop.effect_ = &effect;
	rop.technique_ = technique;
	rop.materialProperties_ = &material;
	rop.vertices_ = &vb;
	rop.primitives_ = &ib;
	rop.primGroupIndex_ = groupIndex;
	rop.instanceData_ = instanceData;
	rop.globalStateBlockIndex_ = GlobalStateRecorder::NULL_BUFFER_INDEX;
	rop.flags_ = 0;

	RenderQueue::deepTestRender( &rop, 1, globalStateRecorder, effectVisualContext, NULL );

	CHECK_EQUAL( 1, effect.beginCalled_);
	CHECK_EQUAL( 1, effect.beginPassCalled_ );
	CHECK_EQUAL( 1, effect.commitChangesCalled_ );
	CHECK_EQUAL( 1, effect.endPassCalled_ );
	CHECK_EQUAL( 1, effect.endCalled_ );
	CHECK_EQUAL( 1, effect.setAutoConstantsCalled_ );
	CHECK_EQUAL( 1, effect.currentTechniqueSetCalled_.size() );
	CHECK_EQUAL( technique, effect.currentTechniqueSetCalled_[0] );

	CHECK_EQUAL( 1, vb.setVerticesCalled_ );
	CHECK_EQUAL( 1, ib.setPrimitivesCalled_ );
	CHECK_EQUAL( 1, ib.drawPrimitiveGroupRecorded_.size() );
	CHECK_EQUAL( groupIndex, ib.drawPrimitiveGroupRecorded_[0] );

	CHECK_EQUAL( 1, effect.instanceDataUsed_.size() );
	CHECK_EQUAL( 3, effect.instanceDataUsed_[0].paletteSize_ );

	for (uint32 i = 0; i < NUM_VECTOR4; ++i)
	{
		CHECK( almostEqual(palette[i],  effect.instanceDataUsed_[0].palette_[i], 0.001f));
	}
}

class OpaqueRenderQueueComplexFixture2
{
public:
	OpaqueRenderQueueComplexFixture2():
		effectVisualContext(),
		globalStateRecoreder(),
		ib0("bar"),
		allocator( 1024 )
	{
		for (uint32 i = 0; i < NUM_EFFECTS; i++)
		{
			effects[i] = new MockManagedEffect(effectVisualContext);
		}
		std::sort(effects, effects + NUM_EFFECTS );

		technique0 = (EffectTechnique)0x000000A0;
		technique1 = (EffectTechnique)0x00FFFF00;

		for (uint32 i = 0; i < NUM_MATERIALS; i++)
		{
			materials[i] = new MaterialProperties;
		}
		std::sort(materials, materials + NUM_MATERIALS );

		vbs[0] = new MockVertices("foo0", 0);
		vbs[1] = new MockVertices("foo1", 0);

		std::sort(vbs, vbs + NUM_VBS );
	}

	void	setRenderOp( RenderOp& rop, 
		ManagedEffect* effect,
		EffectTechnique technique,
		MaterialProperties*		materialProperties,
		Vertices*				vertices,
		Primitive*				primitives,
		uint32					primGroupIndex,
		const Matrix&			instanceMatrix,			
		uint32					userConstantBlocks )
	{
		rop.effect_ = effect;
		rop.technique_ = technique;
		rop.materialProperties_ = materialProperties;
		rop.vertices_ = vertices;
		rop.primitives_ = primitives;
		rop.primGroupIndex_ = primGroupIndex;
		InstanceData* instanceData = (InstanceData*)allocator.allocate( sizeof(InstanceData) + sizeof(Matrix) );
		instanceData->paletteSize_ = 0;
		*instanceData->matrix() = instanceMatrix;
		rop.instanceData_ = instanceData;
		rop.globalStateBlockIndex_ = userConstantBlocks;
		rop.flags_ = 0;
	}

	void	initMatrices()
	{
		for (uint32 i = 0;i < NUM_ROPS; ++i)
		{
			for (uint j = 0; j < 4; j++)
			{
				for (uint k = 0; k < 4; k++)
				{
					instanceMatrices[i][j][k] = (j + 1.0f) / (k + 1.0f) * i * 10.0f;
				}
			}
		}
	}

	void	postSetup()
	{
		expectedOrderIndices[0] = 2;
		expectedOrderIndices[1] = 6;
		expectedOrderIndices[2] = 0;
		expectedOrderIndices[3] = 4;
		expectedOrderIndices[4] = 1;
		expectedOrderIndices[5] = 3;
		expectedOrderIndices[6] = 7;
		expectedOrderIndices[7] = 5;
	}

	void setupWithMatrices()
	{

		initMatrices();

		setRenderOp(renderOps[0], effects[0], technique0, materials[1], vbs[1], &ib0, 0,
			instanceMatrices[0], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[1], effects[0], technique1, materials[0], vbs[1], &ib0, 0,
			instanceMatrices[1], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[2], effects[0], technique0, materials[0], vbs[1], &ib0, 0,
			instanceMatrices[2], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[3], effects[1], technique0, materials[1], vbs[1], &ib0, 0,
			instanceMatrices[3], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[4], effects[0], technique1, materials[0], vbs[0], &ib0, 0,
			instanceMatrices[4], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[5], effects[2], technique0, materials[0], vbs[1], &ib0, 0,
			instanceMatrices[5], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[6], effects[0], technique0, materials[1], vbs[0], &ib0, 0,
			instanceMatrices[6], GlobalStateRecorder::NULL_BUFFER_INDEX );

		setRenderOp(renderOps[7], effects[1], technique1, materials[0], vbs[0], &ib0, 0,
			instanceMatrices[7], GlobalStateRecorder::NULL_BUFFER_INDEX );

		postSetup();
	}

	~OpaqueRenderQueueComplexFixture2()
	{
		for (uint32 i = 0; i < NUM_EFFECTS; i++)
		{
			delete effects[i];
		}

		for (uint32 i = 0; i < NUM_MATERIALS; i++)
		{
			delete materials[i];
		}

		for (uint32 i = 0; i < NUM_VBS; i++)
		{
			delete vbs[i];
		}
	}

	static const uint32 NUM_ROPS = 8;
	static const uint32 NUM_EFFECTS = 3;
	static const uint32 NUM_MATERIALS = 2;
	static const uint32 NUM_VBS = 2;

	MockManagedEffect* effects[NUM_EFFECTS];
	MaterialProperties* materials[NUM_MATERIALS];
	EffectTechnique technique0;
	EffectTechnique technique1;
	MockVertices* vbs[NUM_VBS];
	MockEffectVisualContext effectVisualContext;
	GlobalStateRecorder globalStateRecoreder;
	GrowOnlyPodAllocator allocator;

	RenderOp		renderOps[NUM_ROPS];
	MockPrimitives ib0;

	Matrix	instanceMatrices[NUM_ROPS];
	uint32	expectedOrderIndices[NUM_ROPS];
};

TEST_F( OpaqueRenderQueueComplexFixture2, RenderQueue_deepSort )
{
	setupWithMatrices();

	BW::vector<RenderOp> expectedOrder;
	for (uint32 i = 0; i < NUM_ROPS; ++i)
	{
		expectedOrder.push_back( renderOps[expectedOrderIndices[i]] );
	}

	RenderQueue::sortRenderOpsDeep( renderOps, NUM_ROPS );

	// check sort results
	for (uint32 i = 0; i < NUM_ROPS; i++)
	{
		CHECK_EQUAL(0, memcmp(&expectedOrder[i], &renderOps[i],
			sizeof(RenderOp)));
	}
}

TEST_F( OpaqueRenderQueueComplexFixture2, RenderQueue_linearRender )
{
	setupWithMatrices();

	// call linear render on large number of objects
	RenderQueue::linearRender( renderOps, NUM_ROPS, globalStateRecoreder, effectVisualContext );

	CHECK_EQUAL(5, effects[0]->beginCalled_);
	CHECK_EQUAL(2, effects[1]->beginCalled_);
	CHECK_EQUAL(1, effects[2]->beginCalled_);

	CHECK_EQUAL(5, effects[0]->beginPassCalled_);
	CHECK_EQUAL(2, effects[1]->beginPassCalled_);
	CHECK_EQUAL(1, effects[2]->beginPassCalled_);

	CHECK_EQUAL(5, effects[0]->endCalled_);
	CHECK_EQUAL(2, effects[1]->endCalled_);
	CHECK_EQUAL(1, effects[2]->endCalled_);

	CHECK_EQUAL(5, effects[0]->endPassCalled_);
	CHECK_EQUAL(2, effects[1]->endPassCalled_);
	CHECK_EQUAL(1, effects[2]->endPassCalled_);

	CHECK_EQUAL(5, effects[0]->setAutoConstantsCalled_);
	CHECK_EQUAL(2, effects[1]->setAutoConstantsCalled_);
	CHECK_EQUAL(1, effects[2]->setAutoConstantsCalled_);

	CHECK_EQUAL(5, effects[0]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(2, effects[1]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(1, effects[2]->currentTechniqueSetCalled_.size());

	CHECK_EQUAL(5, effects[0]->commitChangesCalled_);
	CHECK_EQUAL(2, effects[1]->commitChangesCalled_);
	CHECK_EQUAL(1, effects[2]->commitChangesCalled_);

	CHECK_EQUAL(5, effects[0]->instanceDataUsed_.size());
	CHECK_EQUAL(2, effects[1]->instanceDataUsed_.size());
	CHECK_EQUAL(1, effects[2]->instanceDataUsed_.size());

	CHECK_EQUAL(3, vbs[0]->setVerticesCalled_);
	CHECK_EQUAL(5, vbs[1]->setVerticesCalled_);

	CHECK_EQUAL(NUM_ROPS, ib0.setPrimitivesCalled_);
}


TEST_F( OpaqueRenderQueueComplexFixture2, RenderQueue_deepSortAndDeepTestRender )
{
	// do not use RenderQueue::Impl light setup deduplication because it provides inconsisent memory addresses
	// this is ok for sorting, but for this test we want to control everything to be able to count number of operations
	setupWithMatrices();

	RenderQueue::sortRenderOpsDeep( renderOps, NUM_ROPS );
	RenderQueue::deepTestRender( renderOps, NUM_ROPS, globalStateRecoreder, effectVisualContext, NULL );

	CHECK_EQUAL(2, effects[0]->beginCalled_);
	CHECK_EQUAL(2, effects[1]->beginCalled_);
	CHECK_EQUAL(1, effects[2]->beginCalled_);

	CHECK_EQUAL(2, effects[0]->beginPassCalled_);
	CHECK_EQUAL(2, effects[1]->beginPassCalled_);
	CHECK_EQUAL(1, effects[2]->beginPassCalled_);

	CHECK_EQUAL(2, effects[0]->endCalled_);
	CHECK_EQUAL(2, effects[1]->endCalled_);
	CHECK_EQUAL(1, effects[2]->endCalled_);

	CHECK_EQUAL(2, effects[0]->endPassCalled_);
	CHECK_EQUAL(2, effects[1]->endPassCalled_);
	CHECK_EQUAL(1, effects[2]->endPassCalled_);

	CHECK_EQUAL(5, effects[0]->setAutoConstantsCalled_);
	CHECK_EQUAL(2, effects[1]->setAutoConstantsCalled_);
	CHECK_EQUAL(1, effects[2]->setAutoConstantsCalled_);

	CHECK_EQUAL(2, effects[0]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(2, effects[1]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(1, effects[2]->currentTechniqueSetCalled_.size());

	CHECK_EQUAL(5, effects[0]->commitChangesCalled_);
	CHECK_EQUAL(2, effects[1]->commitChangesCalled_);
	CHECK_EQUAL(1, effects[2]->commitChangesCalled_);

	CHECK_EQUAL(5, effects[0]->instanceDataUsed_.size());
	CHECK_EQUAL(2, effects[1]->instanceDataUsed_.size());
	CHECK_EQUAL(1, effects[2]->instanceDataUsed_.size());

	CHECK_EQUAL(3, vbs[0]->setVerticesCalled_);
	CHECK_EQUAL(4, vbs[1]->setVerticesCalled_);

	CHECK_EQUAL(1, ib0.setPrimitivesCalled_);
}

TEST_F( OpaqueRenderQueueSimpleFixture2, RenderQueue_multiPass )
{
	effect.mockNumPasses_ = 3;

	uint32 groupIndex = 11;

	// prepare data for the render op and record it
	// allocate and populate instance data
	InstanceData* instanceData = (InstanceData*)allocator.allocate( sizeof(InstanceData) + sizeof(Matrix));
	instanceData->paletteSize_ = 0;
	// generate random world transform matrix
	Matrix&	worldTransform = *instanceData->matrix();
	worldTransform.setTranslate( 1.0f, 2.0f, 3.0f );
	worldTransform.preRotateY( 1.5f );
	worldTransform.preRotateX( -1.0f );

	RenderOp rop;
	rop.effect_ = &effect;
	rop.technique_ = technique;
	rop.materialProperties_ = &material;
	rop.vertices_ = &vb;
	rop.primitives_ = &ib;
	rop.primGroupIndex_ = groupIndex;
	rop.instanceData_ = instanceData;
	rop.globalStateBlockIndex_ = GlobalStateRecorder::NULL_BUFFER_INDEX;
	rop.flags_ = 0;

	RenderQueue::deepTestRender( &rop, 1, globalStateRecorder, effectVisualContext, NULL );

	CHECK_EQUAL( 1, effect.beginCalled_);
	CHECK_EQUAL( 3, effect.beginPassCalled_ );
	CHECK_EQUAL( 3, effect.commitChangesCalled_ );
	CHECK_EQUAL( 3, effect.endPassCalled_ );
	CHECK_EQUAL( 1, effect.endCalled_ );
	CHECK_EQUAL( 3, effect.setAutoConstantsCalled_ );
	CHECK_EQUAL( 1, effect.currentTechniqueSetCalled_.size() );
	CHECK_EQUAL( technique, effect.currentTechniqueSetCalled_[0] );

	CHECK_EQUAL( 1, vb.setVerticesCalled_ );
	CHECK_EQUAL( 1, ib.setPrimitivesCalled_ );
	CHECK_EQUAL( 3, ib.drawPrimitiveGroupRecorded_.size() );
	CHECK_EQUAL( groupIndex, ib.drawPrimitiveGroupRecorded_[0] );
	CHECK_EQUAL( 1, effectProperty->applyCalled_.size() );
	CHECK_EQUAL( materialParamHandle, effectProperty->applyCalled_[0].hProperty_ );

	CHECK_EQUAL( 3, effect.instanceDataUsed_.size() );
	CHECK_EQUAL( 0, effect.instanceDataUsed_[0].paletteSize_ );

	CHECK( almostEqual(worldTransform, effect.instanceDataUsed_[0].matrix_) );
	CHECK( almostEqual(worldTransform, effect.instanceDataUsed_[1].matrix_) );
	CHECK( almostEqual(worldTransform, effect.instanceDataUsed_[2].matrix_) );
}

TEST_F( OpaqueRenderQueueComplexFixture2, RenderQueue_multiPass2 )
{
	effects[0]->mockNumPasses_ = 2;
	effects[2]->mockNumPasses_ = 5;
	setupWithMatrices();

	RenderQueue::sortRenderOpsDeep( renderOps, NUM_ROPS );
	RenderQueue::deepTestRender( renderOps, NUM_ROPS, globalStateRecoreder, effectVisualContext, NULL );

	CHECK_EQUAL(2, effects[0]->beginCalled_);
	CHECK_EQUAL(2, effects[1]->beginCalled_);
	CHECK_EQUAL(1, effects[2]->beginCalled_);

	CHECK_EQUAL(2 * effects[0]->mockNumPasses_, effects[0]->beginPassCalled_);
	CHECK_EQUAL(2, effects[1]->beginPassCalled_);
	CHECK_EQUAL(1 * effects[2]->mockNumPasses_, effects[2]->beginPassCalled_);

	CHECK_EQUAL(2, effects[0]->endCalled_);
	CHECK_EQUAL(2, effects[1]->endCalled_);
	CHECK_EQUAL(1, effects[2]->endCalled_);

	CHECK_EQUAL(2 * effects[0]->mockNumPasses_, effects[0]->endPassCalled_);
	CHECK_EQUAL(2, effects[1]->endPassCalled_);
	CHECK_EQUAL(1 * effects[2]->mockNumPasses_, effects[2]->endPassCalled_);

	CHECK_EQUAL(5 * effects[0]->mockNumPasses_, effects[0]->setAutoConstantsCalled_);
	CHECK_EQUAL(2, effects[1]->setAutoConstantsCalled_);
	CHECK_EQUAL(1 * effects[2]->mockNumPasses_, effects[2]->setAutoConstantsCalled_);

	CHECK_EQUAL(2, effects[0]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(2, effects[1]->currentTechniqueSetCalled_.size());
	CHECK_EQUAL(1, effects[2]->currentTechniqueSetCalled_.size());

	CHECK_EQUAL(5 * effects[0]->mockNumPasses_, effects[0]->commitChangesCalled_);
	CHECK_EQUAL(2, effects[1]->commitChangesCalled_);
	CHECK_EQUAL(1 * effects[2]->mockNumPasses_, effects[2]->commitChangesCalled_);

	CHECK_EQUAL(5 * effects[0]->mockNumPasses_, effects[0]->instanceDataUsed_.size());
	CHECK_EQUAL(2, effects[1]->instanceDataUsed_.size());
	CHECK_EQUAL(1 * effects[2]->mockNumPasses_, effects[2]->instanceDataUsed_.size());

	CHECK_EQUAL(5, vbs[0]->setVerticesCalled_);
	CHECK_EQUAL(6, vbs[1]->setVerticesCalled_);

	CHECK_EQUAL(1, ib0.setPrimitivesCalled_);
}
#endif // 0

