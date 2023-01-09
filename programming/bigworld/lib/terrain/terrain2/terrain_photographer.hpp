#ifndef TERRAIN_PHOTOGRAPHER_HPP
#define TERRAIN_PHOTOGRAPHER_HPP

#ifdef EDITOR_ENABLED

BW_BEGIN_NAMESPACE

namespace Moo
{
	class RenderTarget;
	typedef SmartPointer<RenderTarget> RenderTargetPtr;
}

namespace Terrain
{
	class TerrainRenderer2;
	class BaseTerrainBlock;

	/**
	 */
	class TerrainPhotographer
	{
	public:
		TerrainPhotographer(TerrainRenderer2& drawer);
		~TerrainPhotographer();

		bool init( uint32 basePhotoSize );
		bool photographBlock(	BaseTerrainBlock*	pBlock,
								const Matrix&		transform );

		bool output(ComObjectWrap<DX::Texture>&	pDestTexture,
					D3DFORMAT					destImageFormat = D3DFMT_UNKNOWN );

	private:
		Moo::RenderTargetPtr	pBasePhoto_;
		TerrainRenderer2&	 drawer_;	 
	};

} // namespace Terrain

BW_END_NAMESPACE

#endif //-- EDITOR_ENABLED

#endif // TERRAIN_BLOCK2_HPP
