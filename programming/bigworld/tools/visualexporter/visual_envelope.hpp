#ifndef VISUAL_ENVELOPE_HPP
#define VISUAL_ENVELOPE_HPP

#include "mfxnode.hpp"
#include "phyexp.h"
#include "visual_mesh.hpp"

// forward declarations from max sdk.
class ISkin;

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
	typedef BW::vector< INode* > NodeVector;

	VisualEnvelope();
	~VisualEnvelope();

	bool init( INode* node, MFXNode* root );

	virtual bool save(
		DataSectionPtr pVisualSection,
		DataSectionPtr spExistingVisual,
		const BW::string& primitiveFile,
		bool useIdentifier );
	virtual bool savePrimXml( const BW::string& xmlFile );
	virtual void add( VisualMesh & destMesh, int forceMeshIndex = -1 ) { }
	virtual bool isVisualEnvelope() { return true; }

	bool split( size_t boneCount, BW::vector<VisualEnvelopePtr>& splitEnvelopes );

	size_t boneNodesCount() { return boneNodes_.size(); }

	struct BoneVertex
	{
		BoneVertex( const Point3& position, int index, int index2 = 0, int index3 = 0, float weight = 1, float weight2 = 0, float weight3 = 0 )
			: position_( position ),
			boneIndex_( index ),
			boneIndex2_( index2 ),
			boneIndex3_( index3 ),
			weight_( weight ),
			weight2_( weight2 ),
			weight3_( weight3 )
		{ }
		Point3	position_;
		int		boneIndex_;
		int		boneIndex2_;
		int		boneIndex3_;
		float	weight_;
		float	weight2_;
		float	weight3_;
	};

private:
#pragma pack(push, 1)
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

	int			boneIndex( INode* node );
	void		createVertexList( BlendedVertexVector& vertices );
	void		createVertexList( TBBlendedVertexVector& vertices );

	void		getScaleValues( BW::vector< Point3 > & scales );
	void		scaleBoneVertices( BW::vector< Point3 >& scales );
	bool		collectInitialTransforms(IPhysiqueExport* phyExport);
	bool		collectInitialTransforms(ISkin* pSkin);
	void		normaliseInitialTransforms();
	void		initialPoseVertices();
	void		relaxedPoseVertices();

	NodeVector	boneNodes_;

	typedef BW::vector< BoneVertex > BoneVVector;
	BoneVVector		boneVertices_;

	typedef BW::vector< Matrix3 >	MatrixVector;
	MatrixVector	initialTransforms_;

	Matrix3			initialObjectTransform_;

	BW::string originalIdentifier_;

	VisualEnvelope( const VisualEnvelope& );
	VisualEnvelope& operator=( const VisualEnvelope& );
};

BW_END_NAMESPACE

#endif // VISUAL_ENVELOPE_HPP
