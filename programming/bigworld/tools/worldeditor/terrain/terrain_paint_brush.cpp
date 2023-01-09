#include "pch.hpp"

#include "terrain_paint_brush.hpp"

#include "worldeditor/terrain/terrain_texture_utils.hpp"

#include "terrain/terrain_texture_layer.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( TerrainPaintBrush )

PY_BEGIN_METHODS( TerrainPaintBrush )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainPaintBrush )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( TerrainPaintBrush )


namespace
{
	/**
	 *	This reads noise values out of a DataSection (usually for a brush).
	 *
	 *	@param pDataSection	The DataSection to read from.
	 *	@param noise		This is set with the noise values.
	 *	@return				True if the noise could be read, false
	 *						otherwise.
	 */
	bool readNoise( DataSectionPtr pDataSection, SimplexNoise & noise)
	{
		BW_GUARD;

		if (!pDataSection)
		{
			return false;
		}

		noise = SimplexNoise(); // Clear existing values

		// Get the octave DataSections:
		BW::vector< DataSectionPtr > octaveSections;
		pDataSection->openSections( "octave", octaveSections );

		// Read the individual octaves:
		SimplexNoise::OctaveVec octaves;
		for (size_t i = 0; i < octaveSections.size(); ++i)
		{
			DataSectionPtr pOctaveSect = octaveSections[ i ];
			SimplexNoise::Octave octave;
			octave.waveLength_	= pOctaveSect->readFloat( "waveLength", 1.0f );
			octave.weight_	    = pOctaveSect->readFloat( "weight"    , 0.0f );
			octave.seed_		= pOctaveSect->readInt  ( "seed"      , 0    );
			octaves.push_back( octave );
		}

		// Set the octaves:
		noise.octaves( octaves );

		return true;
	}


	/**
	 *	This writes noise values to a DataSection (usually for a brush).
	 *
	 *	@param pDataSection	The DataSection to write to.
	 *	@param noise		This is the noise values to write.
	 *	@return				True if the noise could be written, false
	 *						otherwise.
	 */
	bool writeNoise( DataSectionPtr pDataSection, const SimplexNoise & noise )
	{
		BW_GUARD;

		if (!pDataSection)
		{
			return false;
		}

		// Clear any existing data:
		pDataSection->deleteSections( "octave" );

		const SimplexNoise::OctaveVec & octaves = noise.octaves();
		for (size_t i = 0; i < octaves.size(); ++i)
		{
			const SimplexNoise::Octave & octave = octaves[ i ];
			DataSectionPtr octaveSec = 
				pDataSection->newSection( "octave" );
			octaveSec->writeFloat( "waveLength", octave.waveLength_ );
			octaveSec->writeFloat( "weight"    , octave.weight_     );
			octaveSec->writeInt  ( "seed"      , octave.seed_       );
		}

		return true;
	}
}


/**
 *	This is the TerrainPaintBrush default constructor.
 */
TerrainPaintBrush::TerrainPaintBrush( PyTypeObject *pType /*= &s_type_*/ ):
    PyObjectPlus( pType ),
	strength_( 100.0f ),
	size_( 10.0f ),
	opacity_( 255 ),
	uvLocked_( true ),
	heightMask_( false ),
	h1_( 0.0f ),
	h2_( 0.0f ),
	h3_( 0.0f ),
	h4_( 0.0f ),
	slopeMask_( false ),
	s1_( 0.0f ),
	s2_( 0.0f ),
	s3_( 0.0f ),
	s4_( 0.0f ),
	textureMask_( false ),
	textureMaskIncludeProj_( false ),
	textureMaskIncludeBump_( false ),
	textureMaskInvert_( false ),
    noiseMask_( false ),
	noiseMinSat_( 0.0f ),
	noiseMaxSat_( 1.0f ),
	noiseMinStrength_( 0.0f ),
	noiseMaxStrength_( 1.0f ),
	importMask_( false ),
	importMaskTL_( Vector2::zero() ),
	importMaskBR_( Vector2::zero() ),
	importMaskAdd_( 0.0f ),
	importMaskMul_( 1.0f / std::numeric_limits< uint16 >::max() ), // full strength
	maxLayerLimit_( false ),
	maxLayers_( 4 )
{
	Terrain::TerrainTextureLayer::defaultUVProjections(
		paintUProj_, paintVProj_ );

	textureMaskUProj_ = paintUProj_;
	textureMaskVProj_ = paintVProj_;
}


/**
 *	This loads the brush from a DataSection.
 *
 *	@param pDataSection	The DataSection to load from.
 *	@return				True if the brush was successfully loaded, false
 *						otherwise.
 */
bool TerrainPaintBrush::load( DataSectionPtr pDataSection )
{
	BW_GUARD;

	if (!pDataSection)
	{
		return false;
	}

	Vector4 defaultUProj, defaultVProj;
	Terrain::TerrainTextureLayer::defaultUVProjections( defaultUProj, defaultVProj );

	BW::string brushType   = pDataSection->readString ( "type" );
	if (brushType != "TerrainPainting")
	{
		return false;
	}

	paintTexture_			= pDataSection->readString ( "texture"                   );
	paintBumpTexture_		= pDataSection->readString ( "bumpTexture"				 );
	strength_				= pDataSection->readFloat  ( "strength"                  );
	size_					= pDataSection->readFloat  ( "size"                      );
	paintUProj_				= pDataSection->readVector4( "uProjection", defaultUProj );
	paintVProj_				= pDataSection->readVector4( "vProjection", defaultVProj );
	opacity_				= (uint8)pDataSection->readInt( "opacity", 255 );
	uvLocked_				= pDataSection->readBool   ( "uvLocked"    , true        );

	heightMask_				= pDataSection->readBool ( "heightMask"   , false );
	h1_						= pDataSection->readFloat( "heightMask/h1", 0.0f  );
	h2_						= pDataSection->readFloat( "heightMask/h2", 0.0f  );
	h3_						= pDataSection->readFloat( "heightMask/h3", 0.0f  );
	h4_						= pDataSection->readFloat( "heightMask/h4", 0.0f  );

	slopeMask_				= pDataSection->readBool ( "slopeMask"   , false );
	s1_						= pDataSection->readFloat( "slopeMask/s1", 0.0f  );
	s2_						= pDataSection->readFloat( "slopeMask/s2", 0.0f  );
	s3_						= pDataSection->readFloat( "slopeMask/s3", 0.0f  );
	s4_						= pDataSection->readFloat( "slopeMask/s4", 0.0f  );

	textureMask_			= pDataSection->readBool   ( "textureMask"            , false        );
	textureMaskIncludeProj_ = pDataSection->readBool   ( "textureMask/includeProj", false        );
	textureMaskIncludeBump_ = pDataSection->readBool   ( "textureMask/includeBump", false		 );
	textureMaskUProj_		= pDataSection->readVector4( "textureMask/uProjection", defaultUProj );
	textureMaskVProj_		= pDataSection->readVector4( "textureMask/vProjection", defaultVProj );
	textureMaskTexture_		= pDataSection->readString ( "textureMask/texture"                   );
	bumpTextureMaskTexture_	= pDataSection->readString ( "textureMask/bumpTexture"				 );
	textureMaskInvert_		= pDataSection->readBool   ( "textureMask/invert"     , false        );

	noiseMask_				= pDataSection->readBool ( "noiseMask"            , false );
	noiseMinSat_			= pDataSection->readFloat( "noiseMask/minSat"     , 0.0f  );
	noiseMaxSat_			= pDataSection->readFloat( "noiseMask/maxSat"     , 1.0f  );
	noiseMinStrength_		= pDataSection->readFloat( "noiseMask/minStrength", 0.0f  );
	noiseMaxStrength_		= pDataSection->readFloat( "noiseMask/maxStrength", 1.0f  );
	if (noiseMask_)
	{
		DataSectionPtr pNoiseSection = 
			pDataSection->openSection( "noiseMask/noise" );
		if (pNoiseSection)
		{
			readNoise( pNoiseSection, noise_ );
		}
	}

	// We cannot save the import mask, it doesn't make sense

	maxLayerLimit_			= pDataSection->readBool( "maxLayerLimit", false );
	maxLayers_				= pDataSection->readInt ( "maxLayers"    , 4     );

	return true;
}



/**
 *	This saves the brush to a DataSection.
 *
 *	@param pDataSection	The DataSection to save to.
 *	@return				True if the brush was successfully saved, false
 *						otherwise.
 */
bool TerrainPaintBrush::save( DataSectionPtr pDataSection ) const
{
	BW_GUARD;

	if (!pDataSection)
	{
		return false;
	}

	pDataSection->writeString ( "type"        , "TerrainPainting" );
	pDataSection->writeString ( "texture"     , paintTexture_     );
	pDataSection->writeString ( "bumpTexture" ,	paintBumpTexture_ );
	pDataSection->writeFloat  ( "strength"    , strength_         );
	pDataSection->writeFloat  ( "size"        , size_             );
	pDataSection->writeVector4( "uProjection" , paintUProj_       );
	pDataSection->writeVector4( "vProjection" , paintVProj_       );
	pDataSection->writeInt    ( "opacity"     , opacity_          );
	pDataSection->writeBool   ( "uvLocked"    , uvLocked_         );

	pDataSection->writeBool( "heightMask", heightMask_ );
	if (heightMask_)
	{
		pDataSection->writeFloat( "heightMask/h1", h1_);
		pDataSection->writeFloat( "heightMask/h2", h2_);
		pDataSection->writeFloat( "heightMask/h3", h3_);
		pDataSection->writeFloat( "heightMask/h4", h4_);
	}

	pDataSection->writeBool( "slopeMask", slopeMask_ );
	if (slopeMask_)
	{
		pDataSection->writeFloat( "slopeMask/s1" , s1_ );
		pDataSection->writeFloat( "slopeMask/s2" , s2_ );
		pDataSection->writeFloat( "slopeMask/s3" , s3_ );
		pDataSection->writeFloat( "slopeMask/s4" , s4_ );
	}

	pDataSection->writeBool( "textureMask", textureMask_ );
	if (textureMask_)
	{
		pDataSection->writeBool   ( "textureMask/includeProj", textureMaskIncludeProj_ );
		pDataSection->writeBool   ( "textureMask/includeBump", textureMaskIncludeBump_ );
		pDataSection->writeVector4( "textureMask/uProjection", textureMaskUProj_	   );
		pDataSection->writeVector4( "textureMask/vProjection", textureMaskVProj_	   );
		pDataSection->writeString ( "textureMask/texture"    , textureMaskTexture_     );
		pDataSection->writeString ( "textureMask/bumpTexture", bumpTextureMaskTexture_ );
		pDataSection->writeBool   ( "textureMask/invert"     , textureMaskInvert_      );
	}

	pDataSection->writeBool( "noiseMask", noiseMask_ );
	if (noiseMask_)
	{
		pDataSection->writeFloat( "noiseMask/minSat"     , noiseMinSat_	     );
		pDataSection->writeFloat( "noiseMask/maxSat"     , noiseMaxSat_	     );
		pDataSection->writeFloat( "noiseMask/minStrength", noiseMinStrength_ );
		pDataSection->writeFloat( "noiseMask/maxStrength", noiseMaxStrength_ );
		if (noiseMask_)
		{
			DataSectionPtr pNoiseSection = 
				pDataSection->openSection( "noiseMask/noise", true );
			writeNoise( pNoiseSection, noise_ );
		}
	}

	// We cannot save the import mask, it doesn't make sense

	pDataSection->writeBool( "maxLayerLimit", maxLayerLimit_ );
	pDataSection->writeInt ( "maxLayers"    , maxLayers_     );

	return true;
}


/**
 *	Assign the values of given brush to this
 *
 *	@param			The brush to copy values from
 */
void TerrainPaintBrush::assign( const TerrainPaintBrush& that )
{
	DataSectionPtr ds = new XMLSection( "brush" );

	that.save( ds );
	this->load( ds );
}


/**
 *	This suggests a filename for the brush.  It does not include the extension.
 *
 *	@return			A suggested filename that can be used to save the brush.
 */
BW::string TerrainPaintBrush::suggestedFilename() const
{
	BW_GUARD;

	BW::string result;

	// Use the texture name:
	BW::StringRef texureFile = BWResource::getFilename( paintTexture_ );
	texureFile = BWResource::removeExtension( texureFile );
	if (texureFile.empty())
	{
		result += "empty";
	}
	else
	{
		result += texureFile.to_string();
	}

	// Add some info for the height mask:
	if (heightMask_)
	{
		result += LocaliseUTF8(L"_h%0_%1", h2_, h3_ );
	}

	// Add some info for the slope mask:
	if (slopeMask_)
	{
		result += LocaliseUTF8(L"_s%0_%1", s2_, s3_ );
	}

	// Add some info for the texture mask:
	BW::StringRef texureMaskFile = BWResource::getFilename( textureMaskTexture_ );
	texureMaskFile = BWResource::removeExtension( texureMaskFile );	
	if (textureMask_ && !texureMaskFile.empty())
	{
		result += "_t";
		result += texureMaskFile.to_string();
	}

	// Add some info for the noise mask:
	if (noiseMask_)
	{
		result += "_n";
	}

	return result;
}


/**
 *	This gets the name of the texture used by the brush defined in the given
 *	DataSection.
 *
 *	@param pDataSection	The DataSection that defines the brush.
 *	@return				The name of the texture file used by the brush.  This
 *						can be the empty string if there is not texture or if
 *						the DataSection is NULL.
 */
/*static*/ BW::string TerrainPaintBrush::texture( 
	DataSectionPtr	pDataSection )
{
	BW_GUARD;

	if (!pDataSection)
	{
		return BW::string();
	}
	else
	{
		BW::string brushType = pDataSection->readString ( "type" );
		if (brushType != "TerrainPainting")
		{
			return BW::string();
		}
		else
		{
			return pDataSection->readString( "texture" );
		}
	}
}

/**
 *	This gets the name of the bump texture used by the brush defined in the given
 *	DataSection.
 *
 *	@param pDataSection	The DataSection that defines the brush.
 *	@return				The name of the bump texture file used by the brush.  This
 *						can be the empty string if there is not texture or if
 *						the DataSection is NULL.
 */
/*static*/ BW::string TerrainPaintBrush::bumpTexture(const DataSectionPtr&	pDataSection)
{
	BW_GUARD;

	if (!pDataSection)
	{
		return BW::string();
	}
	else
	{
		BW::string brushType = pDataSection->readString ( "type" );
		if (brushType != "TerrainPainting")
		{
			return BW::string();
		}
		else
		{
			return pDataSection->readString( "bumpTexture" );
		}
	}
}


/**
 *	This compares two TerrainPaintBrushes to see if they are the same.
 *
 *	@param other		The TerrainPaintBrush to compare against.
 *	@return 			True if the TerrainPaintBrush are the same, false 
 *						otherwise.
 */
bool TerrainPaintBrush::operator==( const TerrainPaintBrush & other ) const
{
	return !operator!=( other );
}


/**
 *	This compares two TerrainPaintBrushes to see if they are different.
 *
 *	@param other		The TerrainPaintBrush to compare against.
 *	@return 			True if the TerrainPaintBrush are different, false 
 *						otherwise.  Note that we do NOT compare differences
 *						between import masks.
 */
bool TerrainPaintBrush::operator!=( const TerrainPaintBrush & other ) const
{
	return 
		paintTexture_ != other.paintTexture_
		||
		paintBumpTexture_ != other.paintBumpTexture_
		||
		strength_ != other.strength_
		||
		size_ != other.size_
		||
		paintUProj_ != other.paintUProj_
		||
		paintVProj_ != other.paintVProj_
		||
		uvLocked_ != other.uvLocked_		
		||
		noise_ != other.noise_
		||
		heightMask_ != other.heightMask_
		||
		h1_ != other.h1_
		||
		h2_ != other.h2_
		||
		h3_ != other.h3_
		||
		h4_ != other.h4_
		||
		slopeMask_ != other.slopeMask_
		||
		s1_ != other.s1_
		||
		s2_ != other.s2_
		||
		s3_ != other.s3_
		||
		s4_ != other.s4_
		||
		textureMask_ != other.textureMask_
		||
		textureMaskIncludeProj_ != other.textureMaskIncludeProj_
		||
		textureMaskIncludeBump_ != other.textureMaskIncludeBump_
		||
		textureMaskUProj_ != other.textureMaskUProj_
		||
		textureMaskVProj_ != other.textureMaskVProj_
		||
		textureMaskTexture_ != other.textureMaskTexture_ 
		||
		bumpTextureMaskTexture_ != other.bumpTextureMaskTexture_
		||
		textureMaskInvert_ != other.textureMaskInvert_
        ||
        noiseMask_ != other.noiseMask_
		||
		importMask_ != other.importMask_
		||
		importMaskTL_ != other.importMaskTL_
		||
		importMaskBR_ != other.importMaskBR_
		||
		importMaskImage_.getObject() != other.importMaskImage_.getObject()
		||
		importMaskAdd_ != other.importMaskAdd_
		||
		importMaskMul_ != other.importMaskMul_
		||
		opacity_ != other.opacity_
		||
		maxLayerLimit_ != other.maxLayerLimit_
		||
		maxLayers_ != other.maxLayers_;
}


/**
 *	This can be used to determine whether the given layer is in the layer
 *	mask.  The layer being painted is always in the mask layer.
 *
 *	@param layer		The layer to test.
 *	@return 			True if the layer should be masked off, false 
 *						otherwise.
 */
bool TerrainPaintBrush::isMaskLayer( const Terrain::TerrainTextureLayer & layer ) const
{
	BW_GUARD;

	return
		// The layer being painted?
		layer.sameTexture
		(
			paintTexture_, 
			paintBumpTexture_,
			paintUProj_, 
			paintVProj_, 
			TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON
		)
		||
		// If we don't care about projections then a match on the texture name
		// will do.
		(
			!textureMaskIncludeProj_
			&&
			layer.textureName() == textureMaskTexture_
			&&
			layer.bumpTextureName() == (textureMaskIncludeBump_ ? bumpTextureMaskTexture_ : layer.bumpTextureName())
		)
		||
		// If we do care about projections then compare the projects as well.
		layer.sameTexture
		(
			textureMaskTexture_, 
			textureMaskIncludeBump_ ? bumpTextureMaskTexture_ : layer.bumpTextureName(),
			textureMaskUProj_, 
			textureMaskVProj_, 
			TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON
		);
}
BW_END_NAMESPACE

