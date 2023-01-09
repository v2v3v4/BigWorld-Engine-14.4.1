#ifndef HEIGHT_MAP_HPP
#define HEIGHT_MAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/import/elevation_blit.hpp"
#include "worldeditor/import/import_image.hpp"
#include "cstdmf/init_singleton.hpp"
#include "moo/device_callback.hpp"
#include "moo/moo_dx.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/texture_lock_wrapper.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class manages a terrain height map texture.
 */
class HeightMap : public Moo::DeviceCallback, public Singleton< HeightMap >
{
public:

    HeightMap();
    ~HeightMap();

    void spaceInformation
    (
        BW::string         const &spaceName,
        int32               localToWorldX,
        int32               localToWorldY,
        uint16              width,
        uint16              height,
        bool                createAndValidateTexture    = true
    );

    void save();

    void onStop();

    void setTexture(uint8 textureStage);

    void update(float dtime);

    DX::Texture *texture() const;

    float minHeight() const;
    float maxHeight() const;

    void updateMinMax();

    static float normaliseHeight(float height);

    void drawImportedTerrain
    (
        float               left,
        float               top,
        float               right,
        float               bottom,
        ElevationBlit::Mode mode,
        float               minv,
        float               maxv,
        float               alpha, 
        ImportImage			const &image
    );

    void undrawImportedTerrain();
    void clearImpTerrain();
    void invalidateImportedTerrain();
    
    void dirtyThumbnail(Chunk *chunk, bool justLoaded = false);
    void dirtyThumbnail(int32 gridX, int32 gridY, bool justLoaded = false);

    float heightAtGridPos(Vector2 const &pos) const;

    void 
    heightRange
    (
        Vector2             const &pos1, 
        Vector2             const &pos2,
        float               &minv,
        float               &maxv
    ) const;

    void makeValid(bool progress = true);

    void doNotSaveHeightMap();

    bool isPS2() const;

	bool aborted() const { return aborted_; }

protected:

    void extractSubimage
    (
        float               left,
        float               top,
        float               right,
        float               bottom,
        Moo::Image<uint16>    &subImage
    ) const;

    void restoreSubimage
    (
        float               left,
        float               top,
        float               right,
        float               bottom,
        Moo::Image<uint16>    const &subImage
    );

    void addTerrain
    (
        float               left,
        float               top,
        float               right,
        float               bottom,
        ElevationBlit::Mode mode,
        float               minv,
        float               maxv,
        float               alpha, 
        ImportImage			const &image
    );

	void createTexture();
	void deleteTexture();

    bool 
    chunkToHeightMap
    (
        int32			gx, 
        int32			gy,
		bool			force
    );

    bool drawSomeChunks(bool force);

    uint32       recommendHeightMapSize(uint32       x) const;

    bool toTexCoords
    (
        float           left,
        float           top,
        float           right,
        float           bottom,
        int32           &texLeft,
        int32           &texTop,
        int32           &texRight,
        int32           &texBottom,
        int32           *ileft      = NULL,
        int32           *itop       = NULL,
        int32           *iright     = NULL,
        int32           *ibottom    = NULL
    ) const;

    void blitAdditive
    (
        ImportImage		const &image,
        int32           left,
        int32           top,
        int32           right,
        int32           bottom,
        int32           ileft,
        int32           itop,
        int32           iright,
        int32           ibottom,
        float           scale,
        float           offset
    );

    void blitSubtractive
    (
        ImportImage		const &image,
        int32           left,
        int32           top,
        int32           right,
        int32           bottom,
        int32           ileft,
        int32           itop,
        int32           iright,
        int32           ibottom,
        float           scale,
        float           offset
    );

    void blitMin
    (
        ImportImage		const &image,
        int32           left,
        int32           top,
        int32           right,
        int32           bottom,
        int32           ileft,
        int32           itop,
        int32           iright,
        int32           ibottom,
        float           scale,
        float           offset
    );

    void blitMax
    (
        ImportImage		const &image,
        int32           left,
        int32           top,
        int32           right,
        int32           bottom,
        int32           ileft,
        int32           itop,
        int32           iright,
        int32           ibottom,
        float           scale,
        float           offset
    );

    void blitReplace
    (
        ImportImage		const &image,
        int32           left,
        int32           top,
        int32           right,
        int32           bottom,
        int32           ileft,
        int32           itop,
        int32           iright,
        int32           ibottom,
        float           scale,
        float           offset
    );

    BW::string cachedHeightMapName() const;

    bool loadCachedHeightMap();
    void saveCachedHeightMap();

    void redrawAllChunks();

    void ensureValidTexture() const;

    void beginTexture( uint16 **base, uint32 *pitch, bool readOnly ) const;
    void endTexture() const;

    uint32       textureWidth() const;
    uint32       textureHeight() const;

    bool hasTexture() const;

private:
    typedef std::pair<int32, int32> GridPos;

    mutable ComObjectWrap<DX::Texture>	texture_;
	mutable ComObjectWrap<DX::Texture>	textureSubstitute_;
	mutable bool	accessReadOnly_;

    uint32                  gridWidth_;
    uint32                  gridHeight_;
    BW::string             spaceName_;
    float                   minHeightTerrain_;
    float                   maxHeightTerrain_;
    float                   minHeightImpTerrain_;
    float                   maxHeightImpTerrain_;
    float                   minHeight_;
    float                   maxHeight_;
    uint32                  numPolesX_;
    uint32                  numPolesY_;
    uint32                  blockWidth_;
    uint32                  blockHeight_;
    uint32                  visOffX_;
    uint32                  visOffY_;
    int32                   localToWorldX_;
    int32                   localToWorldY_;
    BW::list<GridPos>      undrawnChunks_;
    Moo::Image<uint16>		heightMap_;
    Moo::Image<uint16>		impTerrSubImg_;
    float                   impTerrLeft_;
    float                   impTerrTop_;
    float                   impTerrRight_;
    float                   impTerrBottom_;
    bool                    initialised_;
    mutable bool            invalidTexture_;
    mutable bool            isEightBitTex_;
    bool                    isPS2_;
    uint32                  recommendSize_;
	bool					madeValidOnce_;
	bool					aborted_;
};

BW_END_NAMESPACE

#endif // HEIGHT_MAP_HPP
