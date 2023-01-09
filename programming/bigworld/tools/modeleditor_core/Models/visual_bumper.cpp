#include "pch.hpp"

#include "nvmeshmender.h"

#include "visual_bumper.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"

#ifndef CODE_INLINE
#include "visual_bumper.ipp"
#endif

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
VisualBumper::VisualBumper( DataSectionPtr pVisual, const BW::string& primitivesName ):
	vertGroup_(""),
	indicesGroup_(""),
	format_(""),
	pVisual_( pVisual ),
	pPrimFile_( NULL ),
	primitivesName_( primitivesName )
{
}


/**
 *	This method will iterate through the model's rendersets and geometries 
 *	and bump them if they are not already bumped.
 *
 *	@return Whether the vertices have been bumped.
 */
bool VisualBumper::bumpVisual() 
{
	BW_GUARD;

	pPrimFile_ = PrimitiveFile::get( primitivesName_ );
	if (!pPrimFile_)
	{
		pPrimFile_ = new BinSection( primitivesName_, new BinaryBlock( 0, 0, "BinaryBlock/VisualBumper" ) );
	}

	bool beenBumped = false;
	// iterate through our rendersets
	BW::vector< DataSectionPtr >	pRenderSets;
	pVisual_->openSections( "renderSet", pRenderSets );
	BW::vector< DataSectionPtr >::iterator renderSetsIt = pRenderSets.begin();
	BW::vector< DataSectionPtr >::iterator renderSetsEnd = pRenderSets.end();
	while (renderSetsIt != renderSetsEnd)
	{
		DataSectionPtr renderSet = *renderSetsIt++;

		// iterate through our geometries
		BW::vector< DataSectionPtr >	pGeometries;
		renderSet->openSections( "geometry", pGeometries );
		BW::vector< DataSectionPtr >::iterator geometriesIt = pGeometries.begin();
		BW::vector< DataSectionPtr >::iterator geometriesEnd = pGeometries.end();
		while (geometriesIt != geometriesEnd)
		{
			DataSectionPtr geometries = *geometriesIt++;

			// Get the vertex group's name
			vertGroup_ = geometries->readString( "vertices", "" );
			if (vertGroup_.find_first_of( '/' ) < vertGroup_.size())
			{
				BW::string fileName, partName;
				splitOldPrimitiveName( vertGroup_, fileName, partName );
				vertGroup_ = partName;
			}

			// Get the vertex group's stream info
			streams_.clear();
			geometries->readStrings( "stream", streams_ );
			for (uint i=0; i<streams_.size(); i++)
			{
				if (streams_[i].find_first_of( '/' ) < streams_[i].size())
				{
					BW::string fileName, partName;
					splitOldPrimitiveName( streams_[i], fileName, partName );
					streams_[i] = partName;
				}
			}

			// get the index groups's name
			indicesGroup_ = geometries->readString( "primitive", "" );
			if (indicesGroup_.find_first_of( '/' ) < indicesGroup_.size())
			{
				BW::string fileName, partName;
				splitOldPrimitiveName( indicesGroup_, fileName, partName );
				indicesGroup_ = partName;
			}

			beenBumped |= this->updatePrimitives();
		}
	}

	if (beenBumped)
	{
		pPrimFile_->save();
	}

	return beenBumped;
}

/**
 *	This method saves the primitive file with the given vertex,
 *	index and primitive group information.
 *
 *	@param bumpedVertices The vertices with the added tangent
 *						  and binormal information.
 *	@param newIndices	  The updated indices.
 *	@param newPrimGroups  The updated primitive groups.
 *
 *	@return Whether the save was successful.
 */
template< typename BumpedVertexType >
bool VisualBumper::savePrimitiveFile( 
					const BW::vector< BumpedVertexType >& bumpedVertices,
					const BW::vector<Moo::VertexStream>& streams,
					const Moo::IndicesHolder& newIndices, 
					const PGVector& newPrimGroups )
{
	BW_GUARD;

	if (pPrimFile_)
	{
		// save it out to the file
		// The entry size checks to see how many bytes of information
		// is required by each index. != 2 means it is not 2 bytes,
		// not 16 bit therefore is 32 bit format.
		bool isFormat32 = (newIndices.entrySize() != 2);
		
		Moo::IndexHeader ih;
		bw_snprintf( ih.indexFormat_, sizeof( ih.indexFormat_ ), isFormat32 ? "list32" : "list" );
		ih.nIndices_ = newIndices.size();
		ih.nTriangleGroups_ = static_cast<int>(newPrimGroups.size());

		QuickFileWriter qfw;
		qfw << ih;
		qfw.write( newIndices.indices(), newIndices.size() * newIndices.entrySize() );
		qfw << newPrimGroups;
		pPrimFile_->writeBinary( indicesGroup_, qfw.output() );

		qfw = QuickFileWriter();
		Moo::VertexHeader vh;
		bw_snprintf( vh.vertexFormat_, sizeof( vh.vertexFormat_ ), ( format_ + "tb" ).c_str() );
		vh.nVertices_ = static_cast<int>(bumpedVertices.size());
		qfw << vh;
		qfw << bumpedVertices;

		if (pPrimFile_->writeBinary( vertGroup_, qfw.output() ))
		{
			// write out the stream info...
			MF_ASSERT( streams_.size() == streams.size() );
			for (size_t i=0; i<streams_.size(); ++i)
			{
				pPrimFile_->writeBinary( streams_[i], streams[i].data() );
			}
			
			return true;
		}
	}

	return false;
}


/**
 *	This method adds tangent and binormal information to the given vertex,
 *	index and primitive group information. It saves the updated primitive file
 *	directly to disk.
 *
 *	@param vertices			The unbumped vertices.
 *	@param nVertices		The number of vertices there are.
 *	@param indices			The unbumped indices.
 *	@param primGroups		The unbumped primitive groups.
 *
 *	@return Whether the bumping was successful.
 */
template< typename VertexType, typename BumpedVertexType >
bool VisualBumper::makeBumped( const VertexType* pVertices, int nVertices, 
						BW::vector<Moo::VertexStream>& streams,
						const Moo::IndicesHolder& indices, 
						const PGVector& primGroups )
{
	BW_GUARD;

	BW::vector< BumpedVertexType > bumpedVertices;
	BW::vector< uint32 > bumpedIndices;
	BW::vector< size_t > material;
	size_t materialIndex = 0;

	int curStreamStartIndex( 0 );

	// bump the primitive groups individually
	for (size_t i = 0; i < primGroups.size(); ++i)
	{
		Moo::PrimitiveGroup pg = primGroups[ i ];

		if (i==0)
		{
			curStreamStartIndex = pg.startVertex_;
		}

		BW::vector< MeshMender::Vertex > theVerts;
		// nvmeshmender.h requires unsigned int
		BW::vector< unsigned int > theIndices;
		BW::vector< unsigned int > mappingNewToOld;
		
		// collect the vertices information
		for (size_t j = pg.startVertex_; j < (size_t)(pg.startVertex_ + pg.nVertices_); ++j)
		{
			const VertexType& vert = pVertices[ j ];

			MeshMender::Vertex v;
			v.pos = vert.pos_;
			v.normal = unpackTheNormal( vert.normal_ );
			v.s = vert.uv_.x;
			v.t = 1.f - vert.uv_.y;
			theVerts.push_back( v );
		}

		// collect the indices information ( make sure you remove the offset ). 
		// Multiply by 3 as we are dealing in triangles.
		for (int j = pg.startIndex_; j < pg.startIndex_ + pg.nPrimitives_ * 3; ++j)
		{
			theIndices.push_back(  indices[ j ] - pg.startVertex_ );
		}
		
		// Refer to nvmeshmender.h
		// theVerts = The vertices to be bumped (size may change)
		// theIndices = The indices to be bumped
		// mappingNewToOld = Will contain a mapping of newVertexIndex -> oldVertexIndex
		// 0.5f = min. cosine of angle between normals (ignored)
		// 0.5f = min. cosine of angle between tangens 
		// 0.5f = min. cosine of angle between binormals
		// 0 = an amount to blend the normalised face normal,
		//		and unnormalised face normal together.
		// MeshMender::DONT_CALCULATE_NORMALS = do not calc. normals
		MeshMender mender;
		mender.Mend( theVerts, theIndices, mappingNewToOld,
			0.5f, 0.5f, 0.5f, 
			0,
			MeshMender::DONT_CALCULATE_NORMALS );
		
		// new vector as we still need to copy the extra data from verts
		BW::vector< BumpedVertexType > newVerts; 
		for (size_t j = 0; j < theVerts.size(); ++j)
		{
			const MeshMender::Vertex& vert = theVerts[ j ];

			BumpedVertexType bv;
			bv.pos_.x = vert.pos.x;
			bv.pos_.y = vert.pos.y;
			bv.pos_.z = vert.pos.z;
			bv.normal_ = Moo::packNormal( Vector3( vert.normal.x, vert.normal.y, vert.normal.z ) );
			bv.tangent_ = Moo::packNormal( Vector3( vert.tangent.x, vert.tangent.y, vert.tangent.z ) );
			bv.binormal_ = Moo::packNormal( Vector3( vert.binormal.x, vert.binormal.y, vert.binormal.z ) );
			bv.uv_.x = vert.s;
			bv.uv_.y = 1.f - vert.t;
			copyVertsExtra( pVertices[ pg.startVertex_ + mappingNewToOld[ j ] ], bv );
			newVerts.push_back( bv );
		}

		// update the streams...
		for (size_t j=0; j<streams.size(); ++j)
		{
			streams[j].remapVertices( curStreamStartIndex, pg.nVertices_, mappingNewToOld );
		}
		curStreamStartIndex += static_cast<int>(theVerts.size());

		// restore the offset for the indices (because they may be from a 
		// different primGroup), since we appended vertices to the new list, 
		// the startVertex would be where the last primGroup ends
		// i.e. bumpedVertices.size()
		for (size_t j = 0; j < theIndices.size(); ++j)
		{
			theIndices[ j ] += static_cast<uint>(bumpedVertices.size());
		}

		bumpedVertices.insert( bumpedVertices.end(), newVerts.begin(), newVerts.end() );
		bumpedIndices.insert( bumpedIndices.end(), theIndices.begin(), theIndices.end() );
		// need to remember which indices belong to which primGroup
		materialIndex += theIndices.size();
		material.push_back( materialIndex );
	}

	// create the primitive groups
	PGVector newPrimGroups;
	Moo::IndicesHolder newIndices;
	this->createPrimitiveGroups( bumpedVertices.size(), bumpedIndices, material, newPrimGroups, newIndices );
	return this->savePrimitiveFile< BumpedVertexType >( bumpedVertices, streams, newIndices, newPrimGroups );
}


/**
 *	This method creates the primitive groups out of the given bumped 
 *	indices. 
 *	
 *	@param nBumpedVertices	The number of bumped vertices there are.
 *	@param bumpedIndices	The vector of the new indices that have been 
 *							bumped.
 *	@param material			This is used to inform the function which 
 *							parts of the bumpedIndices belong to which 
 *							primitive group, it is basically a seperator 
 *							for the vector.
 *	@param newPrimGroups	A reference to a vector of primitive groups. 
 *							This information is created in this function.
 *	@param newIndices		A reference to the IndicesHolder that needs to 
 *							be filled with the bumped indices information.
 */
void VisualBumper::createPrimitiveGroups( size_t nBumpedVertices, 
									const BW::vector< uint32 >& bumpedIndices, 
									const BW::vector< size_t >& material, 
									PGVector& newPrimGroups, 
									Moo::IndicesHolder& newIndices )
{
	BW_GUARD;

	if (bumpedIndices.size())
	{
		uint32 firstVertex = bumpedIndices.front();
		uint32 lastVertex = firstVertex;
		int firstTriangle = 0;

		size_t materialIndex = 0;
		newIndices.setSize( static_cast<uint32>(bumpedIndices.size()), 
						Moo::IndicesReference::bestFormat( static_cast<uint32>(nBumpedVertices )) );

		for (size_t i = 0; i <= bumpedIndices.size(); i+=3)
		{
			// If we have reached the last index for this primitive
			// group then create a new primitive group
			if (i == material[materialIndex]) 
			{
				Moo::PrimitiveGroup pg;
				pg.startVertex_ = firstVertex;
				pg.nVertices_ = lastVertex - firstVertex + 1;
				pg.startIndex_ = firstTriangle * 3;
				// the number of triangles minus the offset
				pg.nPrimitives_ = static_cast<int>(i / 3 - firstTriangle);
				newPrimGroups.push_back( pg );	

				// We created a primitive group but there is more to go
				if (i < bumpedIndices.size()) 
				{
					firstVertex = bumpedIndices[ i ];
					lastVertex = firstVertex;
					firstTriangle = static_cast<int>(i / 3);
					materialIndex++;
				}
			}

			// Update indices
			if (i < bumpedIndices.size())
			{
				const int32 index = static_cast<int32>(i);
				newIndices.set( index  , bumpedIndices[ i   ] );
				newIndices.set( index+1, bumpedIndices[ i+1 ] );
				newIndices.set( index+2, bumpedIndices[ i+2 ] );
				firstVertex = min( firstVertex, min( bumpedIndices[ i ], min( bumpedIndices[ i+1 ], bumpedIndices[ i+2 ] ) ) );
				lastVertex  = max( lastVertex,  max( bumpedIndices[ i ], max( bumpedIndices[ i+1 ], bumpedIndices[ i+2 ] ) ) );
			}
		}
	}
}


/**
 *	This method reads in the vertex information and determines if this vertex section
 *	needs to be bumped.
 *	It will then initiate the bumper with the correct vertex format, i.e. the extra vertex
 *	information depending on the vertex format will be copied to the new bumped vertices.
 *
 *	@return Whether the vertices have been bumped.
 */
bool VisualBumper::updatePrimitives() 
{
	BW_GUARD;

	// get the vertices information
	BinaryPtr pVertices = pPrimFile_->readBinary( vertGroup_ );
	if (!pVertices)
	{
		return false;
	}

	const Moo::VertexHeader* vh = reinterpret_cast< const Moo::VertexHeader* >( pVertices->data() );
	
	// make sure the model isnt already bumped
	format_ = BW::string( vh->vertexFormat_ );
	if (format_.find("tb") != format_.npos)
	{
		return false;
	}

	BW::vector<Moo::VertexStream> streams;
	for (uint i=0; i<streams_.size(); i++)
	{
		BinaryPtr pStream = pPrimFile_->readBinary( streams_[i] );		
		if (pStream)
		{
			BW::string::size_type pos = streams_[i].find_last_of('.');
			BW::string type;
			if (pos == BW::string::npos)
				type = streams_[i];
			else
				type = streams_[i].substr(pos+1);

			if (type == Moo::UV2Stream::TYPE_NAME)
			{
				Moo::VertexStream s = 
					Moo::VertexStream::create<Moo::UV2Stream>(pStream, vh->nVertices_);
				streams.push_back( s );
			}
			else if (type == Moo::ColourStream::TYPE_NAME)
			{
				Moo::VertexStream s = 
					Moo::VertexStream::create<Moo::ColourStream>(pStream, vh->nVertices_);
				streams.push_back( s );
			}
		}
	}

	// get the indices (make sure you convert it to the correct format type)
	BinaryPtr pIndices = pPrimFile_->readBinary( indicesGroup_ );
	if (!pIndices)
	{
		return false;
	}

	const Moo::IndexHeader* ih = ( const Moo::IndexHeader* )pIndices->data();
	Moo::IndicesHolder sourceIndices;
	if (BW::string( ih->indexFormat_ ) == BW::string( "list" ))
	{
		sourceIndices.assign( ( ih + 1 ), ih->nIndices_, D3DFMT_INDEX16 );
	}
	else
	{
		sourceIndices.assign( ( ih + 1 ), ih->nIndices_, D3DFMT_INDEX32 );
	}

	// get the primitive groups
	const Moo::PrimitiveGroup* pgs = reinterpret_cast< const Moo::PrimitiveGroup* >( 
		(unsigned char*)( ih + 1 ) + ih->nIndices_ * sourceIndices.entrySize() );
	MF_ASSERT( pgs && "VisualBumper::updatePrimitives found a broken primitive group" );

	PGVector primGroups;
	primGroups.assign( pgs, pgs + ih->nTriangleGroups_ );

	// pass the information read from the file to the actual bumper
	bool beenBumped = false;

	if (Moo::VertexFormatCache::getFormatName<Moo::VertexXYZNUV>().equals(format_))
	{
		Moo::VertexXYZNUV* verts = (Moo::VertexXYZNUV*)(vh + 1);
		beenBumped = this->makeBumped< Moo::VertexXYZNUV, Moo::VertexXYZNUVTB >( 
			verts, vh->nVertices_, streams, sourceIndices, primGroups );
	}
	else if (Moo::VertexFormatCache::getFormatName<Moo::VertexXYZNUVIIIWW>().equals(format_))
	{
		Moo::VertexXYZNUVIIIWW* verts = (Moo::VertexXYZNUVIIIWW*)(vh + 1);
		beenBumped = this->makeBumped< Moo::VertexXYZNUVIIIWW, Moo::VertexXYZNUVIIIWWTB >( 
			verts, vh->nVertices_, streams, sourceIndices, primGroups );
	}
	return beenBumped;
}

 BW_END_NAMESPACE

