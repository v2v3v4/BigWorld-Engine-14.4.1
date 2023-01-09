#ifndef TEXTURE_AGGREGATOR_HPP
#define TEXTURE_AGGREGATOR_HPP

#include "texture_manager.hpp"
#include <exception>


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	Dynamically aggregates individual texture maps into a single big map. 
 *
 *	By using a TextureAggregator, smaller individual texture maps 
 *	(or just sections of them) may be tiled into a single bigger texture. 
 *	This is useful when compounding objects loaded separately to be drawn 
 *	with a single draw call or less texture swapping.
 *
 *	The aggregated texture will grow on demand to accommodate added tiles.
 *
 *	As textures get added to the aggregator, they will be tiled side by 
 *	side in the aggregated map. A tile ID is returned. Texture coordinates 
 *	of each tile in the aggregated map are accessible at any time given a 
 *	tile ID. Texture coordinates of tiles are always expressed in texels
 *	space.  A texture transform matrix is provided to translate from texels
 *	space the [0, 1] range. This prevents vertex buffers from needing to be 
 *	rebuilt when the aggregated map grows.
 *
 *	Tiles can also be removed from the aggregator. The texture will shrink
 *	when there is enough unused space in the aggregated map. During the 
 *	shrinking process, tiles will be reshuffled. When this happens, 
 *	previously returned texture coordinates will be stale and any vertex 
 *	buffer referring to them will have to be rebuilt. A callback informs 
 *	clients when this happens.
 *
 *	Currently, the TextureAggregator is limited to tiles of power of two size
 *	and, although supported, non-square tiles are stretched to be square and
 *	may present some visual artifacts.
 */
class TextureAggregator
{
public:
	typedef void(*ResetNotifyFunc)(void);

	TextureAggregator(ResetNotifyFunc notifyFunc = NULL);
	~TextureAggregator();

	int addTile(
		BaseTexturePtr  tex, 
		const Vector2 & min, 
		const Vector2 & max);

	void getTileCoords(int id, Vector2 & min, Vector2 & max) const;

	void delTile(int id);

	void repack();
	bool tilesReset() const;

	DX::Texture * texture() const;
	const Matrix & transform() const;

	int minSize() const;
	void setMinSize(int minSize);

	int maxSize() const;
	void setMaxSize(int maxSize);

	int mipLevels() const;
	void setMipLevels(int mipLevels);

private:
	std::auto_ptr<struct TextureAggregatorPimpl> pimpl_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_AGGREGATOR_HPP
