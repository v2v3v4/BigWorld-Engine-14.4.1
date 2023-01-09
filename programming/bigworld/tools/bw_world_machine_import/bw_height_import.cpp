#include "pch.hpp"
#include "bw_height_import.hpp"
#include "core\HField.h"
#include "math/vector4.hpp"
#include "terrain/height_map_compress.hpp"
#include "terrain/terrain_data.hpp"
#include "resmgr/bwresource.hpp"
#include <commdlg.h>



namespace
{

/*
 *	Function exposed to the world machine GUI to reset chunk scale
 */
void BWHeightImport_resetChunkScale( void * pDevice )
{
	BWHeightImport* pHeightImport = (BWHeightImport*)pDevice;
	pHeightImport->resetChunkScale();
}

/*
 *	Function exposed to the world machine GUI to select space
 */
void BWHeighImport_selectSpace( void * pDevice )
{
	BWHeightImport* pHeightImport = (BWHeightImport*)pDevice;
	pHeightImport->selectSpace();
}

/*
 *	Creator functions used by world machine
 */
Device *BWHeightImportMaker() { return new BWHeightImport; };
void BWHeightImportKiller(Device *spr) { delete spr; };

}

/**
 *	Constructor
 */
BWHeightImport::BWHeightImport(void)
{
	lifeptrs.maker = BWHeightImportMaker;
	lifeptrs.killer = BWHeightImportKiller;
	strncpy(lifeptrs.nametag, "BWHI", 4);

	SetLinks(0, 1);

	// Parameters
	AddParam(Parameter(HEIGH_SCALE_PROPERTY, 1.f, 0.0f, 10000.0f));
	AddParam(Parameter(CHUNK_SCALE_PROPERTY, 100.f, 0.0f, 10000.0f));
	AddParam(Parameter("Reset Chunk Scale", (VPtrType)&BWHeightImport_resetChunkScale));
	AddParam(Parameter("Select space", (VPtrType)&BWHeighImport_selectSpace));

	// Help strings
	params.GetParam(0)->setHelpString("Height scale of the terrain");
	params.GetParam(1)->setHelpString("Chunk scale for the terrain");
	params.GetParam(2)->setHelpString("Reset the Chunk scale of the terrain");
	params.GetParam(3)->setHelpString("Select a space");

}


BWHeightImport::~BWHeightImport(void)
{

}


/**
 *	This method loads the current state of the BWHeightImport object 
 *	from a world machine project
 */
bool BWHeightImport::Load(std::istream &in) {
	char tag[5];
	in.read( tag, 4);
	if (strncmp(tag, lifeptrs.nametag, 4) == 0) 
	{
		int ver = 0;
		in.read((char*) &ver, 1);

		char spaceRoot[257];
		spaceRoot[256] = 0;
		in.read( spaceRoot, 256 );

		spaceHelper_.init( spaceRoot + BW::string("space.settings") );

		return Generator::Load(in);
	}
	else
		return false;
};


/**
 *	This method saves the current state of the BWHeightImport object 
 *	to a world machine project
 */
bool BWHeightImport::Save(std::ostream &out) 
{
	out.write( lifeptrs.nametag, 4);
	int ver = 1;
	out.write((char*) &ver, 1);

	char spaceRoot[256];
	bw_snprintf( spaceRoot, 256, spaceHelper_.spaceRoot().c_str() );
	out.write( spaceRoot, 256 );

	return Generator::Save(out);
};

namespace
{

/**
 *	This function loads a height map from the provided cdata section
 *	@param pCDataSection the cdata section
 *	@param oHeights the height image to output the heights to
 *	@return true if the height map is successfully loaded
 */
/* static */
bool  loadHeightMap( BW::DataSectionPtr pCDataSection, BW::Moo::Image<float>& oHeights )
{
	if (!pCDataSection)
	{
		return false;
	}

	bool ret = false;

	BW::BinaryPtr pHeightBlock = pCDataSection->readBinary( "terrain2/heights" );

	if (pHeightBlock)
	{
		const BW::Terrain::HeightMapHeader* pHeader = 
			(const BW::Terrain::HeightMapHeader*)pHeightBlock->data();

		if (pHeader->magic_ == BW::Terrain::HeightMapHeader::MAGIC &&
			pHeader->version_ == BW::Terrain::HeightMapHeader::VERSION_ABS_QFLOAT)
		{
			BW::BinaryPtr compData = 
				new BW::BinaryBlock((void *)(pHeader + 1), 
				pHeightBlock->len() - sizeof(*pHeader), "Terrain/HeightMap2/BinaryBlock");
				
			if (BW::Terrain::decompressHeightMap(compData, oHeights))
			{
				ret = true;
			}
		}
	}

	return ret;
}

}


/**
 *	This method actives the height import device, i.e. it creates the
 *	world machine height map from the bigworld space
 */
bool BWHeightImport::Activate(BuildContext &context) 
{
	// Create a new heightfield using current specified world size
	HField *map = GetNewHF(context.GetWorldSize());

	map->Clear();

	BW::Moo::Image<float> heights;

	if (spaceHelper_.valid())
	{
		const SizeData& sd = context.GetWorldSize();

		// Calculate scale from BigWorld height units to World Machine
		float heightScale = ParmFRef( HEIGH_SCALE_PROPERTY );
		heightScale *= sd.vert_scale / 1000.f;

		// Get the offset of the lower corner of the WM machine height map in metres
		CoordF destOffset = context.GetPanLoc();
		destOffset *= WMGlobal->km_wm_relation * 1000.f;

		// The size of the world machine height map in metres
		CoordF destSize = sd.scalesize * WMGlobal->km_wm_relation * 1000.f;

		// Get the user defined chunk scale
		float chunkScale = ParmFRef( CHUNK_SCALE_PROPERTY );

		// Calculate the overlap between the World Machine height map and
		// the BigWorld space
		int chunkXMin = max( spaceHelper_.xMin(), 
			int( floorf( destOffset.x / chunkScale ) ) );
		int chunkXMax = min( spaceHelper_.xMax(), 
			int( floorf( (destOffset.x + destSize.x) / chunkScale ) ) );

		int chunkZMin = max( spaceHelper_.zMin(), 
			int( floorf( destOffset.y / chunkScale ) ) );
		int chunkZMax = min( spaceHelper_.zMax(), 
			int( floorf( (destOffset.y + destSize.y) / chunkScale ) ) );

		// Calculate the number of steps to perform while building the height map
		int numBuildSteps = (chunkXMax - chunkXMin + 1) * map->h();
		int buildProgress = 0;

		// Iterate over the chunks to load up their height maps
		for (int chunkZ = chunkZMin; chunkZ <= chunkZMax; ++chunkZ)
		{
			for (int chunkX = chunkXMin; chunkX <= chunkXMax; ++chunkX)
			{
				BW::Moo::Image<float> heights;

				if (loadHeightMap( spaceHelper_.getCDataForChunk( chunkX, chunkZ ), heights ))
				{

					// The offset of the bottom left corner of the chunk
					CoordF chunkCorner( float(chunkX) * chunkScale, float(chunkZ) * chunkScale );
					chunkCorner -= destOffset;

					// The start of the chunk in the destination map
					float xChunkStart = (chunkCorner.x / destSize.x) * float(map->w());
					float yChunkStart = (chunkCorner.y / destSize.y) * float(map->h());

					// The pixel indexes this chunk covers in the destination height map
					int xStart = int(ceilf( xChunkStart));
					int yStart = int(ceilf(yChunkStart));
					int xEnd = int(ceilf( ((chunkCorner.x + chunkScale) / destSize.x) * float(map->w()) ));
					int yEnd = int(ceilf( ((chunkCorner.y + chunkScale) / destSize.y) * float(map->h()) ));

					xStart = max( xStart, 0 );
					yStart = max( yStart, 0 );
					xEnd = min( xEnd, map->w() );
					yEnd = min( yEnd, map->h() );

					// Calculate the mapping from one pixel in the destination height map to the source height map
					Vector2 gradient( (float( heights.width() - 5) / chunkScale) / (float( map->w() ) / destSize.x),
						(float( heights.height() - 5) / chunkScale) / (float( map->h() ) / destSize.y) );

					// Calculate the offset into the source height map that maps to 0,0 in the destination height map
					CoordF cornerOffset( -xChunkStart * gradient.x + 2.f, -yChunkStart * gradient.y + 2.f );

					for (int y = yStart; y < yEnd; ++y)
					{
						for (int x = xStart; x < xEnd; ++x)
						{
							(*map)[Coord(x,y)] = heightScale * 
								heights.getBilinear( 
								cornerOffset.x + float(x) * gradient.x, 
								cornerOffset.y + float(y) * gradient.y );
						}

						context.ReportProgress( this, ++buildProgress, numBuildSteps );
					}
				}
			}
		}
	}


	// pass the heightfield to the output stage.
	StoreData(map,0, context); 

	return true;
}

/**
 *	This method resets the chunk scale to the space default
 */
void BWHeightImport::resetChunkScale()
{
	Parameter* pChunkScale = params.GetParam( CHUNK_SCALE_PROPERTY );
	if (pChunkScale)
	{
		pChunkScale->setFloat( spaceHelper_.chunkSize() );
	}
}


/**
 *	This method brings up a dialog box that lets you choose a space to load
 */
void BWHeightImport::selectSpace()
{
	OPENFILENAME	ofn;
	memset( &ofn, 0, sizeof(ofn) );

	char * filters = "Space Settings\0space.settings\0\0";

	char filename[512] = "";

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = filters;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = "Select space";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR | OFN_READONLY | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "settings";

	if (GetOpenFileName( &ofn ))
	{
		if (strlen(filename) != 0)
		{
			this->spaceHelper_.init( filename );
		}
	}
}

// Static variable definitions
const char* BWHeightImport::HEIGH_SCALE_PROPERTY = "Height Scale";
const char* BWHeightImport::CHUNK_SCALE_PROPERTY = "Chunk Scale";

