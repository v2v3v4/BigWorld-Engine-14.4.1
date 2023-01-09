#include "pch.hpp"

#include "math/matrix.hpp"
#include "math/math_extra.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "test_matrix_data.hpp"

BW_BEGIN_NAMESPACE

namespace TestMatrix
{

struct Fixture
{
	Fixture()
		: matrix_(  Vector4( 0.0f, 1.0f, 2.0f, 3.0f),
					Vector4( 4.0f, 5.0f, 6.0f, 7.0f),
					Vector4( 8.0f, 9.0f,10.0f,11.0f),
					Vector4(12.0f,13.0f,14.0f,15.0f) )
	{
		new CStdMf;
	}

	~Fixture()
	{
		delete CStdMf::pInstance();
	}

	Matrix matrix_;
};

bool validateMatrixMultiplication( const Matrix & m1, const Matrix & m2 )
{
	Matrix target;
	// Rolled up mulitply loop, we use this to verify the unrolled multiply
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				target( i, k ) = target( i, k ) + m1( i, j ) * m2( j, k );
			}
		}
	}

	Matrix actual;
	actual.multiply( m1, m2 );

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (!almostEqual( target( i, j ), actual( i, j ) ))
			{
				return false;
			}
		}
	}

	return true;
}

TEST_F( Fixture, Matrix_testConstruction )
{
	// Test initial value, should be a zero matrix
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			matrix_( i, j ) = float(i) * float(j);
		}
	}
}

TEST_F( Fixture, Matrix_testSetZero )
{
	matrix_.setZero();

	// Should be a zero matrix
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			CHECK( isZero( matrix_(i,j) ) );
		}
	}
}

TEST_F( Fixture, Matrix_testIndexing )
{
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			CHECK( isEqual( float(i) * 4.0f + float(j), matrix_( i, j ) ) );
		}
	}
}

TEST_F( Fixture, Matrix_testRow )
{	
	// read rows;
	Vector4 r[4];

	for ( uint i = 0; i < 4; i++ )
	{
		r[i] = matrix_.row(i);
	}

	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			CHECK( isEqual( r[i][j], matrix_( i, j ) ) );
		}
	}

	// write rows
	matrix_.row( 0, Vector4( 1.0f, 0.0f, 0.0f, 0.0f  ) );
	matrix_.row( 1, Vector4( 0.0f, 1.0f, 0.0f, 0.0f  ) );
	matrix_.row( 2, Vector4( 0.0f, 0.0f, 1.0f, 0.0f  ) );
	matrix_.row( 3, Vector4( 0.0f, 0.0f, 0.0f, 1.0f  ) );

	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			CHECK( isEqual( Matrix::identity(i, j) , matrix_( i, j ) ) );
		}
	}
}

TEST_F( Fixture, Matrix_testSetIdentity )
{
	matrix_.setIdentity();

	// Validate identity
	// Test initial value, should be a zero matrix
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{
			if ( j == i)
			{
				CHECK( isEqual( 1.0f, matrix_(i,j) ) );
			}
			else
			{
				CHECK( isZero( matrix_(i,j) ) );
			}
		}
	}
}

TEST_F( Fixture, Matrix_testTranspose )
{
	Matrix mt;

	// Set up matrix
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{		
			matrix_(i,j) = float(i) * float(j);
		}
	}

	mt.transpose(matrix_);

	// check matrix
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{		
			CHECK( isEqual( matrix_(i,j) , mt(j,i) ) );
		}
	}
}

TEST_F( Fixture, Matrix_testMultiply )
{
	Matrix zero, id, x;

	// set up some values
	zero.setZero();
	id.setIdentity();

	// x 
	for ( uint32 i = 0; i < 4; i++ )
	{
		for ( uint32 j = 0; j < 4; j++ )
		{		
			x(i,j) = float(i+1) * float(j+5);
		}
	}

	CHECK( validateMatrixMultiplication( zero, zero ) );

	CHECK( validateMatrixMultiplication( zero, x ) );
	CHECK( validateMatrixMultiplication( x, zero ) );

	CHECK( validateMatrixMultiplication( x, id ) );
	CHECK( validateMatrixMultiplication( id, x ) );

	CHECK( validateMatrixMultiplication( x, x ) );
}

TEST( Matrix_testAlmostEqual )
{
	for (int seed = 0; seed < 10; ++seed)
	{
		BWRandom random( seed );

		const Matrix testMat = createRandomTransform( random, -10.0f, 10.0f );
		const float epsilon = random( 0.0f, 1.0f );
		Matrix testMatPlusEps;
		Matrix testMatMinusEps;
		Matrix testMatPlusMoreThanEps;
		Matrix testMatMinusMoreThanEps;

		Matrix testMatPlusLessThanEps;
		Matrix testMatMinusLessThanEps;

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				const float testMatVal = testMat( i, j );

				testMatPlusEps( i, j ) = testMatVal + epsilon;
				testMatMinusEps( i, j ) = testMatVal - epsilon;
				
				testMatPlusMoreThanEps( i, j ) =
					testMatVal + epsilon + 0.00001f;
				MF_ASSERT( testMatPlusMoreThanEps( i, j ) >
					testMatPlusEps( i, j ) );
				testMatMinusMoreThanEps( i, j ) =
					testMatVal - epsilon - 0.00001f;
				MF_ASSERT( testMatMinusMoreThanEps( i, j ) <
					testMatMinusEps( i, j ) );

				testMatPlusLessThanEps( i, j ) =
					testMatVal + epsilon - 0.00001f;
				MF_ASSERT( testMatPlusLessThanEps( i, j ) <
					testMatPlusEps( i, j ) );
				testMatMinusLessThanEps( i, j ) =
					testMatVal - epsilon + 0.00001f;
				MF_ASSERT( testMatMinusLessThanEps( i, j ) >
					testMatMinusEps( i, j ) );
			}
		}
		
		//Expect fail because the almostEqual function is not inclusive
		//of values that differ by exactly epsilon.
		CHECK( !almostEqual( testMat, testMatPlusEps, epsilon ) );
		CHECK( !almostEqual( testMat, testMatMinusEps, epsilon ) );

		CHECK( !almostEqual( testMat, testMatPlusMoreThanEps, epsilon ) );
		CHECK( !almostEqual( testMat, testMatMinusMoreThanEps, epsilon ) );

		CHECK( almostEqual( testMat, testMatPlusLessThanEps, epsilon ) );
		CHECK( almostEqual( testMat, testMatMinusLessThanEps, epsilon ) );
	}
}

bool testYawPitchRollConversion( const float * mats, size_t numFloats )
{
	// Convert matrices to yaw, pitch, roll and convert back again.
	// Should get the same rotation matrix back. This may not be the
	// case in some instances as it is possible to have more than one
	// solution.

	// We only have the rotation matrix stored in the test matrices.

	const size_t numFloatsPerMat = 9;
	const size_t numMats = numFloats/numFloatsPerMat;

	for (size_t i = 0; i < numMats; ++i)
	{
		Matrix m;
		const float* values = mats + (i*numFloatsPerMat);
		m.row(0, Vector4( values[0], values[1], values[2], 0.f ));
		m.row(1, Vector4( values[3], values[4], values[5], 0.f ));
		m.row(2, Vector4( values[6], values[7], values[8], 0.f ));
		m.row(3, Vector4( 0.f, 0.f, 0.f, 1.f ));

		const float yaw = m.yaw();
		const float pitch = m.pitch();
		const float roll = m.roll();

		Matrix mRot;
		mRot.setRotate( yaw, pitch, roll );

		const float len0 = m.row(0).length();
		const float len1 = m.row(1).length();
		const float len2 = m.row(2).length();

		if (!almostEqual( len0, 1.f ) &&
			!almostEqual( len1, 1.f ) &&
			!almostEqual( len2, 1.f ) )
		{
			//TRACE_MSG("Not normalised %f %f %f\n", len0, len1, len2 );
			//continue;
		}

		// It is possible that there is scale in the matrices which would have
		// been lost by converted to roll, pitch and yaw. Reapply the 
		// scale we got from the original matrix.
		mRot.row( 0, mRot.row(0)*len0 );
		mRot.row( 1, mRot.row(1)*len1 );
		mRot.row( 2, mRot.row(2)*len2 );

		if(!almostEqual( m, mRot, 0.005f ))
		{
			return false;
		}
	}

	return true;
}

TEST( Matrix_testYawPitchRollConversion )
{
	// The test data used here is taken from urban, highlands spaces
	// which originally highlighted a problem in the older conversion code

	const size_t numUrban = sizeof(matrixValuesUrban)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesUrban, numUrban) );

	const size_t numHighlands = sizeof(matrixValuesHighlands)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesHighlands, numHighlands) );

	const size_t numMinspec = sizeof(matrixValuesMinspec)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesMinspec, numMinspec) );

	const size_t numArctic = sizeof(matrixValuesArctic)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesArctic, numArctic) );

	const size_t numDesert = sizeof(matrixValuesDesert)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesDesert, numDesert) );

	const size_t numAllModels = sizeof(matrixValuesAllModels)/sizeof(float);
	CHECK( testYawPitchRollConversion(matrixValuesAllModels, numAllModels) );
}

} // namespace TestMatrix

BW_END_NAMESPACE

// test_matrix.cpp
