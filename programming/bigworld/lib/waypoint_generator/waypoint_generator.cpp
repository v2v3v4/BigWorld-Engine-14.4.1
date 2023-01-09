#include "pch.hpp"

#include "waypoint_flood.hpp"
#include "waypoint_generator.hpp"
#include "waypoint/waypoint_tga.hpp"

#include "cstdmf/debug.hpp"

#include "chunk/chunk_space.hpp"

#include <cmath>

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

static int 			g_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static int 			g_dz[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static Vector2		g_normal[8];

// Floating point fudge factor
#define GENERATOR_FLOAT_FUDGE	0.001f

// BSP nodes longer or wider than this will be split in half
// instead of along an edge (this is in units of the grid resolution)
#define EVEN_SPLIT_THRESHOLD 25.0f

// The maximum range of grid heights that a waypoint can contain (in metres)
const float MAX_HEIGHT_RANGE = 3.f;


/**
 *	This is the comparison operator for PointDefs. It sorts first by angle, 
 *	then by offset, then by t. Note that since offset and t are floating 
 *	point, there is special handling to prevent rounding errors.
 *	This is used by the set for sorting and uniqueness.
 */ 
bool WaypointGenerator::PointDef::operator<(
		const WaypointGenerator::PointDef& p) const
{
	BW_GUARD;

	if ( angle < p.angle )
		return true;
	if ( angle > p.angle )
		return false;

	if ( fabs(offset - p.offset) > GENERATOR_FLOAT_FUDGE )
	{
		if ( offset < p.offset )
			return true;
		if ( offset > p.offset )
			return false;
	}

	if ( fabs(t - p.t) > GENERATOR_FLOAT_FUDGE )
	{
		return t < p.t;
	}

	if (!isEqual( minHeight, p.minHeight ))
	{
		return minHeight < p.minHeight;
	}

	if (!isEqual( maxHeight, p.maxHeight ))
	{
		return maxHeight < p.maxHeight;
	}

	return false;
}

/**
 *	This is the equivalence operator for PointDefs. Note that since offset 
 *	and t are floating point, there is special handling to prevent
 *	rounding errors. This is used only by us.
 */ 
bool WaypointGenerator::PointDef::operator==(
		const WaypointGenerator::PointDef& p) const
{
	BW_GUARD;

	return 
		angle == p.angle &&
	   	fabs( offset - p.offset ) < GENERATOR_FLOAT_FUDGE &&
		fabs( t - p.t ) < GENERATOR_FLOAT_FUDGE;
}

bool WaypointGenerator::PointDef::nearlySame(
		const WaypointGenerator::PointDef& p) const
{
	BW_GUARD;

	return 
		angle == p.angle &&
	   	fabs( offset - p.offset ) < 1.5 &&
		fabs( t - p.t ) < 1.5;
}


/**
 *	This is the comparison operator for EdgeDef structures.
 *	It is used exclusive by set for sorting and equality.
 */
bool WaypointGenerator::EdgeDef::operator<(
		const WaypointGenerator::EdgeDef& e) const
{
	if ( from.x != e.from.x )
		return from.x < e.from.x;
	if ( from.y != e.from.y )
		return from.y < e.from.y;
	if ( to.x != e.to.x )
		return to.x < e.to.x;
	if ( to.y != e.to.y )
		return to.y < e.to.y;
	return id < e.id;
}

/**
 *	This is the equivalence operator for EdgeDef structures.
 *	It is used internally by us.
 */
bool WaypointGenerator::EdgeDef::operator==(
		const WaypointGenerator::EdgeDef& e) const
{
	if ( from.x != e.from.x )
		return false;
	if ( from.y != e.from.y )
		return false;
	if ( to.x != e.to.x )
		return false;
	if ( to.y != e.to.y )
		return false;
	return true;
}

/**
 *	This method says whether or not the given pt is near enough
 *	to being inside this polygon
 */
bool WaypointGenerator::PolygonDef::ptNearEnough( const Vector3 & pt ) const
{
	BW_GUARD;

	if ( pt.y > maxHeight + 1.5f || pt.y < minHeight - 0.5f ) 
		return false;

	uint ai = static_cast<uint>(vertices.size() - 1);
	for ( uint bi = 0; bi < vertices.size(); ai=bi++ )
	{
		vertices[ai].pos;
		vertices[bi].pos;

		float u = vertices[bi].pos.x - vertices[ai].pos.x;
		float v = vertices[bi].pos.y - vertices[ai].pos.y;

		float xd = pt.x - vertices[ai].pos.x;
		float zd = pt.z - vertices[ai].pos.y;

		bool anyInside = false;
		xd += 1.f;	zd -= 2.f;	// before first loop will change to both -1
		for ( int j = 0; j < 9; ++j )
		{
			if (j % 3 != 0)
			{
				xd += 1.f;
			}
			else
			{
				xd -= 2.f;
				zd += 1.f;
			}
			if (xd * v - zd * u >= 0) anyInside = true;
		}
		if ( !anyInside )
			return false;
	}
	return true;
}


/**
 *	This is the WaypointGenerator constructor.
 */ 
WaypointGenerator::WaypointGenerator() :
	gridX_(),
	gridZ_(),
	gridResolution_( 0.0f ),
	pProgress_( NULL ),
	sets_( 0 )
{
	BW_GUARD;

	for ( int g = 0; g < 16; ++g )
	{
		adjGrids_[g] = NULL;
		hgtGrids_[g] = NULL;
	}

	for ( int i = 0; i < 8; ++i )
	{
		g_normal[i].x = float(g_dx[i]);
		g_normal[i].y = float(g_dz[i]);
		// g_normal[i].normalise(); this use DX9 function which produce wrong
		//							results on some platform
		g_normal[i] *= 1.f/g_normal[i].length();
	}
}

/**
 *	This is the WaypointGenerator destructor.
 */ 
WaypointGenerator::~WaypointGenerator()
{
	BW_GUARD;

	this->clear();
}

/**
 *	This method returns the generator to its initial state. All allocated
 *	memory is freed.
 */ 
void WaypointGenerator::clear()
{
	BW_GUARD;

	for ( int g = 0; g < 16; ++g )
	{
		bw_safe_delete_array(adjGrids_[g]);
		bw_safe_delete_array(hgtGrids_[g]);
	}

	bsp_.clear();
	points_.clear();
	edges_.clear();
	polygons_.clear();
}

/**
 *	This method reads adjacency information from a TGA file generated by
 *	the WaypointFlood class. Each pixel is 8 bits, and each bit indicates
 *	whether one of the 8 neighbouring pixels is adjacent to this one.
 *	The sampling resolution and offset are stored as additional fields
 *	in the TGA header.
 *
 *	@param filename		Name of the TGA file to load.
 *	@return True if successful.
 */
bool WaypointGenerator::readTGA(const char* filename)
{
	BW_GUARD;

	WaypointTGAHeader hdr;
    FILE* pFile;

	if ( !(pFile = bw_fopen(filename, "rb")) )
	{
		ERROR_MSG("WaypointGenerator: Failed to open %s.\n", filename);
		return false;
	}

	if ( !(fread(&hdr, sizeof(hdr), 1, pFile)) )
	{
		ERROR_MSG("WaypointGenerator: Failed to read header.\n");
		fclose(pFile);
		return false;
	}

	if ( hdr.extraLength != sizeof(Vector3) + sizeof(float) ||
		hdr.colourMapType != 1 ||
		hdr.imageType != 1 ||
		hdr.colourMapStart != 0 ||
		hdr.colourMapLength != 256 ||
		hdr.colourMapDepth != 24 ||
		hdr.x != 0 ||
		hdr.y != 0 ||
		hdr.bpp != 8 ||
		hdr.imageDescriptor != 0)
	{
		ERROR_MSG("WaypointGenerator: Invalid TGA header.\n");
		fclose(pFile);
		return false;
	}

	if ( !this->init( hdr.width/(16*4), hdr.height/(16*4), hdr.gridMin, hdr.gridResolution ) )
	{
		fclose(pFile);
		return false;
	}

	if(fseek(pFile, 768, SEEK_CUR) != 0)
	{
		ERROR_MSG("WaypointGenerator: Seek error.\n");
		fclose(pFile);
		return false;
	}
	
	for ( int g = 0; g < 16; ++g )
	{
		if (fread(
			adjGrids_[g], gridX_ * gridZ_ * sizeof(AdjGridElt), 1, pFile ) != 1)
		{
			ERROR_MSG("WaypointGenerator: Read error.\n");
			fclose(pFile);
			return false;
		}
	}
	for ( int g = 0; g < 16; ++g )
	{
		if (fread(
			hgtGrids_[g], gridX_ * gridZ_ * sizeof(float), 1, pFile ) != 1)
		{
			ERROR_MSG("WaypointGenerator: Read error.\n");
			fclose(pFile);
			return false;
		}
	}

	fclose( pFile );

	identifier_ = filename;

	if (identifier_.rfind( '/' ) != identifier_.npos)
	{
		identifier_ = identifier_.substr( identifier_.rfind( '/' ) + 1 );
	}

	return true;
}


/**
 *	This method initialises this waypoint generator with an empty
 *	grid of the given dimensions
 */
bool WaypointGenerator::init( int width, int height,
	const Vector3 & gridMin, float gridResolution )
{
	BW_GUARD;

	this->clear();

	gridX_ = width;
	gridZ_ = height;
	gridMin_ = gridMin;
	gridResolution_ = gridResolution;
	
	for ( int g = 0; g < 16; ++g )
	{
		adjGrids_[g] = new AdjGridElt[gridX_ * gridZ_];
		hgtGrids_[g] = new float[gridX_ * gridZ_];
	}

	return true;
}


/**
 *	This method generates waypoint polygons. It should be called after readTGA.
 */ 
void WaypointGenerator::generate()
{
	BW_GUARD;

	this->initBSP();

	BW::vector<int> indexStack;
	indexStack.push_back( 0 );
	while (!indexStack.empty()) 
	{
		this->processNode( indexStack );
	}

	this->generatePoints();
	this->generatePolygons();
	this->generateAdjacencies();
#ifndef DEBUG_UNRECIPROCATED_ADJACENCY
	this->joinPolygons();
#endif
}

/**
 *	This method creates the initial BSP node. It is a rectangle, the size 
 *	of the entire area.
 */ 
void WaypointGenerator::initBSP()
{
	BW_GUARD;

	BSPNode node;
	node.splitOffset = 0.0f;
	node.splitNormal = 0;
	node.waypoint = false;
	node.baseX = 0;
	node.baseZ = 0;
	node.width = 0;

	Vector2 min(0.0f, 0.0f);
	Vector2 max(float(gridX_ - 1), float(gridZ_ - 1));
	
	node.borderOffset[0] = -g_normal[0].dotProduct(min);
	node.borderOffset[1] = -g_normal[1].dotProduct(min);
	node.borderOffset[2] = -g_normal[2].dotProduct(min);
	node.borderOffset[3] = -g_normal[3].dotProduct(Vector2(min.x, max.y));
	node.borderOffset[4] = -g_normal[4].dotProduct(max);
	node.borderOffset[5] = -g_normal[5].dotProduct(max);
	node.borderOffset[6] = -g_normal[6].dotProduct(max);
	node.borderOffset[7] = -g_normal[7].dotProduct(Vector2(max.x, min.y));

	node.parent = -1;
	node.front = -1;
	node.back = -1;
	node.waypointIndex = -1;
	node.minHeight = -13000.f;
	node.maxHeight =  13000.f;
	node.calcCentre();
	this->reduceNodeHeight(node);

	bsp_.push_back(node);
}


bool WaypointGenerator::nodesSame( const int index1, const int index2 )
{
	BW_GUARD;

	for ( int i=0; i<8; ++i ) 
	{
		// find the ith angle edge for each node.
		PointDef p1a;
		PointDef p1b;
		bool res1 = calcEdge( index1, i, p1a, p1b );
		PointDef p2a;
		PointDef p2b;
		bool res2 = calcEdge( index2, i, p2a, p2b );

		// if the edge exists for one but not the other, then not same.
		if ( res1 != res2 )
		{
			return false;
		}

		// if there is an edge then check to see that same 
		if ( res1 )
		{
			if( !(p1a.nearlySame(p2a)) ) // operator== includes float_fudge factor.
			{
				return false;
			}
			if( !(p1b.nearlySame(p2b)) )
			{
				return false;
			}
		}

	}

	return true;
}


//#define BSP_GEN_DEBUG

/**
 *	This method is called recursively to generate the BSP tree. It subdivides
 *	the current node until it is entirely passable, or entirely impassable. It
 *	tries to split the node along edges, between passable and impassable regions.
 *	It also tries to find a split close to the centre of the region, in order
 *	to create a balanced tree. 
 *	
 *	@param index	Index of the node to process.
 */

void WaypointGenerator::processNode( BW::vector<int> & indexStack )
{
	BW_GUARD;

	int g, ng, a;
	unsigned long mask;
	bool passable = true;
	SplitDef split, bestSplit;
	int passableCount = 0;
	int impassableCount = 0;
	float nh, ah;

	const int tgx = int(gridX_);
	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {tgx, tgx, 0, -tgx, -tgx, -tgx, 0, tgx};

	const int index = indexStack.back();
	indexStack.pop_back();

	bestSplit.value = -1.0f;

	if ( pProgress_ )
	{
		pProgress_->onProgress( L"BSP", static_cast<int>(bsp_.size()) );
	}

	const int z1 = (int)-bsp_[index].borderOffset[0];
	const int z2 = (int)bsp_[index].borderOffset[4];
	const int x1 = (int)-bsp_[index].borderOffset[2];
	const int x2 = (int)bsp_[index].borderOffset[6];
	const float y1 = bsp_[index].minHeight;
	const float y2 = bsp_[index].maxHeight;

	// Examine every point in the node. For each point, examine each
	// neighbour that is inside the node. For the node to be passable,
	// there must be an adjacency to every neighbour.
	for ( int x=x1; x<=x2; ++x )
    {
        for ( int z=z1; z<=z2; ++z )
		{
			if ( !bsp_[index].pointInNode(Vector2(float(x), float(z))) )
				continue;

			int ptindex = x + gridX_ * z;

			// make sure there is only one point at these coordinates in the node
			ng = -1;
			for ( g = 0; g < 16; ++g )
			{
				ah = hgtGrids_[g][ptindex];
				if ( ah < y1 || ah > y2 ) continue;
				if ( adjGrids_[g][ptindex].all == 0 ) continue;

				// ok, this is our point. see if we already have one
				if ( ng != -1 ) break;

				// this is the first so record it
				ng = g;
				nh = ah;
			}

			// see if we have overlapping points
			if ( g != 16 )
			{
				// suggest a split
				split.normal = SplitDef::VERTICAL_SPLIT;
				split.heights[0] = min(nh,ah);
				split.heights[1] = max(nh,ah);
				this->calcSplitValue(bsp_[index], split);
				if(split.value > bestSplit.value)
					bestSplit = split;

				// and let none pass
				passable = false;
				impassableCount += 8;
				continue;
			}

			// set the mask then
			mask = 0;
			if ( ng != -1 )
			{
				for ( a=0; a<8; ++a )
				{
					int og = adjGrids_[ng][ptindex].angle( a );
					if ( og != 0 )
					{
						float h = hgtGrids_[og][ptindex+dx[a]+dy[a]];
						if ( h >= y1 && h <= y2 )
							mask |= 1<<a;
					}
				}

			}

			// OK, Now examine the adjacency mask
			if ( mask == 0x00 )
			{
				// This point has no adjacencies.
				// No need to check them individually.
				passable = false;
				impassableCount += 8;
				continue;
			}

			if ( mask == 0xFF )
			{
				// This point has 8 adjacencies.
				// No need to check them individually.
				passableCount += 8;
				continue;
			}

			// Check all the adjacencies individually.
			for ( a=0; a<8; ++a )
			{
				Vector2 neighbour(float(x + g_dx[a]), float(z + g_dz[a]));

				// If the point isn't in this node, then we're crossing an
				// existing boundary, so there's no need to try to split here.
				if (!bsp_[index].pointInNode(neighbour))
					continue;

				// See if we cannot go at angle a.
				if(!(mask & (1 << a)))
				{
					passable = false;
					impassableCount++;

					// This edge should be passable, but it is not.
					// So, if the edge to the left or right of it
					// is passable, attempt to split the node along
					// the passable edge.
					if(mask & (1 << ((a + 1) % 8)))
					{
						split.normal = (a + 7) % 8;
						split.position[0] = float(x);
						split.position[1] = float(z);
						this->calcSplitValue(bsp_[index], split);
						if(split.value > bestSplit.value)
							bestSplit = split;
					}

					if(mask & (1 << ((a + 7) % 8)))
					{
						split.normal = (a + 1) % 8;
						split.position[0] = float(x);
						split.position[1] = float(z);
						this->calcSplitValue(bsp_[index], split);
						if(split.value > bestSplit.value)
							bestSplit = split;
					}
				}
				else
				{
					passableCount++;
				}
			}
		}
	}

	// If after all that, we didn't find any impassable edges,
	// this node is entirely passable. So classify it as a waypoint.
	if(bestSplit.value == -1 && passableCount > impassableCount)
		passable = true;

	// If there is too much variation in height and we're not going to
	// split it for another reason, then split away

	if (passable && y2 - y1 > MAX_HEIGHT_RANGE)
	{
		passable = false;

		if (bestSplit.value == -1)
		{
			bestSplit.normal = 8;
			bestSplit.heights[0] = y1;
			bestSplit.heights[1] = y2;
			bestSplit.value = 1;
		}

	}

	// If it's still considered passable, flag it as a waypoint and return
	if (passable)
	{
		bsp_[index].waypoint = true;
		return;
	}

	// If we have no idea how to split this node, then give up
	// (whether or not it was passable, usually mostly impassable)
	if (bestSplit.value <= 0.0f) 
	{
		bsp_[index].waypoint = false;
		return;
	}

	// If our node is really big, replace whatever split was selected
	// with an even split, so a more balanced tree is created.
	float width = float(x2 - x1);
	float height = float(z2 - z1);
	if(width > EVEN_SPLIT_THRESHOLD || height > EVEN_SPLIT_THRESHOLD)
	{
		bestSplit.position[0] = float(x1 + int(width / 2));
		bestSplit.position[1] = float(z1 + int(height / 2));
		bestSplit.normal = width > height ? 2 : 0;
	}

	// And subdivide it further
	if ( this->splitNode( index, bestSplit ) )
	{
		indexStack.push_back( bsp_[index].back );
		indexStack.push_back( bsp_[index].front );
	}

	// the split was vertical and there was a problem.
	// if there was a problem, bsp_ and indexStack are untouched.
	else 
	{
		// INFO_MSG( "Problem vertical splitting, using best horizontal split instead\n" );
		doBestHorizontalSplit(index,indexStack);
	}

}

void WaypointGenerator::doBestHorizontalSplit( int index, BW::vector<int> & indexStack )
{
	BW_GUARD;

	bool override = false;

	SplitDef split, bestSplit;

	// find center
	bsp_[index].calcCentre();
	this->reduceNodeHeight( bsp_[index] );

	float cx = (float)floor( bsp_[index].centre.x + (-bsp_[index].borderOffset[2]) );
	float cz = (float)floor( bsp_[index].centre.y + (-bsp_[index].borderOffset[0]) );

	split.position[0] = cx;
	split.position[1] = cz;
	split.value = -1.0;
	split.normal = 0;
	bestSplit = split;
		
	// don't override existing bsp_ children on first split - create new ones.
	if ( this->splitNode( index, split, override ) )
	{
		override = true;

		std::pair<float,int> hgt1 = this->averageHeight( bsp_[bsp_[index].front] );
		std::pair<float,int> hgt2 = this->averageHeight( bsp_[bsp_[index].back] );
		// ensure at least some points in each node.
		if ( hgt1.second != 0 && hgt2.second != 0 )
		{
			// ensure evenish split.
			float ratio = (float)hgt1.second / (float)hgt2.second;
			if( ratio >= 0.2 && ratio <= 5.0 )
			{
				float value = (float)fabs( hgt1.first - hgt2.first );
				// acceptable split - store!
				bestSplit.value = value;
			}
		}
	}

	// trying latter 4 angles is redundant.
	for ( int a=1; a<4; ++a )
	{
		split.normal = a;
		if ( this->splitNode( index, split, override ) ) // override already calculated front/backs.
		{
			override = true;

			std::pair<float,int> hgt1 = this->averageHeight( bsp_[bsp_[index].front] );
			std::pair<float,int> hgt2 = this->averageHeight( bsp_[bsp_[index].back] );
			// ensure at least some points in each node.
			if ( hgt1.second != 0 && hgt2.second != 0 )	
				{
				// ensure evenish split.
				float ratio = (float)hgt1.second / (float)hgt2.second;
				if( ratio >= 0.2 && ratio <= 5.0 )
				{
					float value = (float)fabs( hgt1.first - hgt2.first );
					// if best split so far, then store!
					if (value > bestSplit.value) {
						bestSplit.value = value;
						bestSplit.normal = a;
					}
				}
			}
		}
	}
		
	if (bestSplit.value >= 0.0f)
	{
	
		this->splitNode( index, bestSplit, true );

		// check to see if front or back are the same
		// this is a bit redundant now the above ratio checks are in place,
		// but keep it here anyway, as this part of the algorithm doesn't
		// take very long anyway.
		bool same = nodesSame( index, bsp_[index].front );

		if ( !same )
		{
			same = nodesSame( index, bsp_[index].back );
			if ( !same )
			{
				// INFO_MSG( "Split successful\n" );
				indexStack.push_back( bsp_[index].back );
				indexStack.push_back( bsp_[index].front );
				return;
			}
		}
	}

	// else
	// split didn't work! quit. not waypoints, and don't add to stack to recurse.
	bsp_[bsp_[index].back].waypoint = false;
	bsp_[bsp_[index].front].waypoint = false;
	WARNING_MSG( "Problem splitting node %d. Throwing away\n", index );

}


std::pair<float,int> WaypointGenerator::averageHeight(const BSPNode & node)
{	
	BW_GUARD;

	float sum = 0.0;

	const int z1 = (int)-node.borderOffset[0];
	const int z2 = (int)node.borderOffset[4];
	const int x1 = (int)-node.borderOffset[2];
	const int x2 = (int)node.borderOffset[6];
	const float y1 = node.minHeight;
	const float y2 = node.maxHeight;

	BW::vector<float> sums(16);
	BW::vector<int> counts(16);

	int count = 0;
	for (int x=x1; x<=x2; ++x)
    {
        for (int z=z1; z<=z2; ++z)
		{
			if(!node.pointInNode(Vector2(float(x), float(z))))
				continue;
			
			int ptindex = x + gridX_ * z;

			for (int g=0; g<16; ++g)
			{
				float h = hgtGrids_[g][ptindex];
				if (h < y1 || h > y2) continue;
				if (adjGrids_[g][ptindex].all == 0) continue;
				sums[g] += h;
				++counts[g];
			}
		}
	}

	// find highest count.
	// we will split based on hieght diff in most populated level.
	float maxs;
	int maxc = 0;
	for (int i=0; i<16; ++i)
	{
		if (maxc < counts[i])
		{
			maxc = counts[i];
			maxs = sums[i];
		}
	}

	if (maxc == 0)
	{
		return std::pair<float,int>(0.0f, 0);
	}

	// else
	return std::pair<float,int>( maxs / (float)maxc, maxc );
}


/**
 *	This method checks for points disconnected due to the height range
 *	of the node.
 */
bool WaypointGenerator::findDispoints( const BSPNode & frontNode )
{
	BW_GUARD;

	const int tgx = int(gridX_);
	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {tgx, tgx, 0, -tgx, -tgx, -tgx, 0, tgx};

	frontNode.updatePointInNodeFlags();

	int z1 = (int)-frontNode.borderOffset[0];
	int z2 = (int)frontNode.borderOffset[4];
	int x1 = (int)-frontNode.borderOffset[2];
	int x2 = (int)frontNode.borderOffset[6];

	for (int x = x1; x <= x2; ++x)
	{
		for (int z = z1; z <= z2; ++z)
		{
			if (!frontNode.pointInNode( x, z ))
			{
				continue;
			}

			int ptindex = x + gridX_ * z;

			for (int g = 0; g < 16; g++)
			{
				float ah = hgtGrids_[g][ptindex];
				if (ah < frontNode.minHeight || ah > frontNode.maxHeight)
					continue;
				
				AdjGridElt & age = adjGrids_[g][ptindex];
				if (age.all == 0) continue;
				for (int a = 0; a < 8; a++)
				{
					int og = age.angle( a );
					if (og == 0) continue;

					if (!frontNode.pointInNode( x + g_dx[a], z + g_dz[a] ))
						continue;

					// ok, we want to be connected to this node,
					// and it is within our x-z range, so check its height
					float h = hgtGrids_[og][ptindex+dx[a]+dy[a]];
					if (h >= frontNode.minHeight && h <= frontNode.maxHeight)
						continue;

					return true;
				}
			}
		}
	}

	return false;
}


/**
 *	This method calculates a split value for the given split.
 *	The larger this value, the better the split. The criteria
 *	is currently just the inverse distance from the centre of
 *   the node.
 *	
 *	@param index	Index of the node that is being split
 *	@param split	The split parameters are in here, and the
 *					split value is returned in here as well.
 */ 
void WaypointGenerator::calcSplitValue( const BSPNode & node, SplitDef & split )
{
	BW_GUARD;

	float n1, n2, n3, n4, value;
	
	// If the normal is 8, then we're doing a vertical split
	if (split.normal == SplitDef::VERTICAL_SPLIT)
	{
		// Calculate value using an algo similar to a horizontal split
		// (inverse of distance from centre squared)
		value = ( split.heights[0] + split.heights[1] ) * 0.5f
			- (node.minHeight+node.maxHeight)*0.5f;
		value = value * value;
		if (value == 0.0f) value = 0.1f;
		split.value = 1.0f / value;
		return;
	}

	// First check to see if the split is valid.
	// If not, return a value of -1.
	n1 = node.borderOffset[split.normal];
	n2 = -g_normal[split.normal].dotProduct( Vector2(split.position[0], split.position[1]) );

	n3 = node.borderOffset[split.normal ^ 4];
	n4 = -g_normal[split.normal ^ 4].dotProduct( Vector2(split.position[0], split.position[1]) );

	if(fabs(n1 - n2) < 0.001 || fabs(n3 - n4) < 0.001)
	{
		split.value = -1.0f;
		return;
	}
	
	// Split value is currently the inverse of the distance from the
	// centre of the node. A larger split value is better, and they
	// must be greater than zero to be valid.
	value = (node.centre - Vector2(split.position[0],split.position[1])).lengthSquared();
	
	if (value < 0.1f)
	{
		value = 0.1f;
	}

	split.value = 1.0f / value;
}

/**
 *	This method performs a split. It creates two new BSP nodes,
 *	and calculates the new bounding edges for each one.
 *	
 *	@param index	Index of the node to split
 *	@param split	The position and angle of the split.
 *  @param replace  (defaults to false).
 *  @returns  true if split ok, false if problem with vertical split.
 */ 
bool WaypointGenerator::splitNode( int index, const SplitDef & split,
	bool replace )
{
	BW_GUARD;

	BSPNode node = bsp_[index];
	//const BSPNode save = node;
	BSPNode front = node;
	BSPNode back = node;

	front.parent = index;
	front.front = -1;
	front.back = -1;
	back.parent = index;
	back.front = -1;
	back.back = -1;

	float d1 = 0.f;
	float d2 = 0.f;
	if (split.normal == SplitDef::VERTICAL_SPLIT)
	{
		if ( split.heights[0] == split.heights[1] )
		{
			// cannot do a vertical split, heights are the same
			return false;
		}
		d1 = (split.heights[0] + split.heights[1]) * 0.5f;
		d2 = d1;
	}
	else
	{
		d1 = -g_normal[split.normal].dotProduct( 
			Vector2(split.position[0],split.position[1]) );
		d2 = -d1;
	}
	
	node.splitNormal = split.normal;
	node.splitOffset = d1;

	if (split.normal == SplitDef::VERTICAL_SPLIT)
		front.maxHeight = d1;
	else
		front.borderOffset[split.normal] = d1;

	refineNodeEdges( &front );
	front.calcCentre();
	this->reduceNodeHeight( front );

	if (split.normal == SplitDef::VERTICAL_SPLIT)
	{
		if (this->findDispoints( front ))
		{
//			INFO_MSG( "splitNode: Vertical split at %f (collapsed to %f) leads "
//				"to points disconnected by split. Trying range %f to %f.\n",
//				d1, front.maxHeight, split.position[0], split.position[1] );

			float h;
			for (h = split.position[0] + 0.1f;
				h < split.position[1] - 0.1f;
				h += 0.1f)
			{
				front.maxHeight = h;

				refineNodeEdges( &front );
				front.calcCentre( );
				this->reduceNodeHeight( front );
				if (!this->findDispoints( front ))
				{
//					INFO_MSG( "splitNode: Found clean vertical split "
//						"at %f (collapsed to %f)\n", h, front.maxHeight );
					d2 = h;
					break;
				}
			}

			if (h >= split.position[1] - 0.1f)
			{
//				INFO_MSG( "splitNode: Could not find any clean vertical split "
//					"(caller will deal)\n" );
				return false; // problem with vertical split. bsp_[index] unchanged.
			}
		}
	}

	if (split.normal == SplitDef::VERTICAL_SPLIT)
	{
		//back.minHeight = (split.position.x+split.position.y)*0.5f;
		// split nodes must share some points!
		// so use maxHeight from front's calcCentre)
		back.minHeight = d2;
	}
	else 
	{
		back.borderOffset[split.normal ^ 4] = d2;
	}

	refineNodeEdges( &back );
	back.calcCentre();
	this->reduceNodeHeight( back );

	if ( (fabs(back.borderOffset[0] + back.borderOffset[4]) < 0.01) ||
		 (fabs(back.borderOffset[1] + back.borderOffset[5]) < 0.01) ||
         (fabs(back.borderOffset[2] + back.borderOffset[6]) < 0.01) ||
         (fabs(back.borderOffset[3] + back.borderOffset[7]) < 0.01) ||
		 (fabs(front.borderOffset[0] + front.borderOffset[4]) < 0.01) ||
		 (fabs(front.borderOffset[1] + front.borderOffset[5]) < 0.01) ||
         (fabs(front.borderOffset[2] + front.borderOffset[6]) < 0.01) ||
         (fabs(front.borderOffset[3] + front.borderOffset[7]) < 0.01) )
	{
		return false;
	}

	if (replace)
	{
		bsp_[node.front] = front;
		bsp_[node.back] = back;
	}
	else 
	{
		node.front = static_cast<int>(bsp_.size());
		node.back = static_cast<int>(bsp_.size() + 1);
		bsp_.push_back(front);
		bsp_.push_back(back);
	}

	bsp_[index] = node; // change node only if split.
	return true;
}

/**
 *	This method returns true if the given point is within the given BSP node.
 *	Points on the border of the node are included within the node.
 *	This method only checks the diagonal edges.
 *
 *	@param point	Point to check
 *	@param offsets	The diagonal offsets
 *	@return True if successful.
 */ 
static bool pointInNodeDiagonalOnly( float x, float z, float offset[ 4 ] )
{
	BW_GUARD;

	if (x + z + offset[ 0 ] < -0.1f)
	{
		return false;
	}
	if (x - z + offset[ 1 ] < -0.1f)
	{
		return false;
	}
	if (- x - z + offset[ 2 ] < -0.1f)
	{
		return false;
	}
	if (- x + z + offset[ 3 ] < -0.1f)
	{
		return false;
	}

	return true;
}

/**
 *	This method refine the node edges to make them exactly wrap the BSP node
 *
 *	@param node		the node to be refined
 */
void WaypointGenerator::refineNodeEdges( BSPNode* node ) const
{
	BW_GUARD;

	float x, x1, x2, z, z1, z2;
	z1 = -node->borderOffset[0];
	z2 = node->borderOffset[4];
	x1 = -node->borderOffset[2];
	x2 = node->borderOffset[6];

	static const float HALF_ROOT_2 = 0.707107f;

	float offsets[4] = {
		node->borderOffset[1] / HALF_ROOT_2,
		node->borderOffset[3] / HALF_ROOT_2,
		node->borderOffset[5] / HALF_ROOT_2,
		node->borderOffset[7] / HALF_ROOT_2
	};

	float minX = 100000, maxX = 0, minZ = 100000, maxZ = 0;
	for (x = x1; x <= x2; x += gridResolution_)
	{
		for (z = z1; z <= z2; z += gridResolution_)
		{
			if (x >= minX && x <= maxX &&
				z >= minZ && z <= maxZ)
			{
				continue;
			}

			if (!pointInNodeDiagonalOnly( x, z, offsets ))
				continue;
			if( x < minX )
				minX = x;
			if( x > maxX )
				maxX = x;
			if( z < minZ )
				minZ = z;
			if( z > maxZ )
				maxZ = z;
		}
	}

	if (minX > maxX || minZ > maxZ)
	{
		minX = maxX = x1;
		minZ = maxZ = z1;
	}

	// prevent the BSP node from having zero size
	if (almostEqual( minX, maxX ))
	{
		if( maxX < x2 )
			maxX += gridResolution_;
		else
			minX -= gridResolution_;
	}
	if (almostEqual( minZ, maxZ ))
	{
		if( maxZ < z2 )
			maxZ += gridResolution_;
		else
			minZ -= gridResolution_;
	}

	node->borderOffset[0] = -minZ;
	node->borderOffset[4] = maxZ;
	node->borderOffset[2] = -minX;
	node->borderOffset[6] = maxX;
}


void WaypointGenerator::reduceNodeHeight( BSPNode & node )
{
	BW_GUARD;

	float newMin = 13000.f;
	float newMax = -13000.f;

	int x, x1, x2, z, z1, z2;
	z1 = (int)-node.borderOffset[0];
	z2 = (int)node.borderOffset[4];
	x1 = (int)-node.borderOffset[2];
	x2 = (int)node.borderOffset[6];

	node.updatePointInNodeFlags();

	for (x = x1; x <= x2; x++)
    {
        for (z = z1; z <= z2; z++)
		{
			if (!node.pointInNode( x, z ))
				continue;

			for (int g = 15; g >= 0; --g)
			{
				float height = hgtGrids_[g][x + gridX_ * z];
				if (height <= -9999.f) break;
				if (height < node.minHeight || height > node.maxHeight) continue;
				// also ignore this node if it has no adjacencies
				// (this is so failed fills do not distract us)
				if (adjGrids_[g][x + gridX_ * z].all == 0) continue;
			
				if (newMax < height)
					newMax = height;
				
				if (newMin > height)
					newMin = height;

			}
		}
	}

	node.minHeight = newMin;
	node.maxHeight = newMax;
}


/**
 *	This method recreate the point in node flags
 *
 */ 
void WaypointGenerator::BSPNode::updatePointInNodeFlags() const
{
	BW_GUARD;

	int z1 = (int)-borderOffset[0];
	int z2 = (int)borderOffset[4];
	int x1 = (int)-borderOffset[2];
	int x2 = (int)borderOffset[6];

	baseX = x1 - 2;
	baseZ = z1 - 2;
	width = ( z2 + 2 ) - ( z1 - 2 );

	pointInNodeFlags.assign( width * ( ( x2 + 2 ) - ( x1 - 2 ) ), FALSE );

	for (; x1 <= x2; ++x1)
	{
		for (z1 = baseZ + 2; z1 <= z2; ++z1)
		{
			if (pointInNode( Vector2( float( x1 ), float( z1 ) ) ))
			{
				pointInNodeFlags[ ( x1 - baseX ) * width + z1 - baseZ ] = TRUE;
			}
		}
	}
}


/**
 *	This method returns true if the given point is within the given BSP node.
 *	Points on the border of the node are included within the node.
 *
 *	@param node		The BSP node
 *	@param x, z		Point to check
 *	@return True if successful.
 */ 
BOOL WaypointGenerator::BSPNode::pointInNode( int x, int z ) const
{
	BW_GUARD;

	return pointInNodeFlags[ ( x - baseX ) * width + z - baseZ ];
}

/**
 *	This method calculates the centre of the given node.
 *
 *	@param node	The node for which to calculate the centre.
 */
void WaypointGenerator::BSPNode::calcCentre()
{
	BW_GUARD;

	centre.x = (borderOffset[6] + borderOffset[2]) / 2;
	centre.y = (borderOffset[4] + borderOffset[0]) / 2;
}

/**
 *	This method returns true if the given point is within the given BSP node.
 *	Points on the border of the node are included within the node.
 *
 *	@param node		The BSP node
 *	@param point	Point to check
 *	@return True if successful.
 */ 
bool WaypointGenerator::BSPNode::pointInNode(const Vector2& point) const
{
	BW_GUARD;

	static const float HALF_ROOT_2 = 0.707107f;

	if (point.y + borderOffset[ 0 ] < -0.1f)
	{
		return false;
	}

	float x = HALF_ROOT_2 * point.x;
	float y = HALF_ROOT_2 * point.y;

	if (x + y + borderOffset[ 1 ] < -0.1f)
	{
		return false;
	}
	if (point.x + borderOffset[ 2 ] < -0.1f)
	{
		return false;
	}
	if (x - y + borderOffset[ 3 ] < -0.1f)
	{
		return false;
	}
	if (-point.y + borderOffset[ 4 ] < -0.1f)
	{
		return false;
	}
	if (- x - y + borderOffset[ 5 ] < -0.1f)
	{
		return false;
	}
	if (-point.x + borderOffset[ 6 ] < -0.1f)
	{
		return false;
	}
	if (- x + y + borderOffset[ 7 ] < -0.1f)
	{
		return false;
	}

	return true;
}

/**
 *	This method calculates the start and end points of one of the edges of a
 *	BSP node. A node can have up to 8 edges, since there are 8 possible angles.
 *	However, it will not necessarily have all edges. The start and end point are
 *	returned as PointDefs. This basically defines the equation of a line, and the
 *	distance along that line from a known starting point. It is done this way
 *	to make it easy to sort points that are on the same line.
 *
 *	@param index	Index of the BSP node
 *	@param edge		Index of the edge (0-8)
 *	@param p1		The start point is returned here
 *	@param p2 		The end point is returned here
 *
 *	@return True if the edge exists, false otherwise.
 */ 
bool WaypointGenerator::calcEdge(int index, int edge, 
	PointDef& p1, PointDef& p2) const
{
	BW_GUARD;

	const BSPNode& node = bsp_[index];
	Vector2 s, n, d;
	float t, D, t1, t2;
	int i, edge2;
	
	// The starting point is either the intersection with the x or z
	// axis, depending on the gradient of the line.

	if(g_normal[edge].x == 0)
	{
		s.x = 0;
		s.y = -node.borderOffset[edge] / g_normal[edge].y;
	}
	else
	{
		s.x = -node.borderOffset[edge] / g_normal[edge].x;
		s.y = 0;
	}

	d = g_normal[(edge + 6) % 8];
	
	for(i = 1; i <= 3; i++)
	{
		edge2 = (edge + i) % 8;
		n = g_normal[edge2];
		D = -node.borderOffset[edge2];
		t = (D - n.dotProduct(s)) / n.dotProduct(d);
		if(i == 1 || t < t1)
			t1 = t;
	}

	for(i = 5; i <= 7; i++)
	{
		edge2 = (edge + i) % 8;
		n = g_normal[edge2];
		D = -node.borderOffset[edge2];
		t = (D - n.dotProduct(s)) / n.dotProduct(d);
		if(i == 5 || t > t2)
			t2 = t;
	}

	if(t1 <= t2)
		return false;
	
	p1.angle = edge;
	p1.offset = node.borderOffset[edge];
	p1.t = t1;
	p1.minHeight = node.minHeight;
	p1.maxHeight = node.maxHeight;
	
	p2.angle = edge;
	p2.offset = node.borderOffset[edge];
	p2.t = t2;
	p2.minHeight = node.minHeight;
	p2.maxHeight = node.maxHeight;
	
	return true;
}

/**
 *	This method iterates through the BSP tree, finds all points on the borders
 *	of passable regions, and adds them to a set. There are 8 possible angles
 *	for edges that a point can be on, however we only use 4. The other 4 angles
 *	are the same as the first 4, but in the opposite direction.
 */
void WaypointGenerator::generatePoints()
{
	BW_GUARD;

	unsigned int i, a;
	PointDef p1, p2;
	
	// insert an invalid point so we never need to look at points_.begin
	p1.angle = -1000;
	points_.insert( p1 );

	for(i = 0; i < bsp_.size(); i++)
	{
		if(pProgress_)
			pProgress_->onProgress(L"vertices", i);
		
		if(bsp_[i].waypoint)
		{
			for(a = 0; a < 4; a++)
			{
				if(this->calcEdge(i, a, p1, p2))
				{
					points_.insert(p1);
					points_.insert(p2);
				}
			}

			// The last 4 edges use the same angles
			// as the first.
			
			for(a = 4; a < 8; a++)
			{
				if(this->calcEdge(i, a, p1, p2))
				{
					this->invertPoint(p1);
					this->invertPoint(p2);

					points_.insert(p1);
					points_.insert(p2);
				}
			}
		}
	}
}

/**
 *	This method takes a PointDef for an angle 4-8, and inverts it,
 *	such that the angle is 0-3. 
 */	 
void WaypointGenerator::invertPoint(PointDef& p) const
{
	p.angle -= 4;
	p.offset = -p.offset;
	p.t = -p.t;
}
	
/**
 *	This helper function rounds a floating point number to the
 *	nearest fraction, where fraction = 1/denominator.
 */
static float roundToFraction(float f, int denominator)
{
	f *= denominator;
	f = float( f > 0.0f ? int(f + 0.5) : int(f - 0.5) );
	f /= denominator;
	return f;
}

/**
 *	This method takes a PointDef, and converts it to an actual position.
 *
 *	@param p	The PointDef
 *	@param v	The position
 */
void WaypointGenerator::pointToVertex(const PointDef& p, Vector2& v) const
{
	BW_GUARD;

	Vector2 s, d;

	// The starting point is either the intersection with the x or z
	// axis, depending on the gradient of the line.
	
	if(g_normal[p.angle].x == 0)
	{
		s.x = 0;
		s.y = -p.offset / g_normal[p.angle].y;
	}
	else
	{
		s.x = -p.offset / g_normal[p.angle].x;
		s.y = 0;
	}

	d = g_normal[(p.angle + 6) % 8];
	v = s + d * p.t;

	// Round the (x,y) to the nearest 0.5, to avoid floating point comparison
	// problems. Note that this code used to round to integers. However, two
	// diagonals can intersect halfway between the gridpoints. So, accuracy
	// of 0.5 is needed.

	v.x = roundToFraction(v.x, 2);
	v.y = roundToFraction(v.y, 2);
}

/**
 *	This method generates the waypoint polygons from the BSP. If an edge 
 *	is shared by two waypoint polygons, each one must share all the vertices 
 *	on that edge, so that adjacencies can be expressed correctly.
 *	So, this method uses the sorted list of points to find all vertices in
 *	between the start and end of each edge.
 */
void WaypointGenerator::generatePolygons()
{
	BW_GUARD;

	unsigned int i, a;
	BW::set<PointDef>::const_iterator pointIter;
	PointDef p1, p2;
	Vector2 v;
	
	for (i=0; i<bsp_.size(); ++i)
	{
		if(pProgress_)
			pProgress_->onProgress(L"polygons", i);
		
		if(bsp_[i].waypoint)
		{
			// make sure we have a polygon with area
			if (bsp_[i].borderOffset[0] == -bsp_[i].borderOffset[4]) continue;
			if (bsp_[i].borderOffset[1] == -bsp_[i].borderOffset[5]) continue;
			if (bsp_[i].borderOffset[2] == -bsp_[i].borderOffset[6]) continue;
			if (bsp_[i].borderOffset[3] == -bsp_[i].borderOffset[7]) continue;

			PolygonDef polyDef;

			for (a=0; a<4; ++a)
			{
				if ( this->calcEdge(i, a, p1, p2) )
				{
					p2.minHeight = -13000.f;	// sort before all other points
					for(pointIter = points_.lower_bound(p2);
						pointIter != points_.end() && !(*pointIter == p1);
						pointIter++)
					{
						// make sure it overlaps us
						if (pointIter->maxHeight < p1.minHeight) continue;
						if (pointIter->minHeight > p1.maxHeight) continue;

						// make a vertex out of it
						this->pointToVertex(*pointIter, v);
						VertexDef vd;
						vd.pos = v;
						vd.angles = (a - 2) % 8;
						vd.adjNavPoly = 0;
						vd.adjToAnotherChunk = false;
						polyDef.vertices.push_back(vd);
						//polyDef.vertices.push_back(v);
						//polyDef.angles.push_back((a - 2) % 8);
					}
				}
			}

			for(a = 4; a < 8; a++)
			{
				if(this->calcEdge(i, a, p1, p2))
				{
					this->invertPoint(p1);
					this->invertPoint(p2);

					p2.minHeight = 13000.f;	// sort after all other points
					for(pointIter = --points_.upper_bound(p2);
						pointIter != points_.begin() && !(*pointIter == p1);
						pointIter--)
					{
						// make sure it overlaps us
						if (pointIter->maxHeight < p1.minHeight) continue;
						if (pointIter->minHeight > p1.maxHeight) continue;

						// make a vertex out of it
						this->pointToVertex(*pointIter, v);
						VertexDef vd;
						vd.pos = v;
						vd.angles = (a - 2) % 8;
						vd.adjNavPoly = 0;
						vd.adjToAnotherChunk = false;
						polyDef.vertices.push_back(vd);
						//polyDef.vertices.push_back(v);
						//polyDef.angles.push_back((a - 2) % 8);
					}
				}
			}

			if(!polyDef.vertices.empty())
			{
				polyDef.minHeight = bsp_[i].minHeight;
				polyDef.maxHeight = bsp_[i].maxHeight;
				polyDef.set = 0;
				bsp_[i].waypointIndex = static_cast<int>(polygons_.size());
//				dprintf( "Waypoint %d from bsp node %d\n",
//					bsp_[i].waypointIndex, i );
#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
				polyDef.originalId = polygons_.size();
				DEBUG_MSG( "%d\n", polyDef.originalId );
#endif
				polygons_.push_back(polyDef);
				
#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
				if (polygons_.size() == 473)
				{
					DEBUG_MSG( "polygon 473 min, max height: %f %f\n", polygons_[473].minHeight, polygons_[473].maxHeight );
					DEBUG_MSG( "polygon 468 min, max height: %f %f\n", polygons_[468].minHeight, polygons_[468].maxHeight );
				}
#endif

			}
		}
	}
}

/**
 *	This method generates adjacencies for all waypoint polygons.
 *	It first generates a set of unique edges, indexed by start and
 *	end vertex, with the ID of the waypoint polygon as data. Then,
 *	for each edge, it searches for the reverse edge, finds the ID
 *	of the polygon, and makes that the adjacency.
 */ 
void WaypointGenerator::generateAdjacencies()
{
	BW_GUARD;

	unsigned int p, v, vertexCount;
	BW::set<EdgeDef>::iterator edgeIter;
	EdgeDef edge;
	int count = 0;

	// First add all the edges.
	for (p=0; p<polygons_.size(); ++p)
	{
		PolygonDef& polygon = polygons_[p];
		vertexCount = static_cast<uint>(polygon.vertices.size());
		
		if (pProgress_)
			pProgress_->onProgress(L"adjacencies", count++);

		for (v=0; v<vertexCount; ++v)
		{
			edge.from = polygon.vertices[v].pos;
			edge.to = polygon.vertices[(v + 1) % vertexCount].pos;
			if (edge.from == edge.to) continue;	// skip zero-length edges
			edge.id = p + 1;
			edges_.insert(edge);
		}
	}

	// Now calculate adjacencies.
	for (p=0; p<polygons_.size(); ++p)
	{
		PolygonDef& polygon = polygons_[p];
	
#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
		if (p==473 || p==468)
		{
			DEBUG_MSG( "p==%d\n",p );
			DEBUG_MSG( "min, max height, %f %f\n", polygon.minHeight, polygon.maxHeight );
			for (int ii=0; ii<(int)polygon.vertices.size(); ++ii)
			{
				DEBUG_MSG( "%f %f \n", polygon.vertices[ii].pos[0], polygon.vertices[ii].pos[1] );
			}
			DEBUG_MSG( "--\n");
		}
#endif 

		vertexCount = static_cast<uint>(polygon.vertices.size());
		
		if(pProgress_)
			pProgress_->onProgress(L"adjacencies", count++);

		for (v = 0; v < vertexCount; ++v)
		{
			polygon.vertices[v].adjNavPoly = 0;

			edge.to = polygon.vertices[v].pos;
			edge.from = polygon.vertices[(v + 1) % vertexCount].pos;
			edge.id = -1;

			// make sure our edges overlap in 3D
			for (edgeIter = edges_.lower_bound(edge);
				edgeIter != edges_.end() && (*edgeIter) == edge;
				edgeIter++)
			{
				PolygonDef& maybeAdj = polygons_[edgeIter->id-1];
				if (maybeAdj.maxHeight < polygon.minHeight) continue;
				if (maybeAdj.minHeight > polygon.maxHeight) continue;

#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
				if (p == 473 || p == 468)
				{
					DEBUG_MSG("maybeAdj minHeight, maxheight, %f %f\n", maybeAdj.minHeight, maybeAdj.maxHeight);
				}
				
				if (polygon.vertices[v].adjNavPoly != 0)
				{
					CRITICAL_MSG( "Here %d\n", p );
				}
#endif
				polygon.vertices[v].adjNavPoly = edgeIter->id;
				break;
			}
		}
	}
}

/**
 *	This method returns the number of waypoint polygons.
 *
 *	@return Number of polygons
 */ 
int WaypointGenerator::getPolygonCount() const
{
	return static_cast<int>(polygons_.size());
}

/**
 *	This method returns the number of BSP nodes.
 *
 *	@return Number of BSP nodes
 */ 
int WaypointGenerator::getBSPNodeCount() const
{
	return static_cast<int>(bsp_.size());
}

/**
 *	This method provides info on the indicated BSP node.
 *	If there is no such node then its parent is used.
 *	Each bit of cursor from lhs indicates the choice to be made at that depth.
 *	e.g. to find all nodes at depth 2, cursor should be in the range 0-3.
 *
 *	@return The discrepancy between the depth of the node provided
 *			and the depth of the node desired. i.e. 0 if that depth found.
 */ 
int WaypointGenerator::getBSPNode( int cursor, int depth, BSPNodeInfo & bni ) const
{
	BW_GUARD;

	BW::vector<Vector2> boundary;
	int bspIndex = 0;

	while (depth > 0)
	{
		int ni = (!(cursor&(1<<(depth-1)))) ?
			bsp_[bspIndex].front : bsp_[bspIndex].back;
		if (ni == -1) break;

		bspIndex = ni;
		depth--;
	}

	const BSPNode & bn = bsp_[bspIndex];

	PointDef p1, p2;
	Vector2 v1, v2;
	for (int a = 0; a < 8; a++)
	{
		if (!this->calcEdge( bspIndex, a, p1, p2 )) continue;
		if (a >= 4)
		{
			this->invertPoint(p1);
			this->invertPoint(p2);
		}

		this->pointToVertex(p1, v1);
		this->pointToVertex(p2, v2);

		if ((v1-v2).lengthSquared() > GENERATOR_FLOAT_FUDGE)
			boundary.push_back( v1 );
	}

	bni.boundary_.clear();
	int bs = static_cast<int>(boundary.size());
	for (int v = 0; v < bs; v++)
	{
		Vector2 tv = boundary[v];
		Vector2 pv = boundary[(v+bs-1)%bs] - tv;
		Vector2 nv = boundary[(v+1)%bs] - tv;
		float pa = atan2f( pv.x, pv.y );
		float na = atan2f( nv.x, nv.y );
		if (pa > na) { pa = na+pa; na = pa-na; pa = pa-na; }
		float ta = (na-pa > MATH_PI) ? (na+MATH_PI*2.f+pa)/2.f : (na+pa)/2.f;
		// finally, we want to inset along ta
		Vector2 iv = Vector2( sinf(ta), cosf(ta) );
		if (iv.x != 0 && iv.y != 0) iv *= sqrtf(2.f);
		bni.boundary_.push_back( tv + iv * 0.4f );
	}

	bni.minHeight_ = bn.minHeight;
	bni.maxHeight_ = bn.maxHeight;
	bni.internal_ = bn.front != -1 && bn.back != -1;
	bni.waypoint_ = bn.waypoint;

	return depth;
}


/**
 *	This method returns the number of vertices in a given polygon.
 *
 *	@param polygon	Zero-based index of the polygon.
 *
 *	@return Number of vertices
 */ 
int WaypointGenerator::getVertexCount(int polygon) const
{
	BW_GUARD;

	return static_cast<int>(polygons_[polygon].vertices.size());
}

/**
 *	This method returns the height of a given polygon.
 *
 *	@param polygon	Zero-based index of the polygon.
 *
 *	@return Height
 */ 
float WaypointGenerator::getMinHeight(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].minHeight;
}

/**
 *	This method returns the height of a given polygon.
 *
 *	@param polygon	Zero-based index of the polygon.
 *
 *	@return Height
 */ 
float WaypointGenerator::getMaxHeight(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].maxHeight;
}


/**
 *	This method returns the set of a given polygon.
 *
 *	@param polygon	Zero-based index of the polygon.
 *
 *	@return Set
 */ 
int WaypointGenerator::getSet(int polygon) const
{
	BW_GUARD;

	return polygons_[polygon].set;
}

/**
 *	This method returns the num of waypoint sets in this view
 *
 *	@return num of waypoint sets
 */ 
int WaypointGenerator::getSets() const
{
	BW_GUARD;

	return sets_;
}

/**
 *	This method returns the position of a vertex, and the ID of the adjacent
 *	waypoint if any. Note that IDs are One-based, not Zero-based. An ID of
 *	zero means there was no adjacency.
 *
 *	@param polygon		Zero-based index of the polygon
 *	@param vertex		Zero-based index of the vertex
 *	@param v			Position is returned here
 *	@param adjacency	Adjacency ID (One-based) is returned here.
 */
void WaypointGenerator::getVertex(int polygon, int vertex, Vector2& v,
	int& adjacency, bool & adjToAnotherChunk) const
{
	BW_GUARD;

	const PolygonDef & poly = polygons_[polygon];
	const VertexDef & vd = poly.vertices[vertex];
	v = vd.pos;
	adjacency = vd.adjNavPoly; 
	adjToAnotherChunk = vd.adjToAnotherChunk;
}

/**
 *	This method sets the adjacency info for the given vertex in the
 *	given polygon
 */
void WaypointGenerator::setAdjacency( int polygon, int vertex, int newAdj )
{
	BW_GUARD;

	polygons_[polygon].vertices[vertex].adjNavPoly = newAdj;
}


/**
 *	This method returns a single-byte adjacency mask
 *	for the given grid coordinate. It is only used for display purposes.
 */
uint8 WaypointGenerator::gridMask( int x, int z ) const
{
	BW_GUARD;

	uint index = x + gridX_ * z;

	uint8 adjMask = 0;
	for (int g = 0; g < 16; g++)
		for (int a = 0; a < 8; a++)
			if (adjGrids_[g][index].angle( a ) != 0)
				adjMask |= (1<<a);
	return adjMask;
}

/**
 *	This method returns the minimum point in the bounding box.
 *	The y coordinate is faked for now. 
 */
Vector3 WaypointGenerator::gridMin() const
{
	Vector3 gridMin = gridMin_;
	gridMin.y = -1000.0f;
	return gridMin;
}

/**
 *	This method returns the maximum point in the bounding box.
 *	The y coordinate is faked for now. 
 */	
Vector3 WaypointGenerator::gridMax() const
{
	Vector3 gridMax = gridMin_;
	gridMax.x += gridResolution_ * gridX_;
	gridMax.y = 1000.0f;
	gridMax.z += gridResolution_ * gridZ_;
	return gridMax;
}

/**
 *	This method polls at the given cursor in our grid.
 */
bool WaypointGenerator::gridPoll( int cursor, Vector3 & pt, Vector4 & ahgts ) const
{
	BW_GUARD;

	int g = cursor & 15; cursor >>= 4;
	int index = cursor;
	int x = index % gridX_;
	int z = index / gridX_;

	if (adjGrids_[g][index].all == 0) return false;

//	pt.set(
//		gridMin_.x + x * gridResolution_,
//		hgtGrids_[g][index],
//		gridMin_.z + z * gridResolution_ );
	pt.set( float(x), hgtGrids_[g][index], float(z) );
	
	ahgts.set( -10000.f, -10000.f, -10000.f, -10000.f );
	int og;
	if (z != gridZ_-1 && (og=adjGrids_[g][index].each.u))
		ahgts[0] = hgtGrids_[og][index+gridX_];
	if (z != gridZ_-1 && x != gridX_-1 && (og=adjGrids_[g][index].each.ur))
		ahgts[1] = hgtGrids_[og][index+gridX_+1];
	if (x != gridX_-1 && (og=adjGrids_[g][index].each.r))
		ahgts[2] = hgtGrids_[og][index+1];
	if (z != 0 && x != gridX_-1 && (og=adjGrids_[g][index].each.dr))
		ahgts[3] = hgtGrids_[og][index-gridX_+1];
	return true;
}

/**
 *	Returns true if the angle b is between a1 and a2.
 */ 
bool WaypointGenerator::isConvexJoint(int a1, int a2, int b) const
{
	if(a1 < a2)
		return (b >= a1) && (b < a2);
	else if(a2 < a1)
		return (b >= a1) || (b < a2);
	else
		return false;
}

/**
 *	Finds the vertex in a given polygon at the given location.
 *	Returns the index of the vertex if successful, or -1 otherwise.
 *
 *	@param polyIndex	Index of the polygon to search
 *	@param v			Position of the vertex
 *	@return				Index of the vertex, or -1 if not found
 */
int WaypointGenerator::findPolygonVertex(int polyIndex, const Vector2& v) const
{
	BW_GUARD;

	const PolygonDef& p = polygons_[polyIndex];

	for(unsigned int i = 0; i < p.vertices.size(); i++)
	{
		if(fabs(v.x - p.vertices[i].pos.x) < GENERATOR_FLOAT_FUDGE &&
		   fabs(v.y - p.vertices[i].pos.y) < GENERATOR_FLOAT_FUDGE)		
			return i;
	}

	return -1;
}

/**
 *	This method is called after two polygons have been joined. Since one
 *	polygon will no longer exist, all its adjacencies need to be told to
 *	link to the new polygon instead.
 *
 *	@param polyIndex	Index of the polygon to change
 *	@param oldAdj		Old ID of the adjacency
 *	@param newAdj		New ID of the adjacency
 */
void WaypointGenerator::fixupAdjacencies(int polyIndex, int oldAdj, int newAdj)
{
	BW_GUARD;

	PolygonDef& p = polygons_[polyIndex];

	for(unsigned int i = 0; i < p.vertices.size(); i++)
	{
		if(p.vertices[i].adjNavPoly == oldAdj)
			p.vertices[i].adjNavPoly = newAdj;
	}
}

/**
 *	This method attempts to join a polygon to one of its neighbours.
 *
 *	@param polyIndex	Index of the polygon
 *	@return				True if successful 
 */
bool WaypointGenerator::joinPolygon(int polyIndex)
{
	BW_GUARD;

	int i, firstVertex;
	int i1, size1, edgePrev1, edgeStart1, edgeEnd1;
   	int	i2, size2, edgePrev2, edgeStart2, edgeEnd2;
	bool singleAdjacency;

	i1 = polyIndex;
	PolygonDef& p1 = polygons_[i1];
	size1 = static_cast<int>(p1.vertices.size());

	// Check for empty polygons, they can exist.
	// (they have all been created by us however)
	if(p1.vertices.empty())
		return false;
	
	// Find the first vertex with a different angle.
	// This is to make sure we start at the beginning
	// of a new edge.
	firstVertex = 1;
	while( firstVertex < size1 &&
		p1.vertices[firstVertex].angles == p1.vertices[0].angles )
	{
		firstVertex++;
	}
	MF_ASSERT( firstVertex < size1 )

	edgeStart1 = firstVertex;
	edgeEnd1 = firstVertex;

	do
	{	
		// To consider an edge for joining, every point on it must
		// be the same angle (obviously), and they all must have the
		// same adjacent edge.
		singleAdjacency = (p1.vertices[edgeStart1].adjNavPoly > 0);

		// Keep going until we reach a vertex with a different angle.
		while(p1.vertices[edgeStart1].angles == p1.vertices[edgeEnd1].angles)
		{
			if(p1.vertices[edgeStart1].adjNavPoly != p1.vertices[edgeEnd1].adjNavPoly)
				singleAdjacency = false;

			edgeEnd1 = (edgeEnd1 + 1) % size1;
		}

		// Now we have a complete edge, (edgeStart <= v < edgeEnd)
		// If all points on it link to the same adjacency, we can consider
		// linking it.
		if (singleAdjacency)
		{
			i2 = p1.vertices[edgeStart1].adjNavPoly - 1;
			MF_ASSERT( i2 >= 0 && i2 < (int)polygons_.size() );
			PolygonDef& p2 = polygons_[i2];
			size2 = static_cast<int>(p2.vertices.size());
			
			edgeStart2 = this->findPolygonVertex(i2, p1.vertices[edgeEnd1].pos);
			edgeEnd2 = this->findPolygonVertex(i2, p1.vertices[edgeStart1].pos);

			if ( !(size2 > 0) )
			{
				ERROR_MSG( "Polygon %d from %d (%f,%f) to %d (%f,%f) links to "
					"zero sized polygon %d. Skipping.\n", i1,
					edgeStart1, p1.vertices[edgeStart1].pos.x, p1.vertices[edgeStart1].pos.y,
					edgeEnd1, p1.vertices[edgeEnd1].pos.x, p1.vertices[edgeEnd1].pos.y,
					i2 );
			}
			else if ( edgeStart2 == -1 || edgeEnd2 == -1 )
			{
				ERROR_MSG( "Couldn't find edge vertices from polygon %d"
					" in polygon %d. Skipping.\n", i1, i2 );

				DEBUG_MSG( "es1 %d es2 %d ee1 %d ee2 %d, fv %d, s1 %d s2 %d\n",
					edgeStart1, edgeStart2, edgeEnd1, edgeEnd2, firstVertex, size1, size2 );
				
				for (uint e = 0; e < p2.vertices.size(); e++)
				{
					DEBUG_MSG( " vertex %d at (%f,%f)\n",
						e, p2.vertices[e].pos.x, p2.vertices[e].pos.y );
				}
				DEBUG_MSG ( " ee1pos (%f,%f), es1pos (%f,%f)\n",
					p1.vertices[edgeEnd1].pos.x, p1.vertices[edgeEnd1].pos.y,
					p1.vertices[edgeStart1].pos.x, p1.vertices[edgeStart1].pos.y );
			}
			else
			{
				edgePrev1 = (edgeStart1 + size1 - 1) % size1;
				edgePrev2 = (edgeStart2 + size2 - 1) % size2;

				// also make sure they do not encompass too much height range
				float combinedMinHeight = min( p1.minHeight, p2.minHeight );
				float combinedMaxHeight = max( p1.maxHeight, p2.maxHeight );

				// TODO: Do not join if new waypoint would overlap another
				// (where it did not before due to height separation)
				if(isConvexJoint(p1.vertices[edgePrev1].angles, 
					p1.vertices[edgeStart1].angles, p2.vertices[edgeEnd2].angles) &&
				   isConvexJoint(p2.vertices[edgePrev2].angles, 
					p2.vertices[edgeStart2].angles, p1.vertices[edgeEnd1].angles) &&
				   combinedMaxHeight - combinedMinHeight <= MAX_HEIGHT_RANGE)
				{
					PolygonDef p3;

					for(i = edgeEnd1; i != edgeStart1; i = (i + 1) % size1)
					{
						p3.vertices.push_back( p1.vertices[i] );
					}

					for(i = edgeEnd2; i != edgeStart2; i = (i + 1) % size2)
					{
						p3.vertices.push_back( p2.vertices[i] );

						if(p2.vertices[i].adjNavPoly > 0)
						{
		//					dprintf( "Joining: Fixing up adjs on %d to point to %d instead of %d\n",
		//						p2.vertices[i].adjNavPoly - 1, i1, i2 );
							this->fixupAdjacencies(p2.vertices[i].adjNavPoly - 1, i2 + 1, i1 + 1);
						}
					}

					p3.minHeight = combinedMinHeight;
					p3.maxHeight = combinedMaxHeight;

					// fix up all the bsp indices too
					for (i = 0; i < int(bsp_.size()); i++)
					{
						if (bsp_[i].waypointIndex == i2)
							bsp_[i].waypointIndex = i1;
					}

					p1 = p3;
					p2.vertices.clear();
		//			dprintf( "Joined poly %d to %d; emptied %d\n", i2, i1, i2 );
					return true;
				}
			}
		}

		edgeStart1 = edgeEnd1;
	}
	while(edgeEnd1 != firstVertex);

	return false;
}

/**
 *	This method attempts to join as many polygons as possible to
 *	their neighbours.
 */
void WaypointGenerator::joinPolygons()
{
	BW_GUARD;

	unsigned int i, joinCount;
	unsigned int totalJoinCount = 0;

	do
	{
		joinCount = 0;
	
		for(i = 0; i < polygons_.size(); i++)
		{
			if(this->joinPolygon(i))
			{
				joinCount++;
				totalJoinCount++;
			}
		}
	}
	while(joinCount);
}

/**
 *	This method searches for the waypoint containing
 *	the given point. It returns -1 if not found.
 */
BW::vector<int> WaypointGenerator::findWaypoints( const Vector3 & v, float /*girth*/ ) const
{
	BW_GUARD;

	int bspIndex = 0;

	Vector2 v2( (v.x - gridMin_.x) / gridResolution_,
		(v.z - gridMin_.z) / gridResolution_ );
	while (bsp_[bspIndex].front != -1)
	{
		if( bsp_[bspIndex].front <= bspIndex ||
			bsp_[bspIndex].back <= bspIndex )
		{
			CRITICAL_MSG( "Error in BSP tree: node %d front %d back %d\n",
				bspIndex, bsp_[bspIndex].front, bsp_[bspIndex].back );
		}

		int angle = bsp_[bspIndex].splitNormal;
		float offset = bsp_[bspIndex].splitOffset;

		if (angle < 8)
		{
			if(g_normal[angle].dotProduct(v2) + offset > 0)
				bspIndex = bsp_[bspIndex].front;
			else
				bspIndex = bsp_[bspIndex].back;
		}
		else
		{
			bspIndex = bsp_[bspIndex].front;
		}
	}

	BW::vector<int> result;

	result.push_back( bsp_[bspIndex].waypointIndex );

	return result;
}


Vector3 WaypointGenerator::toVector3( const Vector2 & v, const PolygonDef & polygon,
									  Chunk * pChunk ) const
{
	BW_GUARD;

	int g;
	Vector3 p;

	p.x = v.x * gridResolution_ + gridMin_.x;
	p.z = v.y * gridResolution_ + gridMin_.z;
	for (g = 0; g < 16; g++)
	{
		p.y = hgtGrids_[g][(int)v.x + gridX_ * (int)v.y];
		if (p.y >= polygon.minHeight && p.y <= polygon.maxHeight) break;
	}
	if (g == 16) p.y = polygon.maxHeight + 0.01f;

	return pChunk->transformInverse().applyPoint(p);;
}


/**
 * Create polygons that extend one step (0.5m) out past unbound portal.
 * now creates new polygons, not extend old ones.
 */
void WaypointGenerator::extendThroughUnboundPortals( Chunk * pChunk )
{
	BW_GUARD;

	// 1. find all the unbound, extern portals in this chunk.

	ChunkBoundaries cbs = pChunk->bounds();

	BW::vector<ChunkBoundary::Portal *> ubPortals;
	ChunkBoundaries::iterator i = cbs.begin();
	for ( ; i != cbs.end(); ++i )
	{
		if( (*i)->unboundPortals_.size() > 0 ) 
		{
			for ( int j=0; j<(int)(*i)->unboundPortals_.size(); ++j )
			{
				if ( !(*i)->unboundPortals_[j]->isExtern() )
					continue;

				ubPortals.push_back( (*i)->unboundPortals_[j] );
			}
		}
	}

	if ( ubPortals.size() == 0 )
		return;

	BW::vector< PolygonDef > newPolys;


	// 2. identify edge start points that need to be extended
	// through portal. These are those where both ends of the
	// edge are < 0.5, from portal. Also record the offset 
	// from this point of the corresponding point through the
	// portal.

	// for each polygon
	for ( unsigned int p = 0; p < polygons_.size(); ++p )
	{
		PolygonDef& polygon = polygons_[p];
		unsigned int vertexCount = static_cast<uint>(polygon.vertices.size());

		BW::vector< std::pair<int,Vector2> > extendPoints;

		// for each edge in the navpoly
		// edge <-> start vertex of edge.
		for ( unsigned int v = 0; v < vertexCount; ++v )
		{
			VertexDef& cVertex = polygon.vertices[v]; // current vertex.
			VertexDef& nVertex = polygon.vertices[(v+1)%vertexCount]; // next vertex.
			
			// only check edges not already flagged as next to other chunk.
			if ( cVertex.adjToAnotherChunk )
				continue;
			
			// only check edges not adjacent to other navPolys in same set.
			if ( cVertex.adjNavPoly != 0 ) 
				continue;
	
			// don't check vertices on 45 degree angles.
			if ( cVertex.angles % 2 )
				continue;

			// edge positions transformed to world coords.
			Vector3 cTv = this->toVector3( cVertex.pos, polygon, pChunk );
			Vector3 nTv = this->toVector3( nVertex.pos, polygon, pChunk );

			BW::vector<ChunkBoundary::Portal *>::iterator pIter = ubPortals.begin();
			for ( ; pIter != ubPortals.end(); ++pIter ) 
			{

				if ( ((*pIter)->plane.distanceTo( cTv ) < 0.45) && 
					 ((*pIter)->plane.distanceTo( nTv ) < 0.45) )
				{

					// direction to extend outside portal 
					// (dir of line rotated 90 degrees anticlockwise).
					Vector2 extendDir( 
						-(nVertex.pos[1] - cVertex.pos[1]),
						(nVertex.pos[0] - cVertex.pos[0]) );
					extendDir.normalise();

					// remember to change.
					extendPoints.push_back( std::pair<int,Vector2>( v, extendDir ) );

				}
			}

		}

		if ( extendPoints.size() == 0 ) 
			continue;


		// 3. work out end points of edges that aren't already 
		// start points of edges in our list.

		BW::vector< std::pair<int,Vector2> > extraExtendPoints;

		// for each point.
		for ( int i=0; i<(int)extendPoints.size(); ++i ) 
		{
			// look at the other points. Is there a consecutive one? assume not.
			bool found = false;
			for ( int j=0; j<(int)extendPoints.size(); ++j ) 
			{
				if ( extendPoints[j].first == (extendPoints[i].first+1)%vertexCount ) 
				{
					// found consecutive point.
					found = true;
					break;
				}
			}
			// if didn't find next point, add to extra points list. 
			if (!found)
			{
				extraExtendPoints.push_back( 
					std::pair<int,Vector2>( (extendPoints[i].first+1)%vertexCount,
											 extendPoints[i].second) );
			}
		}


		// 4. enforce consecutive vertices.
		
		// 4a. that means only one end point.
		
		if ( extraExtendPoints.size() != 1 )
		{
			ERROR_MSG( "WaypointGenerator::extendThroughUnboundPortals:"
				"extraExtendPoints.size() != 1\n" );
			continue;
		}


		// 4b. and can step back to beginning from end, creating ordered 
		// (backwards) vertex list.
		
		BW::vector< std::pair<int,Vector2> > orderedExtendPoints;

		bool hadError = false;
		for ( int i=0; i<(int)extendPoints.size(); ++i ) 
		{
			bool found = false;
			for ( int j=0; j<(int)extendPoints.size(); ++j ) 
			{
				if ( extendPoints[j].first == 
						(extraExtendPoints[0].first-i-1+vertexCount)%vertexCount )
				{
					orderedExtendPoints.push_back( extendPoints[j] );
					found = true;
					continue;
				}
			}
			if ( !found ) 
			{
				hadError = true;
				ERROR_MSG( "WaypointGenerator::extendThroughUnboundPortals: "
					"non consecutive edges through portal.\n" );
				continue;
			}
		}

		if ( hadError )
			continue;


		// 5. now construct polygon.

		PolygonDef newPoly;
		newPoly.minHeight = polygon.minHeight;
		newPoly.maxHeight = polygon.maxHeight;
		newPoly.set = polygon.set; 

		// most right hand edge adjacent to existing polygon.
		{
			VertexDef toAdd = polygon.vertices[extraExtendPoints[0].first];
			
			toAdd.angles = 0;

			toAdd.adjNavPoly = p+1;
			polygon.vertices[orderedExtendPoints[0].first].adjNavPoly = 
				static_cast<int>(polygons_.size() + newPolys.size() + 1);
			
			toAdd.adjToAnotherChunk = false;
			polygon.vertices[orderedExtendPoints[0].first].adjToAnotherChunk = 
				false;
			
			newPoly.vertices.push_back( toAdd );
		}

		// remaining edges adjacent to existing polygon, and left hand vertical edge.
		for ( int i=0; i<(int)orderedExtendPoints.size(); ++i )
		{
			VertexDef toAdd = polygon.vertices[orderedExtendPoints[i].first];

			toAdd.angles = 0;

			toAdd.adjNavPoly = 0;
			toAdd.adjToAnotherChunk = false;

			if ( i < (int)orderedExtendPoints.size()-1 )
			{
				toAdd.adjNavPoly = p+1;
				polygon.vertices[orderedExtendPoints[i+1].first].adjNavPoly = 
					static_cast<int>(polygons_.size() + newPolys.size() + 1);
				polygon.vertices[orderedExtendPoints[i+1].first].adjToAnotherChunk =
					false;
			}

			newPoly.vertices.push_back( toAdd );
		}

		// edges adjacent to other chunks 
		for ( int i=(int)orderedExtendPoints.size()-1; i>=0; --i )
		{
			VertexDef toAdd = polygon.vertices[orderedExtendPoints[i].first];
			toAdd.pos += orderedExtendPoints[i].second;
			toAdd.angles = 0;
			toAdd.adjNavPoly = 0;
			toAdd.adjToAnotherChunk = true;
			newPoly.vertices.push_back( toAdd );
		}

		// right hand vertical edge. 
		{
			VertexDef toAdd = polygon.vertices[extraExtendPoints[0].first];
			toAdd.pos += orderedExtendPoints[0].second;
			toAdd.angles = 0;
			toAdd.adjToAnotherChunk = false;
			toAdd.adjNavPoly = 0;
			newPoly.vertices.push_back( toAdd );
		}

		newPolys.push_back( newPoly );

	}

	// 6. Now join up created polygons. 

	// TODO: this step. That it's not done won't break 
	// anything though.


	// 7. Add to existing list.

	for ( int i=0; i<(int)newPolys.size(); ++i )
		polygons_.push_back( newPolys[i] );
		
}


/**
 *	This method checks for edges that are entirely within another chunk,
 *	and marks them adjacent to that chunk. Now uses portal list from
 *  pointer to current chunk, rather than old method using adjacent 
 *  chunk list from boundary sections.
 */
void WaypointGenerator::determineEdgesAdjacentToOtherChunks( Chunk * pChunk, IPhysics* physics  )
{
	BW_GUARD;

	// initially assume edges are internal to the current chunk.
	// do this first as some vertices shared.
	for ( unsigned int p = 0; p < polygons_.size(); ++p )
	{
		for ( unsigned int v = 0; v < polygons_[p].vertices.size(); v++ )
		{
			polygons_[p].vertices[v].adjToAnotherChunk = false;
		}
	}

	BW::vector<int> polygonPortalParity(polygons_.size());

	// for each polygon
	for ( unsigned int p = 0; p < polygons_.size(); ++p )
	{

		PolygonDef& polygon = polygons_[p];
		unsigned int vertexCount = static_cast<uint>(polygon.vertices.size());

		// first check if it is a huge polygon covering the entire chunk
		const BoundingBox & b = pChunk->localBB();
		int outsides = 0;
		for (unsigned int v = 0; v < vertexCount; v++)
		{
			Vector3 p1 = this->toVector3(
				polygon.vertices[v].pos, polygon, pChunk );
			if (p1.x < b.minBounds().x && p1.z < b.minBounds().z) outsides |= 1;
			if (p1.x < b.minBounds().x && p1.z > b.maxBounds().z) outsides |= 2;
			if (p1.x > b.maxBounds().x && p1.z < b.minBounds().z) outsides |= 4;
			if (p1.x > b.maxBounds().x && p1.z > b.maxBounds().z) outsides |= 8;
		}
		if (outsides == 1+2+4+8)
		{
			for (unsigned int v = 0; v < vertexCount; v++)
				polygon.vertices[v].adjToAnotherChunk = true;

			DEBUG_MSG( "WaypointGenerator::determineEdgesAdjacentToOtherChunks:"
				" polygon %d completely surrounds chunk\n", p );
			// no point checking portal crossings below then
			continue;
		}

		for (unsigned int v = 0; v < vertexCount; ++v)
		{
			if (polygon.vertices[ v ].adjNavPoly > 0)
			{// we have connected to another nav poly, skip
				continue;
			}

			// vertex 
			Vector2 v1 = polygon.vertices[v].pos;
			Vector2 v2 = polygon.vertices[(v + 1) % vertexCount].pos;

			Vector3 p1 = this->toVector3( v1, polygon, pChunk );
			Vector3 p2 = this->toVector3( v2, polygon, pChunk );

			p1 = pChunk->transform().applyPoint( p1 );
			p2 = pChunk->transform().applyPoint( p2 );

			p1.y = p2.y = polygon.maxHeight + 1.f;

			float y;

			if (physics->findDropPoint( p1, y ))
			{
				p1.y = y;
			}

			if (physics->findDropPoint( p2, y ))
			{
				p2.y = y;
			}

			if (!pChunk->owns( p1 ) && !pChunk->owns( p2 ))
			{
				Vector3 p = ( p1 + p2 ) / 2;

				if (!pChunk->owns( p ))
				{
					polygon.vertices[ v ].adjToAnotherChunk = true;
				}
			}
		}
	}

	for ( int p=0; p<(int)polygons_.size(); ++p )
	{
		if ( polygonPortalParity[p]%2 == 1 )
		{
			WARNING_MSG( "WaypointGenerator::determineEdgesAdjacentToOtherChunks: "
				"polygon %d crosses portal boundaries an odd number (%d) of times\n", p, polygonPortalParity[p] );
		}
	}

}


/**
 *	This method streamlines the generated navPolys by collapsing
 *	shared edges, and removing empty waypoints.
 */
void WaypointGenerator::streamline()
{
	BW_GUARD;

	BW::vector<int>	idRemap;
	uint newID = 0;
	idRemap.push_back( newID++ );
	uint lostEdges = 0;
	uint pointEdges = 0;

	// first remove empty polygons or collapse shared edges
	for ( uint p = 0; p < polygons_.size(); p++ )
	{
		PolygonDef& polygon = polygons_[p];

		if ( polygon.vertices.empty() )
		{
			idRemap.push_back( -1 );

			polygons_.erase( polygons_.begin() + p );
			p--;
		}
		else
		{
			idRemap.push_back( newID++ );

			// find doubled adjacencies
			int lastAdj = polygon.vertices.back().adjNavPoly;
			for (uint v = 0; v < polygon.vertices.size(); v++)
			{
				int thisAdj = polygon.vertices[v].adjNavPoly;
				if (thisAdj == lastAdj && thisAdj > 0)
				{
					// delete our side only (other side found by symmetry)
					polygon.vertices.erase( polygon.vertices.begin() + v );
					v--;
					lostEdges++;
				}
				lastAdj = thisAdj;
			}

			// find zero-length adjacencies
			for (uint v = 0; v < polygon.vertices.size(); v++)
			{
				Vector2 thisPt = polygon.vertices[v].pos;
				Vector2 nextPt = polygon.vertices[
					(v+1)%polygon.vertices.size()].pos;
				if (nextPt == thisPt)
				{
					polygon.vertices.erase( polygon.vertices.begin() + v );
					v--;
					pointEdges++;
				}
			}
		}
	}

	// now remap the adjacencies after the id space contraction
	for (uint p = 0; p < polygons_.size(); p++)
	{
		PolygonDef& polygon = polygons_[p];
		for (uint v = 0; v < polygon.vertices.size(); v++)
		{
			int oldAdj = polygon.vertices[v].adjNavPoly;
			if (oldAdj > 0)
			{
				int newAdj = idRemap[ oldAdj ];
				if (newAdj == -1)
				{
					ERROR_MSG( "Polygon %d has edge %d to empty polygon %d!\n",
						p, v, oldAdj );
					newAdj = 0;
				}
				polygon.vertices[v].adjNavPoly = newAdj;
			}
		}
	}

	// there should have been an even number of edges removed
	if ( lostEdges & 1 )
	{
		ERROR_MSG( "There should have been an even number of edges removed after streamlining." );
		ERROR_MSG( "\tConsider to use custom BSP if the BSP in this chunk is too complex." );
	}
}


/**
 *	This method determines what set each polygon is in, and removes
 *	isolated polygons. A polygon is isolated if it isn't connected
 *	(directly or indirectly through other polygons) to an adjacent chunk.
 *
 *	Note that this routine assumes all adjacencies are bi-directional!
 */
void WaypointGenerator::calculateSetMembership(
	const BW::vector<Vector3> & frontierPts,
	const BW::string & chunkName ) // chunkName used for debugging message.
{
	BW_GUARD;

	// convert world-coord frontier pts into our local coords
	BW::vector<Vector3> localFPs;
	BW::vector<Vector3>::const_iterator fit;
	for (fit = frontierPts.begin(); fit != frontierPts.end(); ++fit)
	{
		localFPs.push_back( Vector3(
			(fit->x - gridMin_.x) / gridResolution_,
			fit->y,
			(fit->z - gridMin_.z) / gridResolution_ ) );
	}


	int setNext = 0;

	// go through all the polygons looking for frontier ones
	// also keep track of the biggest one in case we don't find any
	uint pMax = -1;
	float areaMax = 0.f;
	for (uint p = 0; p < polygons_.size(); p++)
	{
		PolygonDef& polygon = polygons_[p];
		if (polygon.set != 0) continue;

		bool frontier = false;
		Vector2 pmin( 10000.f, 10000.f );
		Vector2 pmax(-10000.f,-10000.f );
		for (uint v = 0; v < polygon.vertices.size(); v++)
		{		// is it frontier 'coz it leads to another chunk?
			if (polygon.vertices[v].adjToAnotherChunk)
			{
				frontier = true;
			}
			if (pmin.x > polygon.vertices[v].pos.x)
			{
				pmin.x = polygon.vertices[v].pos.x;
			}
			if (pmin.y > polygon.vertices[v].pos.y)
			{
				pmin.y = polygon.vertices[v].pos.y;
			}
			if (pmax.x < polygon.vertices[v].pos.x)
			{			
				pmax.x = polygon.vertices[v].pos.x;
			}
			if (pmax.y < polygon.vertices[v].pos.y)
			{
				pmax.y = polygon.vertices[v].pos.y;
			}
		}
		if (!frontier)
		{		// is it frontier 'coz a frontier pt is in it?
			for (fit = localFPs.begin(); fit != localFPs.end(); fit++)
			{
				if (polygon.ptNearEnough( *fit ))
				{
					frontier = true;
					break;
				}
			}
		}

		float area = (pmax.x-pmin.x)*(pmax.y-pmin.y);
		if (pMax == -1 || areaMax < area)
		{
			pMax = p;
			areaMax = area;
		}

		if (!frontier)
		{
			if (setNext > 0 || p < polygons_.size()-1) continue;
			
			INFO_MSG( "WaypointGenerator::calculateSetMembership: "
				"Forming from biggest polygon %d because "
				"none are on the frontier to other chunks\n", pMax );
			p = pMax;
		}

		setNext += 1;
		//dprintf( "Set %d:\n", polygon.set );

		// do an adjacency search to find all those in the same set as us
		BW::vector<int>	indexStack;
		indexStack.push_back( p );
		while (!indexStack.empty())
		{
			int idx = indexStack.back();
			PolygonDef & spoly = polygons_[indexStack.back()];
			indexStack.pop_back();

			if (spoly.set != 0)
			{
				MF_ASSERT( spoly.set == setNext );
				continue;
			}
			spoly.set = setNext;

			for (uint v = 0; v < spoly.vertices.size(); ++v)
			{
				int aidx = spoly.vertices[v].adjNavPoly;
				if (aidx > 0)
				{
					if (polygons_[aidx-1].set != 0)
					{
						//MF_ASSERT( polygons_[aidx-1].set == setNext );
						if (polygons_[aidx-1].set != setNext)
						{

#ifdef DEBUG_UNRECIPROCATED_ADJACENCY
							// I've changed the critical_msg to a warning_msg -
							// the problem is very rare, and isn't critical.
							// It implies a problem before we get to this point,
							// however fixing this isn't a priority right now.
							DEBUG_MSG( "original id %d\n", spoly.originalId );
							DEBUG_MSG( "original id2 %d\n", polygons_[aidx-1].originalId );

							CRITICAL_MSG( "Polygon %d in new set %d has "
								"unreciprocated connection to p %d in set %d\n",
								idx+1, setNext, aidx, polygons_[aidx-1].set );

							spoly.vertices[v].adjNavPoly = 0;
							DEBUG_MSG( "v, aidx: %d %d\n", v, aidx );
							DEBUG_MSG( "473 height: %f %f\n", spoly.minHeight, spoly.maxHeight );
							for (int ii=0; ii<(int)spoly.vertices.size(); ++ii)
							{
								DEBUG_MSG( "spoly: %d %f %f\n", ii, spoly.vertices[ii].pos[0],
									spoly.vertices[ii].pos[1] );
							}
		
							DEBUG_MSG( "---\n\n468 height: %f %f\n", polygons_[aidx-1].minHeight, polygons_[aidx-1].maxHeight );
							for (int ii=0; ii<(int)spoly.vertices.size(); ++ii)
							{
								DEBUG_MSG( "spoly: %d %f %f\n", ii, polygons_[aidx-1].vertices[ii].pos[0],
									polygons_[aidx-1].vertices[ii].pos[1] );
							}

#else
		
							WARNING_MSG(
								"Polygon %d in chunk %s new set %d has "
								"unreciprocated connection to p %d in set %d\n",
								idx+1, chunkName.c_str(), setNext, aidx, polygons_[aidx-1].set );

							spoly.vertices[v].adjNavPoly = 0;
#endif
						}
					
						continue;
					}
					
					indexStack.push_back( aidx-1 );
				}
			}
		}

		// if this one's being processed because there were no frontier
		// polygons, then stop now; we only want one set like that.
		if (!frontier) break;
	}

	BW::vector<int>	idRemap;
	uint newID = 0;
	idRemap.push_back( newID++ );
	uint nRemoved = 0;

	// remove isolated polygons
	for (uint p = 0; p < polygons_.size(); p++)
	{
		if (polygons_[p].set == 0)
		{
			idRemap.push_back( -1 );

			polygons_.erase( polygons_.begin() + p );
			p--;
			nRemoved++;
		}
		else
		{
			idRemap.push_back( newID++ );
		}
	}

	// now remap the adjacencies after the id space contraction
	for (uint p = 0; p < polygons_.size(); p++)
	{
		PolygonDef& polygon = polygons_[p];
		for (uint v = 0; v < polygon.vertices.size(); v++)
		{
			int oldAdj = polygon.vertices[v].adjNavPoly;
			if (oldAdj > 0)
			{
				int newAdj = idRemap[ oldAdj ];
				MF_ASSERT( newAdj != -1 );
				if (newAdj == -1) newAdj = 0;
				polygon.vertices[v].adjNavPoly = newAdj;
			}
		}
	}

	if (setNext == 0)
	{
		MF_ASSERT( nRemoved + polygons_.size() == 0 );
	}

	sets_ = setNext;
}

BW_END_NAMESPACE
