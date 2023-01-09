#include "pch.hpp"

#include <memory>
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"
#include "math/math_extra.hpp"
#include "duplo/ik_constraint_system.hpp"
#include "duplo/pymodel.hpp"
#include "model/super_model.hpp"
#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	///The name of the model file used for the IKConstraintSystem tests.
	const char IKCS_TEST_MODEL_NAME[] = "ikcs_test_scene.model";
	///Nodes available in the test model.
	const char * const IKCS_TEST_MODEL_NODES[] =
		{
			"cube_joint",
			"sphere_joint",
			"cylinder_joint",
			"cone_joint",
			"torus_joint",
			"torus_child_joint"
		};
	const int NUM_IKCS_TEST_MODEL_NODES =
		sizeof(IKCS_TEST_MODEL_NODES) / sizeof(const char * const);


	/**
	 *	@return The test model node name for @a index.
	 */
	const char * getTestModelNodeId( int index )
	{
		MF_ASSERT( 0 <= index );
		MF_ASSERT( index < NUM_IKCS_TEST_MODEL_NODES );
		return IKCS_TEST_MODEL_NODES[index];
	}


	/**
	 *	@pre @a minScale not more than @a maxScale and is more than 0.
	 *	@post Set @a scaleMatrix to contain a scale matrix applying the scales
	 *		for axes assigned into @a scales. The scales have absolute values
	 *		in the range [@a minScale, @a maxScale] and the sign is random.
	 */
	void createRandomScaleMatrix( BW_NAMESPACE BWRandom & randomSource,
		Matrix & scaleMatrix, Vector3 & scales,
		float minScale = 0.0001f, float maxScale = 5.0f )
	{
		MF_ASSERT( 0.0f < minScale );
		MF_ASSERT( minScale <= maxScale );

		scales = createRandomNonZeroVector3( randomSource, minScale, maxScale );
		scaleMatrix.setScale( scales );
	}


	/**
	 *	@pre @a minScale not more than @a maxScale and is more than 0.
	 *	@post Returned a scale matrix with random scale for each axis in the
	 *		range +/-[@a minScale, @a maxScale].
	 */
	Matrix createRandomScaleMatrix( BW_NAMESPACE BWRandom & randomSource,
		float minScale = 0.0001f, float maxScale = 5.0f )
	{
		Matrix result;
		Vector3 scales;
		createRandomScaleMatrix(
			randomSource, result, scales, minScale, maxScale );
		return result;
	}


	/**
	 *	@return The IKCS test model, or NULL if the model didn't load or has
	 *		unexpected data. It's guaranteed to have an instance node tree
	 *		with nodes for all IKCS_TEST_MODEL_NODES.
	 */
	std::auto_ptr<SuperModel> loadTestModel()
	{
		BW::vector<BW::string> modelIds( 1, IKCS_TEST_MODEL_NAME );

		std::auto_ptr<SuperModel> result( new SuperModel( modelIds ) );
		if (result->nModels() != static_cast<int>( modelIds.size() ) )
		{
			DEBUG_MSG( "Failed to load all test models.\n" );
			result.reset();
		}
		const SuperModelNodeTree * nodeTree = result->getInstanceNodeTree();
		if (nodeTree == NULL)
		{
			DEBUG_MSG( "Test model incompatible with instance node tree.\n" );
			result.reset();
		}
		for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
		{
			if (nodeTree->getNodeIndex( getTestModelNodeId( node ) ) < 0)
			{
				DEBUG_MSG( "Test model node %s is missing from the input "
					"model.\n", getTestModelNodeId( node ) );
				nodeTree = NULL;
				result.reset();
				break;
			}
		}

		return result;
	}


	/**
	 *	@return An empty IKConstraintSystem.
	 */
	IKConstraintSystemPtr createIKCS()
	{
		return IKConstraintSystemPtr(
			new IKConstraintSystem,
			IKConstraintSystemPtr::STEAL_REFERENCE );
	}


	/**
	 *	This functor simply checks base Constraint interface access is
	 *	returning the expected data.
	 */
	class TestConstraintDataAccess
	{
	public:
		TestConstraintDataAccess(
			ConstraintPtr & constraint,
			const BW::string & identifier,
			ConstraintSourcePtr & source,
			ConstraintTargetPtr & target,
			float weight,
			Constraint::Type type )
			:
			constraint_( constraint )
			, identifier_( identifier )
			, source_( source )
			, target_( target )
			, weight_( weight )
			, type_( type )
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;
			
			CHECK_EQUAL( identifier_, constraint_->getIdentifier() );
			CHECK( source_.get() == constraint_->getSource().get() );
			CHECK( target_.get() == constraint_->getTarget().get() );
			CHECK( weight_ == constraint_->getWeight() );
			CHECK( type_ == constraint_->getType() );
		}

	private:
		ConstraintPtr constraint_;
		BW::string identifier_;
		ConstraintSourcePtr source_;
		ConstraintTargetPtr target_;
		float weight_;
		Constraint::Type type_;
	};
}


///Test the ConstraintTarget class.
TEST( IKConstraintSystem_ConstraintTarget )
{
	BW_GUARD;
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int testNode = 0; testNode < NUM_IKCS_TEST_MODEL_NODES; ++testNode)
	{
		const BW::string & testNodeId = getTestModelNodeId( testNode );

		ConstraintTargetPtr tgtm1(
			new ConstraintTarget(
				ConstraintTarget::MODEL_NODE, testNodeId ),
			ConstraintTargetPtr::STEAL_REFERENCE );
		ConstraintTargetPtr tgtm2(
			new ConstraintTarget(
				ConstraintTarget::MODEL_NODE, testNodeId ),
			ConstraintTargetPtr::STEAL_REFERENCE );
		ConstraintTargetPtr tgtikt(
			new ConstraintTarget(
				ConstraintTarget::IK_HANDLE_TARGET, "ikhandletarget" ),
			ConstraintTargetPtr::STEAL_REFERENCE );
		ConstraintTargetPtr tgtikpv(
			new ConstraintTarget(
				ConstraintTarget::IK_HANDLE_POLE_VECTOR, "ikhandlepolevector" ),
			ConstraintTargetPtr::STEAL_REFERENCE );
	
		CHECK( *tgtm1 == *tgtm2 );
		CHECK( !(*tgtm1 == *tgtikt) );
		CHECK( !(*tgtikt == *tgtikpv) );
	
		CHECK_EQUAL( testNodeId, tgtm1->getIdentifier() );
		CHECK_EQUAL( "ikhandletarget", tgtikt->getIdentifier() );
		CHECK_EQUAL( "ikhandlepolevector", tgtikpv->getIdentifier() );
	
		CHECK( tgtm1->getType() == ConstraintTarget::MODEL_NODE );
		CHECK( tgtikt->getType() == ConstraintTarget::IK_HANDLE_TARGET );
		CHECK( tgtikpv->getType() == ConstraintTarget::IK_HANDLE_POLE_VECTOR );
	

	
		const int testNodeIndex =
			testModel->getInstanceNodeTree()->getNodeIndex( testNodeId );
		RETURN_ON_FAIL_CHECK( testNodeIndex >= 0 );

		const Matrix & expectedMatrix = testModel->getNodeWorldTransform(
			testNodeIndex );
		
		CHECK( tgtm1->getMatrix( *testModel, *system ) == expectedMatrix );

		//Matrix returned for IK handle targets isn't significant.
	}
}


///Test the ConstraintSourceMatrixProvider class.
TEST( IKConstraintSystem_ConstraintSourceMatrixProvider )
{
	BW_GUARD;

	const uint32 SEED = 1;
	BWRandom random( SEED );
	
	Matrix testInMat = createRandomRotateTranslate( random );

	PyMatrix * directPtr = NULL;
	MatrixProviderPtr matProvider(
		directPtr = new PyMatrix,
		MatrixProviderPtr::STEAL_REFERENCE );
	directPtr->set( testInMat );

	ConstraintSourcePtr mpcs(
		new MatrixProviderConstraintSource( matProvider ),
		ConstraintSourcePtr::STEAL_REFERENCE );
	
	CHECK_EQUAL( ConstraintSource::NONE, mpcs->getDependency().first );

	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	IKConstraintSystemPtr system = createIKCS();

	Matrix testOutMat;
	matProvider->matrix( testOutMat );
	CHECK( testInMat == testOutMat );
	CHECK( mpcs->getMatrix( *testModel, *system ) == testOutMat );
	
	testInMat = createRandomRotateTranslate( random );
	directPtr->set( testInMat );
	matProvider->matrix( testOutMat );
	CHECK( testInMat == testOutMat );
	CHECK( mpcs->getMatrix( *testModel, *system ) == testOutMat );
}


///Test the ConstraintSourceModelNode class.
TEST( IKConstraintSystem_ConstraintSourceModelNode )
{
	BW_GUARD;
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		ConstraintSourcePtr mncs(
			new ModelNodeConstraintSource( getTestModelNodeId( node ) ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		CHECK( mncs->getDependency() ==
			ConstraintSource::Dependency(
				ConstraintSource::MODEL_NODE, getTestModelNodeId( node ) ) );

		const int nodeIndex = testModel->getInstanceNodeTree()->getNodeIndex(
			getTestModelNodeId( node ) );
		RETURN_ON_FAIL_CHECK( nodeIndex >= 0 );

		const Matrix & expectedMatrix = testModel->getNodeWorldTransform(
			nodeIndex );

		CHECK( mncs->getMatrix( *testModel, *system ) ==
			testModel->getNodeWorldTransform( nodeIndex ) );
	}
}


///Test the PointConstraint class.
TEST( IKConstraintSystem_PointConstraint )
{
	const uint32 SEED = 2;
	BWRandom random( SEED );
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		const BW::string & nodeSourceId =
			getTestModelNodeId( node );
		const BW::string & nodeTargetId =
			getTestModelNodeId( (node + 1) % NUM_IKCS_TEST_MODEL_NODES );
		const int nodeSourceIndex = nodeTree->getNodeIndex( nodeSourceId );
		MF_ASSERT( nodeSourceIndex >= 0 );
		const int nodeTargetIndex = nodeTree->getNodeIndex( nodeTargetId );
		MF_ASSERT( nodeTargetIndex >= 0 );


		ConstraintSourcePtr testSource(
			new ModelNodeConstraintSource( nodeSourceId ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		ConstraintTargetPtr testTarget(
			new ConstraintTarget( ConstraintTarget::MODEL_NODE, nodeTargetId ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		const char TEST_ID[] = "pointConstraint";
		const float testWeight = random( 0.001f, 10.0f);
		PointConstraint * directPtr = NULL;
		ConstraintPtr constraint(
			directPtr = new PointConstraint( TEST_ID, testSource, testTarget ),
			ConstraintPtr::STEAL_REFERENCE );
		constraint->setWeight( testWeight );

		TEST_SUB_FUNCTOR( TestConstraintDataAccess( constraint,
			TEST_ID, testSource, testTarget, testWeight,
			Constraint::POSITION ) );
	
		const Vector3 testOffset = createRandomVector3( random, -10.0f, 10.0f );
		directPtr->setOffset( testOffset );
		CHECK_EQUAL( testOffset, directPtr->getOffset() );

		//No scale.
		Matrix sourceMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		Matrix targetMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		Vector3 expectedPos(
			sourceMatrix.applyToOrigin() + testOffset );
		Matrix expectedConstrained(
			targetMatrix.row( 0 ),
			targetMatrix.row( 1 ),
			targetMatrix.row( 2 ),
			Vector4( expectedPos[0], expectedPos[1], expectedPos[2], 1.0f) );

		Matrix constrained = constraint->getMatrix( *testModel, *system );

		CHECK( almostEqual( expectedConstrained, constrained ) );

		//With scale.
		sourceMatrix.preMultiply( createRandomScaleMatrix( random ) );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		targetMatrix.preMultiply( createRandomScaleMatrix( random ) );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		expectedPos = sourceMatrix.applyToOrigin() + testOffset;
		expectedConstrained = Matrix(
			targetMatrix.row( 0 ),
			targetMatrix.row( 1 ),
			targetMatrix.row( 2 ),
			Vector4( expectedPos[0], expectedPos[1], expectedPos[2], 1.0f) );

		constrained = constraint->getMatrix( *testModel, *system );

		CHECK( almostEqual( expectedConstrained, constrained ) );
	}
}


///Test the OrientConstraint class.
TEST( IKConstraintSystem_OrientConstraint )
{
	const uint32 SEED = 3;
	BWRandom random( SEED );
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		const BW::string & nodeSourceId =
			getTestModelNodeId( node );
		const BW::string & nodeTargetId =
			getTestModelNodeId( (node + 1) % NUM_IKCS_TEST_MODEL_NODES );
		const int nodeSourceIndex = nodeTree->getNodeIndex( nodeSourceId );
		MF_ASSERT( nodeSourceIndex >= 0 );
		const int nodeTargetIndex = nodeTree->getNodeIndex( nodeTargetId );
		MF_ASSERT( nodeTargetIndex >= 0 );


		ConstraintSourcePtr testSource(
			new ModelNodeConstraintSource( nodeSourceId ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		ConstraintTargetPtr testTarget(
			new ConstraintTarget( ConstraintTarget::MODEL_NODE, nodeTargetId ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		const char TEST_ID[] = "orientConstraint";
		const float testWeight = random( 0.001f, 10.0f);
		OrientConstraint * directPtr = NULL;
		ConstraintPtr constraint(
			directPtr = new OrientConstraint( TEST_ID, testSource, testTarget ),
			ConstraintPtr::STEAL_REFERENCE );
		constraint->setWeight( testWeight );

		TEST_SUB_FUNCTOR( TestConstraintDataAccess( constraint,
			TEST_ID, testSource, testTarget, testWeight,
			Constraint::ROTATION ) );
	
		//No scale.
		const Vector3 testOffset = createRandomVector3( random, -10.0f, 10.0f );
		directPtr->setOffset( testOffset );
		CHECK_EQUAL( testOffset, directPtr->getOffset() );

		Matrix sourceMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		Matrix targetMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		const Vector3 expectedPos(
			targetMatrix.applyToOrigin() );
		Matrix expectedRotation( sourceMatrix );
		expectedRotation.preRotateZ( testOffset[2] );
		expectedRotation.preRotateY( testOffset[1] );
		expectedRotation.preRotateX( testOffset[0] );

		Matrix expectedConstrained(
			expectedRotation.row( 0 ),
			expectedRotation.row( 1 ),
			expectedRotation.row( 2 ),
			Vector4( expectedPos[0], expectedPos[1], expectedPos[2], 1.0f) );

		Matrix constrained = constraint->getMatrix( *testModel, *system );

		CHECK( almostEqual( expectedConstrained, constrained ) );

		//With scale.
		sourceMatrix.preMultiply( createRandomScaleMatrix( random ) );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		const Matrix targetScaleMatrix = createRandomScaleMatrix( random );
		targetMatrix.preMultiply( targetScaleMatrix );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		expectedConstrained.preMultiply( targetScaleMatrix );

		constrained = constraint->getMatrix( *testModel, *system );

		CHECK( almostEqual( expectedConstrained, constrained ) );
	}
}


///Test the AimConstraint class.
TEST( IKConstraintSystem_AimConstraint )
{
	const uint32 SEED = 4;
	BWRandom random( SEED );
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		const BW::string & nodeSourceId =
			getTestModelNodeId( node );
		const BW::string & nodeTargetId =
			getTestModelNodeId( (node + 1) % NUM_IKCS_TEST_MODEL_NODES );
		const int nodeSourceIndex = nodeTree->getNodeIndex( nodeSourceId );
		MF_ASSERT( nodeSourceIndex >= 0 );
		const int nodeTargetIndex = nodeTree->getNodeIndex( nodeTargetId );
		MF_ASSERT( nodeTargetIndex >= 0 );


		ConstraintSourcePtr testSource(
			new ModelNodeConstraintSource( nodeSourceId ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		ConstraintTargetPtr testTarget(
			new ConstraintTarget( ConstraintTarget::MODEL_NODE, nodeTargetId ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		const char TEST_ID[] = "aimConstraint";
		const float testWeight = random( 0.001f, 10.0f);
		AimConstraint * directPtr = NULL;
		ConstraintPtr constraint(
			directPtr = new AimConstraint( TEST_ID, testSource, testTarget ),
			ConstraintPtr::STEAL_REFERENCE );
		constraint->setWeight( testWeight );

		TEST_SUB_FUNCTOR( TestConstraintDataAccess( constraint,
			TEST_ID, testSource, testTarget, testWeight,
			Constraint::ROTATION ) );
	
		const Vector3 testOffset( 0.1f, -2.6f, 5.14f );
		directPtr->setOffset( testOffset );
		CHECK_EQUAL( testOffset, directPtr->getOffset() );

		const Vector3 TEST_AIM( 0.1f, -0.6f, 2.14f );
		const Vector3 TEST_UP( 0.1f, 1.4f, 0.14f );
		const Vector3 TEST_WUP( 0.15f, 2.1f, 0.13f );
		directPtr->setAimVector( TEST_AIM );
		CHECK_EQUAL( TEST_AIM, directPtr->getAimVector() );
		directPtr->setUpVector( TEST_UP );
		CHECK_EQUAL( TEST_UP, directPtr->getUpVector() );
		directPtr->setWorldUpVector( TEST_WUP );
		CHECK_EQUAL( TEST_WUP, directPtr->getWorldUpVector() );

		//No scale.
		Matrix sourceMatrix;
		sourceMatrix.setRotateX( 0.3f );
		sourceMatrix.postRotateY( -0.2f );
		sourceMatrix.postRotateZ( 5.3f );
		sourceMatrix.postTranslateBy( Vector3( 4.5f, 6.3f, 9.8f ) );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		Matrix targetMatrix;
		targetMatrix.setRotateX( 0.3f );
		targetMatrix.postRotateY( -0.2f );
		targetMatrix.postRotateZ( 5.3f );
		targetMatrix.postTranslateBy( Vector3( 4.5f, 6.3f, 9.8f ) );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		//We only do one comparison for aim (not randomised). We just use a
		//known-correct result to check that the code likely hasn't become
		//broken by changes. Otherwise, calculating expected results is too
		//complex to be worth doing here.
		Matrix expectedConstrained(
			Vector4( 0.2205f,  0.8607f,  0.4588f, 4.5000f ),
			Vector4( 0.7379f, -0.4548f,  0.4987f, 6.3000f ),
			Vector4( 0.6379f,  0.2286f, -0.7354f, 9.8000f ),
			Vector4( 0.0000f,  0.0000f,  0.0000f, 1.0000f ));
		expectedConstrained.transpose();

		Matrix constrained = constraint->getMatrix( *testModel, *system );
	
		CHECK( almostEqual( expectedConstrained, constrained ) );

		//With scale.
		sourceMatrix.preMultiply( createRandomScaleMatrix( random ) );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		const Matrix targetScaleMatrix = createRandomScaleMatrix( random );
		targetMatrix.preMultiply( targetScaleMatrix );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		expectedConstrained.preMultiply( targetScaleMatrix );

		constrained = constraint->getMatrix( *testModel, *system );
		
		CHECK( almostEqual( expectedConstrained, constrained ) );
	}
}


///Test the ParentConstraint class.
TEST( IKConstraintSystem_ParentConstraint )
{
	const uint32 SEED = 5;
	BWRandom random( SEED );
	
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		const BW::string & nodeSourceId =
			getTestModelNodeId( node );
		const BW::string & nodeTargetId =
			getTestModelNodeId( (node + 1) % NUM_IKCS_TEST_MODEL_NODES );
		const int nodeSourceIndex = nodeTree->getNodeIndex( nodeSourceId );
		MF_ASSERT( nodeSourceIndex >= 0 );
		const int nodeTargetIndex = nodeTree->getNodeIndex( nodeTargetId );
		MF_ASSERT( nodeTargetIndex >= 0 );


		ConstraintSourcePtr testSource(
			new ModelNodeConstraintSource( nodeSourceId ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		ConstraintTargetPtr testTarget(
			new ConstraintTarget( ConstraintTarget::MODEL_NODE, nodeTargetId ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		const char TEST_ID[] = "orientConstraint";
		const float testWeight = random( 0.001f, 10.0f);
		ParentConstraint * directPtr = NULL;
		ConstraintPtr constraint(
			directPtr = new ParentConstraint( TEST_ID, testSource, testTarget ),
			ConstraintPtr::STEAL_REFERENCE );
		constraint->setWeight( testWeight );

		TEST_SUB_FUNCTOR( TestConstraintDataAccess( constraint,
			TEST_ID, testSource, testTarget, testWeight,
			Constraint::PARENT ) );
		
		const Vector3 testPosOffset = createRandomVector3(
			random, -10.0f, 10.0f );
		directPtr->setTranslationOffset( testPosOffset );
		CHECK_EQUAL( testPosOffset, directPtr->getTranslationOffset() );
	
		const Vector3 testRotOffset = createRandomVector3(
			random, -10.0f, 10.0f );
		directPtr->setRotationOffset( testRotOffset );
		CHECK_EQUAL( testRotOffset, directPtr->getRotationOffset() );

		//No scale.
		Matrix sourceMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		Matrix targetMatrix = createRandomRotateTranslate( random );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		Matrix expectedConstrained;
		expectedConstrained.setRotateX( testRotOffset[0] );
		expectedConstrained.postRotateY( testRotOffset[1] );
		expectedConstrained.postRotateZ( testRotOffset[2] );
		expectedConstrained[3] = testPosOffset;
		expectedConstrained.postMultiply( sourceMatrix );

		Matrix constrained = constraint->getMatrix( *testModel, *system );

		CHECK( almostEqual( expectedConstrained, constrained ) );

		//With scale.
		Matrix sourceScaleMatrix;
		Vector3 sourceScale;
		createRandomScaleMatrix( random, sourceScaleMatrix, sourceScale );
		const Matrix sourceMatrixNoScale( sourceMatrix );
		sourceMatrix.preMultiply( sourceScaleMatrix );
		testModel->setNodeWorldTransform( nodeSourceIndex, sourceMatrix );

		Matrix targetScaleMatrix;
		Vector3 targetScale;
		createRandomScaleMatrix( random, targetScaleMatrix, targetScale );
		targetMatrix.preMultiply( targetScaleMatrix );
		testModel->setNodeWorldTransform( nodeTargetIndex, targetMatrix );

		//Parent constraint preserves only the scale of the target,
		//and the source scale applies to the translation offset.
		expectedConstrained.setScale( targetScale );
		expectedConstrained.postRotateX( testRotOffset[0] );
		expectedConstrained.postRotateY( testRotOffset[1] );
		expectedConstrained.postRotateZ( testRotOffset[2] );
		expectedConstrained[3] = testPosOffset * sourceScale;
		expectedConstrained.postMultiply( sourceMatrixNoScale );

		constrained = constraint->getMatrix( *testModel, *system );
		
		CHECK( almostEqual( expectedConstrained, constrained ) );
	}
}


///Test the ConstraintConstraintSource class.
TEST( IKConstraintSystem_ConstraintConstraintSource )
{
	const std::auto_ptr<SuperModel> testModel = loadTestModel();
	RETURN_ON_FAIL_CHECK( testModel.get() != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );
	IKConstraintSystemPtr system = createIKCS();

	for (int node = 0; node < NUM_IKCS_TEST_MODEL_NODES; ++node)
	{
		const BW::string & nodeSourceId =
			getTestModelNodeId( node );
		const BW::string & nodeTargetId =
			getTestModelNodeId( (node + 1) % NUM_IKCS_TEST_MODEL_NODES );
		const int nodeSourceIndex = nodeTree->getNodeIndex( nodeSourceId );
		MF_ASSERT( nodeSourceIndex >= 0 );
		const int nodeTargetIndex = nodeTree->getNodeIndex( nodeTargetId );
		MF_ASSERT( nodeTargetIndex >= 0 );
		
		ConstraintSourcePtr testSource(
			new ModelNodeConstraintSource( nodeSourceId ),
			ConstraintSourcePtr::STEAL_REFERENCE );
		ConstraintTargetPtr testTarget(
			new ConstraintTarget( ConstraintTarget::MODEL_NODE, nodeTargetId ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		ConstraintPtr constraint(
			new PointConstraint( "pointConstraint", testSource, testTarget ),
			ConstraintPtr::STEAL_REFERENCE );

		ConstraintSourcePtr ccs(
			new ConstraintConstraintSource( constraint ),
			ConstraintSourcePtr::STEAL_REFERENCE );

		CHECK( ccs->getDependency() ==
			ConstraintSource::Dependency(
				ConstraintSource::CONSTRAINT, constraint->getIdentifier() ) );

		const Matrix sourceMatrix =
			testModel->getNodeWorldTransform( nodeSourceIndex );

		const Matrix targetMatrix =
			testModel->getNodeWorldTransform( nodeTargetIndex );

		const Vector3 expectedPos( sourceMatrix.applyToOrigin() );
		const Matrix expectedConstrained(
			targetMatrix.row( 0 ),
			targetMatrix.row( 1 ),
			targetMatrix.row( 2 ),
			Vector4( expectedPos[0], expectedPos[1], expectedPos[2], 1.0f) );

		CHECK( almostEqual( expectedConstrained,
			ccs->getMatrix( *testModel, *system ) ) );
	}
}


///Test the IKHandle class.
TEST( IKConstraintSystem_IKHandle )
{
	const uint32 SEED = 6;
	BWRandom random( SEED );
	
	PyModelPtr testPyModel( PyModel::createTestPyModel( loadTestModel() ) );
	SuperModel * testModel = testPyModel->pSuperModel();
	RETURN_ON_FAIL_CHECK( testModel != NULL );
	const SuperModelNodeTree * nodeTree = testModel->getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );

	IKConstraintSystemPtr system = createIKCS();

	//Need to attach the test IKConstraintSystem to our PyModel. 
	testPyModel->pySetAttribute( ScriptString::create( "testIKCS" ), 
		ScriptObject( system ) );

	const char CHAIN_START[] = "sphere_joint";
	const char CHAIN_MIDDLE[] = "torus_joint";
	const char CHAIN_END[] = "torus_child_joint";
	const int chainStartIndex = nodeTree->getNodeIndex( CHAIN_START );
	const int chainMiddleIndex = nodeTree->getNodeIndex( CHAIN_MIDDLE );
	const int chainEndIndex = nodeTree->getNodeIndex( CHAIN_END );
	MF_ASSERT( chainStartIndex >= 0 );
	MF_ASSERT( chainMiddleIndex >= 0 );
	MF_ASSERT( chainEndIndex >= 0 );

	CCDChainIKHandle * directPtr = NULL;
	const char TEST_IDENTIFIER[] = "testIKHandle";
	IKHandlePtr ikHandle(
		directPtr =
			new CCDChainIKHandle( TEST_IDENTIFIER, CHAIN_START, CHAIN_END ),
		IKHandlePtr::STEAL_REFERENCE );
	CHECK_EQUAL( TEST_IDENTIFIER, ikHandle->getIdentifier() );
	
	//Arbitrary number of iterations should be large enough to give time for
	//a solution to be reached even if the random data is very far from the
	//solution.
	const int MAX_NUM_ITERATIONS = 1517;
	directPtr->setMaxNumIterations( MAX_NUM_ITERATIONS );
	CHECK_EQUAL( MAX_NUM_ITERATIONS, directPtr->getMaxNumIterations() );
	const float TARGET_DIST_TOLERANCE = 0.000007654321f;
	directPtr->setTargetDistTolerance( TARGET_DIST_TOLERANCE );
	CHECK_EQUAL( TARGET_DIST_TOLERANCE, directPtr->getTargetDistTolerance() );

	//Test bone index access.
	{
		int chainSearch = 0;

		const BW::vector<int> boneIndices =
			ikHandle->getTargetBoneIndices( *testModel );
		RETURN_ON_FAIL_CHECK( boneIndices.size() == 3 );
		
		CHECK_EQUAL( chainStartIndex, boneIndices.at( 0 ) );
		CHECK_EQUAL( chainMiddleIndex, boneIndices.at( 1 ) );
		CHECK_EQUAL( chainEndIndex, boneIndices.at( 2 ) );

		//Check recalculated correctly after reset.
		ikHandle->resetCachedIds();
		CHECK( boneIndices ==  ikHandle->getTargetBoneIndices( *testModel ) );
	}


	//Do some number of loops.
	for (int testLoop = 0; testLoop < 10; ++testLoop)
	{
		//We make sure to test poleVector as a zero vector and not from
		//model, since that is a special case that indicates no pole vector
		//control.
		const bool usePoleVector = ((testLoop % 2) == 0);
		const Vector3 poleVector = usePoleVector ?
			createRandomVector3( random, -5.0f, 5.0f ) : Vector3::ZERO;
		const bool poleIsFromModel = usePoleVector && (random( 0, 1 ) == 1);
		ikHandle->setPoleVector( poleVector, poleIsFromModel );
		CHECK_EQUAL( poleVector, ikHandle->getPoleVector() );
		CHECK_EQUAL( poleIsFromModel,
			ikHandle->getPoleVectorIsFromModelPoint() );
		
		const bool drawTarget = (random( 0, 1 ) != 0);
		ikHandle->setDrawTarget( drawTarget );
		CHECK_EQUAL( drawTarget, ikHandle->getDrawTarget() );
		
		const bool drawPoleVector = (random( 0, 1 ) != 0);
		ikHandle->setDrawPoleVector( drawPoleVector );
		CHECK_EQUAL( drawPoleVector, ikHandle->getDrawPoleVector() );

		//Now do the following test on the system:
		//1. configure bone positions (random).
		//2. save bone lengths along the chain.
		//3. set a test target (a reachable one).
		//4. apply, and check that bone lengths haven't changed while the
		//	end is close to the target.

		//1 and 2.
		const Matrix baseWorldTrans =
			createRandomRotateTranslateScale( random );
		testPyModel->worldTransform( baseWorldTrans );
		RETURN_ON_FAIL_CHECK(
			!!(testPyModel->worldTransform() == baseWorldTrans) );
		//If the start node has a parent, must configure it as if animation
		//was propagated to it.
		const int parentIndex =
			nodeTree->getNode( chainStartIndex ).getParentIndex();
		if (parentIndex >= 0)
		{
			testModel->setNodeRelativeTransform(
				parentIndex, Matrix::identity );
			testModel->setNodeWorldTransform(
				parentIndex, baseWorldTrans );
		}
		
		const Matrix relTransStart = createRandomRotateTranslateScale( random ); 
		Matrix worldTrans;
		worldTrans.multiply( relTransStart, baseWorldTrans );
		const Vector3 startPos = worldTrans.applyToOrigin();
		testModel->setNodeRelativeTransform( chainStartIndex, relTransStart );
		testModel->setNodeWorldTransform( chainStartIndex, worldTrans );
		
		const Matrix relTransMiddle =
			createRandomRotateTranslateScale( random );
		worldTrans.multiply( relTransMiddle, worldTrans );
		const Vector3 middlePos = worldTrans.applyToOrigin();
		const float startToMidDist = startPos.distance( middlePos );
		testModel->setNodeRelativeTransform( chainMiddleIndex, relTransMiddle );
		testModel->setNodeWorldTransform( chainMiddleIndex, worldTrans );

		const Matrix relTransEnd = createRandomRotateTranslateScale( random );
		worldTrans.multiply( relTransEnd, worldTrans );
		const Vector3 endPos = worldTrans.applyToOrigin();
		const float midToEndDist = middlePos.distance( endPos );
		testModel->setNodeRelativeTransform( chainEndIndex, relTransEnd );
		testModel->setNodeWorldTransform( chainEndIndex, worldTrans );

		//We adjust our epsilon based on the scale of the bones.
		const float testEpsilon = (startToMidDist + midToEndDist) * 0.001f;

		//3
		//We make sure the target point is reachable by applying a random
		//rotation to the first and second joints and recording where the
		//target would end up. Otherwise (depending on bone lengths) we could
		//choose a target that can't be reached. E.g., if the first bone is
		//10 units long and the second one 0.1 units long, it would be
		//impossible to reach a point less than 9.9 units or more than 10.1
		//units from the start joint.
		Matrix randomRotateMiddle = createRandomRotate( random );
		randomRotateMiddle.preTranslateBy( -startPos );
		randomRotateMiddle.postTranslateBy( startPos );
		Matrix worldTransRotatedMiddle;
		worldTransRotatedMiddle.multiply(
			testModel->getNodeWorldTransform( chainMiddleIndex ),
			randomRotateMiddle );
		const Vector3 randomRotateMiddlePos =
			worldTransRotatedMiddle.applyToOrigin();
		MF_ASSERT( almostEqual(
			startPos.distance( middlePos ),
			startPos.distance( randomRotateMiddlePos ),
			testEpsilon ) );

		Matrix worldTransEnd;
		worldTransEnd.multiply(
			testModel->getNodeRelativeTransform( chainEndIndex ),
			worldTransRotatedMiddle );

		Matrix randomRotateEnd = createRandomRotate( random );
		randomRotateEnd.preTranslateBy( -randomRotateMiddlePos );
		randomRotateEnd.postTranslateBy( randomRotateMiddlePos );
		Matrix worldTransRotatedEnd;
		worldTransRotatedEnd.multiply( worldTransEnd, randomRotateEnd );
		const Vector3 randomRotateEndPos =
			worldTransRotatedEnd.applyToOrigin();
		MF_ASSERT( almostEqual(
			middlePos.distance( endPos ),
			randomRotateMiddlePos.distance( randomRotateEndPos ),
			testEpsilon ) );

		const Vector3 target = randomRotateEndPos;
		//The target is in model space, so must convert.
		Matrix baseWorldTransInv = baseWorldTrans;
		RETURN_ON_FAIL_CHECK( baseWorldTransInv.invert() );
		Vector3 modelSpaceTarget = baseWorldTransInv.applyPoint( target );
		ikHandle->setTarget( modelSpaceTarget );
		CHECK_EQUAL( modelSpaceTarget, ikHandle->getTarget() );

		//4
		CHECK( ikHandle->apply( *testModel, *system ) );
		const Vector3 outStartPos = testModel->getNodeWorldTransform(
			chainStartIndex ).applyToOrigin();
		const Vector3 outMiddlePos = testModel->getNodeWorldTransform(
			chainMiddleIndex ).applyToOrigin();
		const Vector3 outEndPos = testModel->getNodeWorldTransform(
			chainEndIndex ).applyToOrigin();
		const float outStartToMidDist = outStartPos.distance( outMiddlePos );
		const float outMidToEndDist = outMiddlePos.distance( outEndPos );
		
		CHECK( almostZero( target.distance( outEndPos ),
			testEpsilon * 15.0f ) );
		CHECK( almostEqual( startToMidDist, outStartToMidDist, testEpsilon ) );
		CHECK( almostEqual( midToEndDist, outMidToEndDist, testEpsilon ) );
	}
}

BW_END_NAMESPACE
