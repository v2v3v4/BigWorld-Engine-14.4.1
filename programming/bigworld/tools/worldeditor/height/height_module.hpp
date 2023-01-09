#ifndef HEIGHT_MODULE_HPP
#define HEIGHT_MODULE_HPP


#include "common/grid_coord.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/import/import_image.hpp"
#include "worldeditor/import/elevation_blit.hpp"
#include "appmgr/module.hpp"

BW_BEGIN_NAMESPACE

class TextGUIComponent;


/**
 *  This module draws a height map of the terrain in the space.
 *
 *  The user can select a rectangle to import/export etc.
 */
class HeightModule : public FrameworkModule
{
public:
    enum Mode
    {
        IMPORT_TERRAIN,  // user is importing terrain
		IMPORT_MASK   ,  // user is importing a terrain mask
        EXPORT_TERRAIN   // user is exporting terrain
    };

    HeightModule();
    ~HeightModule();

    virtual bool init( DataSectionPtr pSection );

	static void fini();

    virtual void onStart();
    virtual int  onStop();

    virtual void onPause();
    virtual void onResume( int exitCode );

    virtual bool updateState( float dTime );
	virtual void updateAnimations();
    virtual void render( float dTime );

    virtual bool handleKeyEvent( const KeyEvent & keyEvent );  

	virtual void setApp( App * app ) {};
	virtual void setMainFrame( IMainFrame * mainFrame ) {};

    void gotoWorldPos    ( const KeyEvent &keyEvent );
    void startTerrainEdit( const KeyEvent &keyEvent );
    void startGetHeight  ( const KeyEvent &keyEvent );

    virtual bool handleMouseEvent( const MouseEvent & mouseEvent );

    void handleZoom       ( const MouseEvent & mouseEvent );
    void handlePan        ( const MouseEvent & mouseEvent );
    void handleTerrainEdit( const MouseEvent & mouseEvent );
    void handleGetHeight  ( const MouseEvent & mouseEvent );

    /** Set blend value for space map **/
    void projectMapAlpha( float a );
    float projectMapAlpha();

    /** Get/set the current mode **/
    Mode mode() const;
	void mode(Mode m);

    /** Set the import elevation/mask image, resets the mode to export if null **/
    void importData
    (
        ImportImagePtr		image,
        float               *left   = NULL,
        float               *top    = NULL,
        float               *right  = NULL,
        float               *bottom = NULL
    );
   	bool hasImportData() const; 
	static ImportImagePtr elevationImage();
	static ImportImagePtr maskImage();
	static Vector2 topLeft(bool mask);
	static Vector2 bottomRight( bool mask);

    /** Rotate the imported elevation/mask data **/
    void rotateImportData(bool clockwise);

    enum FlipDir
    {
        FLIP_X,
        FLIP_Y,
        FLIP_HEIGHT
    };

    /** Flip the elevation data about the given axis */
    void flipImportData(FlipDir dir);

    /** Set the import options */
    void 
    terrainImportOptions
    (
        ElevationBlit::Mode mode,
        float               minV, 
        float               maxV,
        float               strength,
		bool				absolute
    );

	/** Set the texture import options */
	void
	textureImportOptions
	(
		BW::string			const &texture,
		Vector4				const &uProjection,
		Vector4				const &vProjection,
		float				strength
	);
	void textureImportStrength(float strength);
	static float textureImportStrength();

    /** The size of the undo if import was done in megabytes. */
    size_t terrainUndoSize() const;

    /** Do an import of the data */
    bool doTerrainImport(bool doUndo, bool showProgress, bool forceToMem);

    /** Do an export of the data */
    bool doTerrainExport(char const *filename, bool showProgress);

    /** Update the min/max range. */
    void updateMinMax();

    /** Ensure that the cached height map is calculated. */
    static void ensureHeightMapCalculated();

    /** Flag that the height map is completely invalid and should not be saved. */
    static void doNotSaveHeightMap();

    /** Get the current instance on the module stack, if any */
    static HeightModule* currentInstance();

    /** True if there is an instance and if it has started */
	static bool hasStarted();

private:
    HeightModule( const HeightModule& );
    HeightModule& operator=( const HeightModule& );

protected:
    void onQuickStart();

    void writeStatus();

    /** Convert from screen to world coordinates */
    Vector2 gridPos(POINT pt) const;

    /** Where in the grid the mouse is currently pointing */
    Vector2 currentGridPos() const;

    /** Get a world position from a grid position */
    Vector3 gridPosToWorldPos(Vector2 const &gridPos) const;

    /** Get a grid position from a world position */
    Vector2 worldPosToGridPos(Vector3 const &pos) const;

    enum TerrainDir
    {
        TERRAIN_MISS,
        TERRAIN_MIDDLE,
        TERRAIN_NORTH,
        TERRAIN_NORTH_EAST,
        TERRAIN_EAST,
        TERRAIN_SOUTH_EAST,
        TERRAIN_SOUTH,
        TERRAIN_SOUTH_WEST,
        TERRAIN_WEST,
        TERRAIN_NORTH_WEST
    };

    /** Get a handle direction from the screen point pt. */
    TerrainDir pointToDir(POINT pt) const;

    /** Make sure that left < right, bottom < top. */
    void normaliseTerrainRect();

    /** Snap based on whether the CTRL key is down. */
    float snapTerrainCoord(float v) const;

    /** Draw an unfilled rectangle at the given position in the given thickness. */
    void drawRect( const Vector2 &ul, const Vector2 &lr, uint32 colour, int ht);

    /** Draw an filled rectangle at the given position. */
    void drawRect( const Vector2 &ul, const Vector2 &lr, uint32 colour);

    /** Draw a handle at the world pos. v of size (2*hx + 1), (2*hy + 1) pixels. */
    void drawHandle( const Vector2 & v, uint32 colour, int hx, int hy ) const;

    /** Draw a vertical line. */
    void drawVLine( float x, uint32 colour, float hx ) const;

    /** Draw a horizontal line. */
    void drawHLine( float y, uint32 colour, float hy ) const;

    /** The imported terrain options/positions etc have changed, redraw it. */
    void redrawImportedTerrain();

    /** Perform a height query. */
    void heightQuery();

    /** Update the cursor. */
    void updateCursor();

	/** Validate the world rectangle to ensure the selection never leaves the world bounds */
	void validateWorldRect();

	/** Update the world rectangle. */
	void updateWorldRect();

	/** The imported image appropriate for the mode. */
	ImportImagePtr importImage() const;

private:
	bool								hasStarted_;
	SmartPointer<TextGUIComponent>		abortedText_;
    float                               spaceAlpha_;
    Moo::BaseTexturePtr                 handDrawnMap_;
    Vector3                             viewPosition_;          /** Camera position, X & Y specify position, Z specifies zoom */
    float                               minViewZ_;
    float                               savedFarPlane_;         /** Saved far-plane, allows restoration when returning to main editing */   
	float								gridSize_;
	int									minX_;
	int									minY_;
    unsigned int                        gridWidth_;             /** Extent of the grid, in number of chunks. It starts at 0,0 */
    unsigned int                        gridHeight_;
    GridCoord                           localToWorld_;          /** Translation from local to world grid coordinates */
    SmartPointer<ClosedCaptions>        cc_;
    bool                                mouseDownOn3DWindow_;   /** True if the mouse button is down */
    Mode                                mode_;                  /** Current mode */
    static ImportImagePtr				s_elevationData_;       /** Imported terrain */ 
	static ImportImagePtr				s_maskData_;			/** Imported mask */
    Moo::Image<uint16>                  subTexture_;            /** Sub texture of height map when importing and editing */
    Vector2                             terrainTopLeft_;        /** Top-left of the imported terrain */
    Vector2                             terrainBottomRight_;    /** Bottom-right of the imported terrain */
    Vector2                             maskTopLeft_;			/** Top-left of the imported mask */
    Vector2                             maskBottomRight_;		/** Bottom-right of the imported mask */
    TerrainDir                          terrainDir_;            /** Direction editing terrain */
    Vector2                             terrainDownPt_;         /** Point mouse went down */
    Vector2                             terrainTLDown_;
    Vector2                             terrainBRDown_;
    float                               scaleX_;                /** Horizontal size of a pixel in world units */
    float                               scaleY_;                /** Vertical size of a pixel in world units */
    Moo::EffectMaterialPtr				terrainShader_;			/** Shader to draw terrain and blend with hand drawn map */
    Moo::EffectMaterialPtr				selectionShader_;		/** Shader to draw selection rectangle */
    ElevationBlit::Mode                 impMode_;               /** How to display temp. bitmap. */
    float                               impMinV_;               /** Minimum height on import */
    float                               impMaxV_;               /** Maximum height on import */
    float                               impStrength_;           /** Import strength */
    Vector2                             heightDownPt_;          /** Getting height down point. */
    Vector2                             heightCurrentPt_;       /** Getting height current point. */
    bool                                gettingHeight_;         /** True if getting height. */
    CPoint                              lastCursorPos_;         /** Last cursor pos - for panning. */
	BW::string							texture_;				/** Imported texture name */
	Vector4								uProj_;					/** U-projection for texture mask */
	Vector4								vProj_;					/** V-projection for texture make */
	bool								absoluteHeights_;		/** Use absolute heights? */
	static float						s_texStrength_;			/** Strength for texture import */
    static HeightModule *				s_currentInstance_;		/** The last created HeightModule */
	static Vector2						s_terrainTopLeft_;		/** Top-left of terrain in world coordinates */
	static Vector2						s_terrainBottomRight_;	/** Bottom-right of terrain in world coordinates */
	static Vector2						s_maskTopLeft_;			/** Top-left of mask in world coordinates */
	static Vector2						s_maskBottomRight_;		/** Bottom-right of mask in world coordinates */

};

BW_END_NAMESPACE

#endif // HEIGHT_MODULE_HPP
