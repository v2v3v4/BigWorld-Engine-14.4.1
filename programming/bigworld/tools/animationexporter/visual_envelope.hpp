#ifndef VISUAL_ENVELOPE_HPP
#define VISUAL_ENVELOPE_HPP

#include "cstdmf/bw_string.hpp"
#include "mfxnode.hpp"
#include "phyexp.h"
#include "visual_mesh.hpp"

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
	VisualEnvelope();
	~VisualEnvelope();

	bool init( INode* node, MFXNode* root );
	void save( DataSectionPtr spVisualSection, const BW::string& primitiveFile );

private:
	struct VertexXYZNUVI
	{
		float pos[3];
		float normal[3];
		float uv[2];
		float index;
	};

	typedef BW::vector<VertexXYZNUVI> VertexVector;

	int			boneIndex( INode* node );
	void		createVertexList( VertexVector& vertices );

	void		getScaleValues( BW::vector< Point3 > & scales );
	void		scaleBoneVertices( BW::vector< Point3 >& scales );
	bool		collectInitialTransforms(IPhysiqueExport* phyExport);
	void		normaliseInitialTransforms();
	void		initialPoseVertices();
	void		relaxedPoseVertices();

	typedef BW::vector< INode* > NodeVector;
	NodeVector	boneNodes_;

	struct BoneVertex
	{
		BoneVertex( const Point3& position, int index )
		: position_( position ),
		  boneIndex_( index )
		{ }
		Point3	position_;
		int		boneIndex_;
	};

	typedef BW::vector< BoneVertex > BoneVVector;
	BoneVVector		boneVertices_;

	typedef BW::vector< Matrix3 >	MatrixVector;
	MatrixVector	initialTransforms_;

	Matrix3			initialObjectTransform_;


	VisualEnvelope( const VisualEnvelope& );
	VisualEnvelope& operator=( const VisualEnvelope& );
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "visual_envelope.ipp"
#endif

#endif // VISUAL_ENVELOPE_HPP
