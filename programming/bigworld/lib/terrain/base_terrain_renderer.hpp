
#ifndef TERRAIN_BASE_TERRAIN_RENDERER_HPP
#define TERRAIN_BASE_TERRAIN_RENDERER_HPP


#include "moo/forward_declarations.hpp"
#include "moo/effect_material.hpp"
#include "moo/complex_effect_material.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

class EffectMaterial;
class BaseRenderTerrainBlock;

class BaseTerrainRenderer : public SafeReferenceCount
{
public:
	/**
	 *  This method sets the current terrain renderer instance. At the moment,
	 *  TerrainRenderers call this method when they are
	 *  inited to set themselves as the current renderer.
	 *
	 *  @param newInstance      pointer to the current terrain renderer, or 
	 *                          NULL to disable terrain rendering.
	 */
	static void instance( BaseTerrainRenderer* newInstance );

	/**
	 *  Singleton instance get method.
	 *
	 *  @return          instance of the current terrain renderer. If no
	 *                   terrain renderer has been set, it returns a pointer
	 *                   to a dummy renderer that does nothing.
	 */
	static BaseTerrainRenderer* instance();

	/**
	 *  Get the version of the terrain renderer.
	 *
	 *  @return          terrain renderer version, 0 if no terrain renderer
	 *                   instance has been set.
	 */
	virtual uint32 version() const = 0;

	/**
	*	This function adds a block for drawing.
	*
	*	@param pBlock			The block to render
	*	@param transform		The transformation matrix to apply when drawing.
	*/
	virtual void addBlock(	BaseRenderTerrainBlock*	pBlock,	
							const Matrix&			transform) = 0;

	/**
	 *  This function draws the list of terrain blocks.
	 *
	 *	@param passType		Rendering pass type.
	 *  @param pOverride    Override material to use for each block.
	 *	@param clearList	Clear the list of blocks to render.
	 */
	virtual void drawAll(
		Moo::ERenderingPassType passType,
		Moo::EffectMaterialPtr pOverride = NULL, 
		bool clearList = true
		) = 0;

	/**
	*	This function asks the renderer to immediately draw the terrain
	*	block. The block is not added to the render list. The caller must 
	*	maintain render context transform state. By default the cached
	*	lighting is not used - the caller must set lighting before call.
	*
	*	@param passType				Rendering pass type
	*	@param pBlock				The block to render.
	*	@param transform			World transform for block.
	*	@param altMat				An alternative drawing material.
	*/
	virtual void drawSingle(
		Moo::ERenderingPassType passType,
		BaseRenderTerrainBlock*	pBlock,	
							const Matrix&			transform, 
		Moo::EffectMaterialPtr altMat = NULL
		) = 0;

	/**
	 *  Clear the list of terrain blocks to be renderered.
	 */
	virtual void clearBlocks() = 0;

	/**
	 *  Method to find out if the terrain should be drawn or not.
	 *
	 *  @return          true if the terrain should be drawn, false otherwise.
	 */
	virtual bool canSeeTerrain() const = 0;

	/**
	 *  Tell the renderer that it should/shouldn't use specular lighting.
	 *  This method is used by the chunk photographer class in WE.
	 *
	 *  @param          true to use specular in the terrain, false otherwise.
	 */
	void enableSpecular(bool enable) { specularEnabled_ = enable; }

	/**
	 *  Method to find out if the terrain should be drawn with specular
	 *  highlights or not. This method is used by the chunk photographer class
	 *  in WE.
	 *
	 *  @return          true if using specular in the terrain, false otherwise.
	 */
	bool specularEnabled() const { return specularEnabled_; }

	/**
	 *	This method sets the visibility flag for the renderer.
	 */
	void isVisible( bool value ) { isVisible_ = value; }

	/**
	*	This method reads a flag that indicates whether or not hole map is used 
	*	during terrain texture rendering.
	*/
	bool holeMapEnabled() const { return holeMapEnabled_ ; }
	bool enableHoleMap( bool value ) ;

	// switch for hiding overlays when generating terrain LOD texture.
	void enableOverlays(bool isEnabled);
	bool overlaysEnabled() const { return overlaysEnabled_; };

#if ENABLE_WATCHERS
	static bool isEnabledInWatcher;
#endif

protected:
	BaseTerrainRenderer() :
		isVisible_( false ),
		specularEnabled_( true ),
		holeMapEnabled_( true ),
		overlaysEnabled_(true)
	{
#if ENABLE_WATCHERS
		initWatcher();
#endif
	}

	bool isVisible_;

#if ENABLE_WATCHERS
	void initWatcher();
#endif

private:
	bool specularEnabled_;
	bool holeMapEnabled_;

	bool overlaysEnabled_;
};

typedef SmartPointer<BaseTerrainRenderer> BaseTerrainRendererPtr;

}; // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_BASE_TERRAIN_RENDERER_HPP
