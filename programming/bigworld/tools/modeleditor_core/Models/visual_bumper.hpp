#ifndef VISUAL_BUMPER_HPP
#define VISUAL_BUMPER_HPP

#include "resmgr/quick_file_writer.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/index_buffer.hpp"
#include "moo/primitive_file_structs.hpp"
#include "moo/vertex_streams.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class is used to add tangent and binormal information to models,
 * that do not have this information for their vertices.
 * It modifies the on disk state of the primitive file. Models cannot be
 * unbumped.
 *
 * TODO: Needs to be integrated with visual loading.
 */
class VisualBumper
{
public:
	VisualBumper( DataSectionPtr pVisual, const BW::string& primitivesName );
	
	bool bumpVisual();

private:

	typedef BW::vector< Moo::PrimitiveGroup > PGVector;

	template< typename BumpedVertexType >
	bool savePrimitiveFile( const BW::vector< BumpedVertexType >& bumpedVertices,
							const BW::vector<Moo::VertexStream>& streams,
							const Moo::IndicesHolder& newIndices, 
							const PGVector& newPrimGroups );

	template< typename VertexType, typename BumpedVertexType >
	bool makeBumped( const VertexType* pVertices, int nVertices, 
					 BW::vector<Moo::VertexStream>& streams,
					 const Moo::IndicesHolder& indices, 
					 const PGVector& primGroups );


	void createPrimitiveGroups( size_t nBumpedVertices, 
								const BW::vector< uint32 >& bumpedIndices, 
								const BW::vector< size_t >& material, 
								PGVector& newPrimGroups, 
								Moo::IndicesHolder& newIndices );

	bool updatePrimitives();

	BW::string vertGroup_;
	BW::vector<BW::string> streams_;
	BW::string indicesGroup_;
	BW::string format_;
	
	DataSectionPtr pVisual_;
	DataSectionPtr pPrimFile_;
	BW::string primitivesName_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "visual_bumper.ipp"
#endif

#endif // VISUAL_BUMPER_HPP
