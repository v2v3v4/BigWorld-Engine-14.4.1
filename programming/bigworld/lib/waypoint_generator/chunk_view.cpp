#include "pch.hpp"

#include "chunk_view.hpp"

#include "waypoint/waypoint.hpp"

#include "math/lineeq.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"

#include "chunk/chunk.hpp"

DECLARE_DEBUG_COMPONENT2( "WayPoint", 0 )


BW_BEGIN_NAMESPACE

ChunkView::ChunkView()
	:
	nextID_(),
	minMaxValid_()
{
	BW_GUARD;

	this->clear();
}

ChunkView::~ChunkView()
{
	BW_GUARD;

	this->clear();
}

void ChunkView::clear()
{
	BW_GUARD;

	minMaxValid_ = false;
	nextID_ = 1;
	polygons_.clear();
}


bool ChunkView::load( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk == NULL) 
	{
		WARNING_MSG( "ChunkView::load: pChunk == 0\n" );
		return false;
	}

	DataSectionPtr pSection;
	DataSection::iterator it;

	this->clear();

	//pSection = XMLSection::createFromFile(filename.c_str());
	//
	//if(!pSection)
	//	return false;
	pSection = BWResource::openSection( pChunk->resourceID() );

	// load legacy waypointSets.
	BW::vector<DataSectionPtr>	wpSets;
	pSection->openSections( "waypointSet", wpSets );
	for (uint i = 0; i < wpSets.size(); i++)
	{
		int set = wpSets[i]->asInt();
		if (int(sets_.size()) <= set) sets_.resize( set+1 );
		sets_[set].girth = wpSets[i]->readFloat( "girth" );
		for (DataSectionIterator it = wpSets[i]->begin();
			it != wpSets[i]->end();
			it++)
		{
			DataSectionPtr pDS = *it;
			if (pDS->sectionName() == "waypoint")
				this->parseNavPoly( pDS, set );
		}
	}

	// load legacy navPolys
	BW::vector<DataSectionPtr>	npSets;
	pSection->openSections( "navPolySet", npSets );
	for (uint i = 0; i < npSets.size(); i++)
	{
		int set = npSets[i]->asInt();
		if (int(sets_.size()) <= set) sets_.resize( set+1 );
		sets_[set].girth = npSets[i]->readFloat( "girth" );
		for (DataSectionIterator it = npSets[i]->begin();
			it != npSets[i]->end();
			it++)
		{
			DataSectionPtr pDS = *it;
			if (pDS->sectionName() == "navPoly")
				this->parseNavPoly( pDS, set, pChunk );
		}
	}

	// now load binary navmesh
	pSection = BWResource::openSection( pChunk->binFileName() );
	BinaryPtr pNavmesh = NULL;
	if (pSection) pNavmesh = pSection->readBinary( "worldNavmesh" );
	if (pNavmesh)
	{
		Matrix tr = Matrix::identity;

		const char * dataBeg = pNavmesh->cdata();
		const char * dataEnd = dataBeg + pNavmesh->len();
		const char * dataPtr = dataBeg;

		int setIdx = 0;
		while (dataPtr < dataEnd)
		{
			int aVersion = *((int*&)dataPtr)++;
			float aGirth = *((float*&)dataPtr)++;
			int navPolyElts = *((int*&)dataPtr)++;
			int navPolyEdges = *((int*&)dataPtr)++;
			// navPolySetEltSize

			sets_.resize( setIdx+2 );
			sets_[setIdx+1].girth = aGirth;

			int polyOffset = nextID_-1;

			const char * edgePtr = dataPtr + navPolyElts * navPolyEltSize;
			for (int p = 0; p < navPolyElts; p++)
			{
				PolygonDef polyDef;
				polyDef.minHeight = *((float*&)dataPtr)++;
				polyDef.maxHeight = *((float*&)dataPtr)++;
				polyDef.set = setIdx+1;

				int vertexCount = *((int*&)dataPtr)++;
				// navPolyEltSize

				Vector3 inLocalFirst( 0, 0, 0 );
				for (int e = 0; e < vertexCount; e++)
				{
					Vector3 inLocal( 0, polyDef.maxHeight, 0 );
					inLocal.x =	*((float*&)edgePtr)++;
					inLocal.z = *((float*&)edgePtr)++;
					int adj = *((int*&)edgePtr)++;
					// navPolyEdgeEltSize

					VertexDef vertexDef;
					Vector3 inGlobal = tr.applyPoint( inLocal );
					vertexDef.position.x = inGlobal.x;
					vertexDef.position.y = inGlobal.z;

					if (uint(adj) < 65535)
					{
						vertexDef.adjacentID = adj+1 + polyOffset;
						vertexDef.adjToAnotherChunk = false;
					}
					else if (adj == 65535)
					{
						vertexDef.adjacentID = 0;
						vertexDef.adjToAnotherChunk = true;
					}
					else
					{
						vertexDef.adjacentID = -~adj;
						vertexDef.adjToAnotherChunk = false;
					}

					polyDef.vertices.push_back( vertexDef );
					if (e == 0) inLocalFirst = inLocal;

					if(!minMaxValid_)
					{
						gridMin_.x = vertexDef.position.x;
						gridMin_.z = vertexDef.position.y;
						gridMin_.y = -1000.0f;
						gridMax_.x = vertexDef.position.x;
						gridMax_.z = vertexDef.position.y;
						gridMax_.y = 1000.0f;
						minMaxValid_ = true;
					}
					else
					{
						if(vertexDef.position.x < gridMin_.x)
							gridMin_.x = vertexDef.position.x;
						if(vertexDef.position.y < gridMin_.z)
							gridMin_.z = vertexDef.position.y;
						if(vertexDef.position.x > gridMax_.x)
							gridMax_.x = vertexDef.position.x;
						if(vertexDef.position.y > gridMax_.z)
							gridMax_.z = vertexDef.position.y;
					}
				}

				inLocalFirst.y = polyDef.minHeight;
				polyDef.minHeight = tr.applyPoint( inLocalFirst ).y;
				inLocalFirst.y = polyDef.maxHeight;
				polyDef.maxHeight = tr.applyPoint( inLocalFirst ).y;

				// store at id nextID_ (in vector elt nextID_-1)
				polygons_.push_back( polyDef );
				nextID_++;
			}

			setIdx++;

			dataPtr = edgePtr;
		}
	}

	identifier_ = pChunk->identifier();

	// Now work out the min and max, and stuff.
	return true;
}

/**
 * Parses a navPoly / waypoint section 
 *
 * @param pSection  the DataSection to parse.
 * @param pChunk  if not NULL this flags that we are parsing a navPoly not a
 * waypoint section as well as providing the chunk from which to get the 
 * transform information to go from local -> global coords.
 */
void ChunkView::parseNavPoly( DataSectionPtr pSection, int set, Chunk * pChunk )
{
	BW_GUARD;

	DataSection::iterator it;
	PolygonDef polyDef;
	int id;
	float height;

	id = pSection->asInt();
	height = pSection->readFloat("height");
	polyDef.minHeight = pSection->readFloat("minHeight",height);
	polyDef.maxHeight = pSection->readFloat("maxHeight",height);
	polyDef.set = set;

	// If chunk provided, we want to convert heights from local -> global coords.
	if (pChunk)
	{
		Vector3 vMin( pChunk->centre().x, polyDef.minHeight, pChunk->centre().z );
		Vector3 vMax( pChunk->centre().x, polyDef.maxHeight, pChunk->centre().z );

		Vector3 vMinT = pChunk->transform().applyPoint( vMin );
		Vector3 vMaxT = pChunk->transform().applyPoint( vMax );

		polyDef.minHeight = vMinT.y;
		polyDef.maxHeight = vMaxT.y;
	}

	// If there is a gap in IDs, add blank polys to fill it up.
	while (nextID_ <= id)
	{
		PolygonDef blankPoly;
		blankPoly.minHeight = 0.0f;
		blankPoly.maxHeight = 0.0f;
		blankPoly.set = 0;
		polygons_.push_back(blankPoly);
		nextID_++;
	}

	// Read all this navPoly's vertices
	for(it = pSection->begin(); it != pSection->end(); it++)
	{
		if((*it)->sectionName() == "vertex")
		{
			DataSectionPtr pVertex = *it;
			VertexDef vertexDef;
			Vector3 vinfo = pVertex->asVector3();
			vertexDef.position.x = vinfo.x;
			vertexDef.position.y = vinfo.y;
			vertexDef.adjacentID = int( vinfo.z );

			// if pChunk provided, want to convert vertex to golbal coords.
			if (pChunk)
			{
				Vector3 v( vinfo.x, polyDef.maxHeight, vinfo.y );
				Vector3 vT = pChunk->transform().applyPoint( v );

				vertexDef.position.x = vT.x;
				vertexDef.position.y = vT.z;
			}

			BW::string notFound( "__NOT_FOUND__" );
			BW::string adjacentChunk = pVertex->readString("adjacentChunk",notFound);
			// if there is an adjacentChunk tag
			// note: only ever occurs within legacy waypoint tags.
			if ( adjacentChunk != notFound ) 
			{
				if (vertexDef.adjacentID != 0) 
				{
					WARNING_MSG( "ChunkView::parseNavPoly: adjacentChunk tag"
						" found but edge adjacency not 0.\n" );
				}
				vertexDef.adjacentID = CHUNK_ADJACENT_CONSTANT;
			}

			// if adjacent to another chunk, flag this in vertexDef.
			// also make (navPoly) adjacentID more sensible.
			vertexDef.adjToAnotherChunk = false;
			if (vertexDef.adjacentID == CHUNK_ADJACENT_CONSTANT) 
			{
				vertexDef.adjacentID = 0;
				vertexDef.adjToAnotherChunk = true;
			}

			polyDef.vertices.push_back( vertexDef );

			if(!minMaxValid_)
			{
				gridMin_.x = vertexDef.position.x;
				gridMin_.z = vertexDef.position.y;
				gridMin_.y = -1000.0f;
				gridMax_.x = vertexDef.position.x;
				gridMax_.z = vertexDef.position.y;
				gridMax_.y = 1000.0f;
				minMaxValid_ = true;
			}
			else
			{
				if(vertexDef.position.x < gridMin_.x)
					gridMin_.x = vertexDef.position.x;
				if(vertexDef.position.y < gridMin_.z)
					gridMin_.z = vertexDef.position.y;
				if(vertexDef.position.x > gridMax_.x)
					gridMax_.x = vertexDef.position.x;
				if(vertexDef.position.y > gridMax_.z)
					gridMax_.z = vertexDef.position.y;
			}
		}
	}

	polygons_[id-1] = polyDef;
}

uint32 ChunkView::gridX() const
{
	return uint32( (gridMax_.x - gridMin_.x) / this->gridResolution() );
}

uint32 ChunkView::gridZ() const
{
	return uint32( (gridMax_.z - gridMin_.z) / this->gridResolution() );
}

uint8 ChunkView::gridMask(int x, int z) const
{
	return 0;
}

Vector3 ChunkView::gridMin() const
{
	return gridMin_;
}

Vector3 ChunkView::gridMax() const
{
	return gridMax_;
}

float ChunkView::gridResolution() const
{
	// This is arbitrary.
	return 0.5f;
}

int ChunkView::getPolygonCount() const
{
	return static_cast<int>(polygons_.size());
}

int ChunkView::getBSPNodeCount() const
{
	return 0;
}

float ChunkView::getGirth( int set ) const
{
	BW_GUARD;

	return sets_[set].girth;
}

int ChunkView::getVertexCount(int polygon) const
{
	BW_GUARD;

	return static_cast<int>(polygons_[polygon].vertices.size());
}

float ChunkView::getMinHeight(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].minHeight;
}

float ChunkView::getMaxHeight(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].maxHeight;
}

int ChunkView::getSet(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].set;
}

int ChunkView::getSets() const
{
	BW_GUARD;

	return int( sets_.size() );
}

// Just to confuse things, id = polygon + 1
// So if we ask for polygon 0, we are asking for id 1.

void ChunkView::getVertex(int polygon, int vertex, 
	Vector2& v, int& adjacency, bool & adjToAnotherChunk) const
{
	BW_GUARD;

	const VertexDef & vertexDef = polygons_[polygon].vertices[vertex];

	v.x = (vertexDef.position.x - gridMin_.x) / this->gridResolution();
	v.y = (vertexDef.position.y - gridMin_.z) / this->gridResolution();
	adjacency = vertexDef.adjacentID;
	adjToAnotherChunk = vertexDef.adjToAnotherChunk;
}

void ChunkView::setAdjacency( int polygon, int vertex, int newAdj )
{
	BW_GUARD;

	VertexDef & vertexDef = polygons_[polygon].vertices[vertex];
	vertexDef.adjacentID = newAdj;
}

BW::vector<int> ChunkView::findWaypoints( const Vector3& inpt, float girth ) const
{
	BW_GUARD;

	BW::vector<int> result;

	Vector2 pt(inpt.x, inpt.z);

	// do a brute force search for the polygon
	uint i, j;
	for (i = 0; i < polygons_.size(); i++)
	{
		const PolygonDef & p = polygons_[i];
		if (!p.vertices.size()) continue;
		if (!this->equivGirth( i, girth )) continue;

		Vector2 av, bv;
		bv = p.vertices.back().position;
		for (j = 0; j < p.vertices.size(); j++)
		{
			av = bv;
			bv = p.vertices[j].position;

			LineEq leq( av, bv );
			if (leq.isInFrontOf( pt )) break;
		}

		if (j == p.vertices.size())
		{
			result.push_back( i );
		}
	}

	return result;
}

BW_END_NAMESPACE
