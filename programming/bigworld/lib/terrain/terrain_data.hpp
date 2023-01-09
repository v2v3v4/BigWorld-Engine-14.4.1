#ifndef TERRAIN_DATA
#define TERRAIN_DATA

BW_BEGIN_NAMESPACE

namespace Terrain
{
const uint		MINIMUM_TERRAIN_HEIGHT_MAP	= 32;

enum HeightMapCompression
{
	COMPRESS_RAW  = 0,
	COMPRESS_LAST = 0x7fffffff
};

enum NormalMapQuality
{
	NMQ_NICE,	// Best looking, but slower to generate
	NMQ_FAST	// Fast to generate, but not as good quality (good for preview)
};

struct HeightMapHeader
{
	enum HMHVersions
	{
		VERSION_ABS_FLOAT		= 1,	// absolute float values
		VERSION_REL_UINT16		= 2,	// relative uint16 values
		VERSION_REL_UINT16_PNG	= 3,	// as above, compressed via png.
		VERSION_ABS_QFLOAT		= 4		// absolute quantised float values
	};

	uint32					magic_;		// Should be "hmp\0"
	uint32					width_;
	uint32					height_;
	HeightMapCompression	compression_;
	uint32					version_;	// format version
	float					minHeight_;
	float					maxHeight_;
	uint32					pad_;
	// static const uint32 MAGIC = '\0pmh';
	static const uint32 MAGIC = 0x00706d68;
};


struct BlendHeader
{
	enum BHMVersions
	{
		VERSION_RAW_BLENDS	= 1,
		VERSION_PNG_BLENDS	= 2
	};

	uint32	magic_;		// Should be "bld\0"
	uint32	width_;
	uint32	height_;
	uint32	bpp_;		// Reserved for future use.
	Vector4	uProjection_;
	Vector4	vProjection_;
	uint32	version_;	// format version
	uint32  hasBumpMap_; // has bump map or not
	uint32	pad_[2];
	// static const uint32 MAGIC = '\0dlb';
	static const uint32 MAGIC = 0x00646c62;
};

struct ShadowHeader
{
	uint32	magic_; // Should be "shd\0"
	uint32	width_;
	uint32	height_;
	uint32	bpp_;
	uint32	version_;
	uint32	pad_[3];
	// static const uint32 MAGIC = '\0dhs';
	static const uint32 MAGIC = 0x00646873;
};

struct HolesHeader
{
	uint32 magic_;
	uint32 width_;
	uint32 height_;
	uint32 version_;
	// static const uint32 MAGIC = '\0loh';
	static const uint32 MAGIC = 0x006c6f68;
};

struct VertexLODHeader
{
	// static const uint32 MAGIC = '\0rev';
	static const uint32 MAGIC = 0x00726576;

	enum VLHVersions
	{
		VERSION_RAW_VERTICES = 1,	// regular vertices
		VERSION_ZIP_VERTICES = 2	// zip compressed vertices
	};

	uint32 magic_;
	uint32 version_;
	uint32 gridSize_;
};

struct TerrainNormalMapHeader
{
	static const uint32 VERSION_16_BIT_PNG	= 1;
	uint32 magic_; // Should be "nrm\0"
	uint32 version_;
	uint32 pad_[2];

	// static const uint32 MAGIC = '\0mrn';
	static const uint32 MAGIC = 0x006d726e;
};

struct TerrainAOMapHeader
{
	static const uint32 VERSION_8_BIT_PNG = 1;
	uint32 magic_; // should be "aom\0"
	uint32 version_;
	uint32 pad_[2];

	// static const uint32 MAGIC = '\0oam";
	static const uint32 MAGIC = 0x006f616d;
};

struct DominantTextureMapHeader
{
	static const uint32 VERSION_ZIP	= 1;
	uint32 magic_;
	uint32 version_;
	uint32 numTextures_;
	uint32 textureNameSize_;
	uint32 width_;
	uint32 height_;
	uint32 pad_[2];
	// static const uint32 MAGIC = '\0tam';
	static const uint32 MAGIC = 0x0074616d;
};

} // namespace Terrain

BW_END_NAMESPACE


#endif
