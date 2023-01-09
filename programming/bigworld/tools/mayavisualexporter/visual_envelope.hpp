#ifndef VISUAL_ENVELOPE_HPP
#define VISUAL_ENVELOPE_HPP

#include "visual_mesh.hpp"
#include "skin.hpp"

#include "moo/vertex_formats.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer<class VisualEnvelope> VisualEnvelopePtr;

/**
 *	This class ...
 *
 *	@todo Document this class.
 */
class VisualEnvelope : public VisualMesh
{
public:
	// Typedefs
	typedef BW::vector< BW::string > NodeVector;

	VisualEnvelope();
	~VisualEnvelope();

	bool init( Skin& skin, Mesh& mesh );

	virtual bool save( DataSectionPtr spVisualSection, DataSectionPtr spExistingVisual, const BW::string& primitiveFile,
		bool useIdentifier );

	virtual bool savePrimXml( const BW::string& xmlFile );

	virtual bool isVisualEnvelope() { return true; }

	size_t boneNodesCount() { return boneNodes_.size(); }

	template<class T>
	void copyVertex( const BloatVertex& inVertex, T& outVertex ) const;
	
	template<class T>
	void copyVerts( BVVector& inVerts, BW::vector<T>& outVerts );

	bool		split( size_t boneCount, BW::vector<VisualEnvelopePtr>& splitEnvelopes );


private:
#pragma pack(push, 1)

	//TODO: use the Moo vertex formats..
	struct VertexXYZNUVIIIWW
	{
		float pos[3];
		uint32 normal;
		float uv[2];
		uint8  index;
		uint8  index2;
		uint8  index3;
		uint8  weight;
		uint8  weight2;
	};

	struct VertexXYZNUVIIIWWTB
	{
		float pos[3];
		uint32 normal;
		float uv[2];
		uint8  index;
		uint8  index2;
		uint8  index3;
		uint8  weight;
		uint8  weight2;
		uint32 tangent;
		uint32 binormal;
	};
#pragma pack(pop)

	typedef BW::vector<VertexXYZNUVIIIWW> BlendedVertexVector;

	typedef BW::vector<VertexXYZNUVIIIWWTB> TBBlendedVertexVector;

	template<class T> void normaliseBoneWeights(int vertexIndex, T& v);
	void		createVertexList( BlendedVertexVector& vertices );
	void		createVertexList( TBBlendedVertexVector& vertices );

	bool		collectInitialTransforms( Skin& skin );
	void		normaliseInitialTransforms();
	void		initialPoseVertices();
	void		relaxedPoseVertices();

	NodeVector	boneNodes_;

	typedef BW::vector< BoneVertex > BoneVVector;
	BoneVVector		boneVertices_;

	typedef BW::vector< matrix4<float> >	MatrixVector;
	MatrixVector	initialTransforms_;

	matrix4<float>	initialObjectTransform_;

	VisualEnvelope( const VisualEnvelope& );
	VisualEnvelope& operator=( const VisualEnvelope& );
};


/**
 *	Templated function for assigning and normalising bone indices and weights.
 *
 *	@param	vertexIndex	The index of the bone vertex.
 *	@param	v			The vertex being assigned bone indices and weights.
 */
template<class T>
void VisualEnvelope::normaliseBoneWeights(int vertexIndex, T& v)
{
	// Check that the weights are in range
	MF_ASSERT(
		boneVertices_[ vertexIndex ].weight1 >= 0.0f &&
		boneVertices_[ vertexIndex ].weight1 <= 1.0f &&
		boneVertices_[ vertexIndex ].weight2 >= 0.0f &&
		boneVertices_[ vertexIndex ].weight2 <= 1.0f &&
		boneVertices_[ vertexIndex ].weight3 >= 0.0f &&
		boneVertices_[ vertexIndex ].weight3 <= 1.0f &&
		"VisualEnvelope::createVertexList - Bone weights outside [0..1] range!" );

	// Check that a valid bone is being assigned.  If a bone is not valid, set
	// the index to the root node and set the weighting to zero
	if( boneVertices_[ vertexIndex ].index1 >= (int)boneNodes_.size() )
	{
		v.index = 0;
		boneVertices_[ vertexIndex ].weight1 = 0.0f;

		float tempTotal =
			boneVertices_[ vertexIndex ].weight2 +
			boneVertices_[ vertexIndex ].weight3;
		if (tempTotal > 0.0f)
			boneVertices_[ vertexIndex ].weight2 /= tempTotal;

		boneVertices_[ vertexIndex ].weight3 =
			1.0f - boneVertices_[ vertexIndex ].weight2;
	}
	else
		v.index = boneVertices_[ vertexIndex ].index1;

	if( boneVertices_[ vertexIndex ].index2 >= (int)boneNodes_.size() )
	{
		v.index2 = 0;
		boneVertices_[ vertexIndex ].weight2 = 0.0f;

		float tempTotal = boneVertices_[ vertexIndex ].weight1 +
			boneVertices_[ vertexIndex ].weight3;
		if (tempTotal > 0.0f)
			boneVertices_[ vertexIndex ].weight1 /= tempTotal;

		boneVertices_[ vertexIndex ].weight3 =
			1.0f - boneVertices_[ vertexIndex ].weight1;
	}
	else
		v.index2 = boneVertices_[ vertexIndex ].index2;

	if( boneVertices_[ vertexIndex ].index3 >= (int)boneNodes_.size() )
	{
		v.index3 = 0;
		boneVertices_[ vertexIndex ].weight3 = 0.0f;

		float tempTotal = boneVertices_[ vertexIndex ].weight1 +
			boneVertices_[ vertexIndex ].weight2;
		if (tempTotal > 0.0f)
			boneVertices_[ vertexIndex ].weight1 /= tempTotal;

		boneVertices_[ vertexIndex ].weight2 =
			1.0f - boneVertices_[ vertexIndex ].weight1;
	}
	else
		v.index3 = boneVertices_[ vertexIndex ].index3;

	// If all bones are out of range, set weight1 to 1.0f and all other
	// weights to zero
	if (v.index == 0 && v.index2 == 0 && v.index3 == 0)
	{
		boneVertices_[ vertexIndex ].weight1 = 1.0f;
		boneVertices_[ vertexIndex ].weight2 = 0.0f;
		boneVertices_[ vertexIndex ].weight3 = 0.0f;
	}

	// When calculating the integer weightings, we determine the remainders
	// for each weight.
	float weight1Rem = 255.f * boneVertices_[ vertexIndex ].weight1 -
						(int)(255.f * boneVertices_[ vertexIndex ].weight1);
	float weight2Rem = 255.f * boneVertices_[ vertexIndex ].weight2 -
						(int)(255.f * boneVertices_[ vertexIndex ].weight2);
	float weight3Rem = 255.f * boneVertices_[ vertexIndex ].weight3 -
						(int)(255.f * boneVertices_[ vertexIndex ].weight3);

	// There are three possible sums:
	//
	//		1)	W1remainder + W2remainder + W3remainder = 0.0f
	//
	//		2)	W1remainder + W2remainder + W3remainder = 1.0f
	//
	//		3)	W1remainder + W2remainder + W3remainder = 2.0f
	//
	//		4)	W1remainder + W2remainder + W3remainder = 3.0f
	//
	// Note that case 4) is actually case one suffering from round off error
	//
	// Case 1)
	if ( weight1Rem + weight2Rem + weight3Rem < 0.1f )
	{
		v.weight = (uint8) (255.f * boneVertices_[ vertexIndex ].weight1);
		v.weight2 = (uint8) (255.f * boneVertices_[ vertexIndex ].weight2);

		// Sanity check
		MF_ASSERT(
			v.weight +
			v.weight2 +
			(uint8) (255.f * boneVertices_[ vertexIndex ].weight3)
				== 255);
	}
	// If 2) find the largest of the three remanders and round it up.  Round
	// the other 2 remainders down
	else if ( weight1Rem + weight2Rem + weight3Rem < 1.1f )
	{
		if ( weight1Rem > weight2Rem )
		{
			if ( weight1Rem > weight3Rem )
			{
				v.weight = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight1) + 1.0f);
				v.weight2 = (uint8) (255.f * boneVertices_[ vertexIndex ].weight2);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) (255.f * boneVertices_[ vertexIndex ].weight3)
						== 255);
			}
			else
			{
				v.weight = (uint8) (255.f * boneVertices_[ vertexIndex ].weight1);
				v.weight2 = (uint8) (255.f * boneVertices_[ vertexIndex ].weight2);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) ((255.f * boneVertices_[ vertexIndex ].weight3) + 1.0f)
						== 255);
			}
		}
		else
		{
			if ( weight2Rem > weight3Rem )
			{
				v.weight = (uint8) (255.f * boneVertices_[ vertexIndex ].weight1);
				v.weight2 = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight2) + 1.0f);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) (255.f * boneVertices_[ vertexIndex ].weight3)
						== 255);
			}
			else
			{
				v.weight = (uint8) (255.f * boneVertices_[ vertexIndex ].weight1);
				v.weight2 = (uint8) (255.f * boneVertices_[ vertexIndex ].weight2);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) ((255.f * boneVertices_[ vertexIndex ].weight3) + 1.0f)
						== 255);
			}
		}
	}
	// If 3) find the smallest of the three remanders and round it down.
	// Round the other 2 remainders up
	else if ( weight1Rem + weight2Rem + weight3Rem < 2.1f )
	{
		if ( weight1Rem < weight2Rem )
		{
			if ( weight1Rem < weight3Rem )
			{
				v.weight = (uint8) (255.f * boneVertices_[ vertexIndex ].weight1);
				v.weight2 = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight2) + 1.0f);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) ((255.f * boneVertices_[ vertexIndex ].weight3) + 1.0f)
						== 255);
			}
			else
			{
				v.weight = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight1) + 1.0f);
				v.weight2 = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight2) + 1.0f);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) (255.f * boneVertices_[ vertexIndex ].weight3)
						== 255);
			}
		}
		else
		{
			if ( weight2Rem < weight3Rem )
			{
				v.weight = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight1) + 1.0f);
				v.weight2 = (uint8) (255.f * boneVertices_[ vertexIndex ].weight2);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) ((255.f * boneVertices_[ vertexIndex ].weight3) + 1.0f)
						== 255);
			}
			else
			{
				v.weight = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight1) + 1.0f);
				v.weight2 = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight2) + 1.0f);

				// Sanity check
				MF_ASSERT(
					v.weight +
					v.weight2 +
					(uint8) (255.f * boneVertices_[ vertexIndex ].weight3)
						== 255);
			}
		}
	}
	// Case 4), which is the same as case 1 above
	else if ( weight1Rem + weight2Rem + weight3Rem < 3.1f )
	{
		v.weight = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight1) + 1.0f);
		v.weight2 = (uint8) ((255.f * boneVertices_[ vertexIndex ].weight2) + 1.0f);

		// Sanity check
		MF_ASSERT(
			v.weight +
			v.weight2 +
			(uint8) ((255.f * boneVertices_[ vertexIndex ].weight3) + 1.0f)
				== 255);
	}

	v.index *= 3;
	v.index2 *= 3;
	v.index3 *= 3;
}

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "visual_envelope.ipp"
#endif

#endif // VISUAL_ENVELOPE_HPP
