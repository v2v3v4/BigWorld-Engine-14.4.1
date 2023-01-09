
#include "pch.hpp"
#include "texture_aggregator.hpp"

#include "wingdi.h"
#include "material.hpp"

DECLARE_DEBUG_COMPONENT2("Moo", 0)

BW_BEGIN_NAMESPACE

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: TextureAggregatorPimpl
// -----------------------------------------------------------------------------

/**
 *	TextuerAggregator private implementation.
 */
struct TextureAggregatorPimpl : DeviceCallback
{
	TextureAggregatorPimpl();		

	/**
	 *	A single slot (space) in the aggregated map.
	 */
	struct Slot : public ReferenceCount
	{
		Slot(int xx, int xy, int ww, int hh) :
			x(xx),
			y(xy),
			width(ww),
			height(hh)
		{}

		void scale(int scaleX, int scaleY)
		{
			x /= scaleX;
			y /= scaleY;
			width  /= scaleX;
			height /= scaleY;
		}

		int x;
		int y;
		int width;
		int height;
	};
	typedef SmartPointer<Slot> SlotPtr;

	/**
	 *	This is the data require to stamp a tile in the aggregated 
	 *	map. It points not only to the slot in it, but also the 
	 *	original texture map and the texture coordinates that 
	 *	delimit what should go into the aggregator.
	 */
	struct Tile
	{
		Tile() {}
		Tile(
				BaseTexturePtr tx, 
				const Vector2 & mn, 
				const Vector2 & mx, 
				int wid, 
				int hei, 
				SlotPtr sl) :
			tex(tx),
			min(mn),
			max(mx),
			width(wid),
			height(hei),
			slot(sl)
		{}

		BaseTexturePtr tex;
		Vector2        min;
		Vector2        max;
		int            width;
		int            height;
		SlotPtr        slot;
	};

	typedef BW::vector<SlotPtr> SlotVector;
	typedef BW::map<int, SlotVector> SlotVectorMap;
	typedef BW::map<int, Tile> TilesMap;

	typedef ComObjectWrap<DX::Texture> DXTexturePtr;

	SlotPtr getSlot(int log2);
	bool makeSlots(int log2);
	bool needsToGrow(int log2) const;
	SlotVector growTexture(int log2Hint);
	DXTexturePtr createTexture(int width, int height);
	void copyWholeTexture(DXTexturePtr src, DXTexturePtr dst);

	int registerTile(
		BaseTexturePtr tex, 
		const Vector2 & min, 
		const Vector2 & max, 
		int width, 
		int height,
		SlotPtr slot);

	void renderTile(
		BaseTexturePtr tex, 
		const Vector2 & min, 
		const Vector2 & max, 
		int width, 
		int height,
		SlotPtr slot);

	void recycleSlot(SlotPtr slot);
	bool areNeightboursFree(SlotPtr slot, SlotVector & slots);
	bool isFree(SlotPtr slot);
	SlotPtr findSlot(int coordx, int coordy, int log2);
	void killSlot(SlotPtr slot);
	float textureUsage() const;

	void repackLater();
	void repackCheck();
	void repack();

	virtual void createUnmanagedObjects();
	virtual void deleteUnmanagedObjects();

	DXTexturePtr  texture;
	Matrix        transform;
	SlotVectorMap freeSlots;
	TilesMap      tiles;
	int           nextId;
	bool		  tilesReset;
	bool          repackPending;
	int           minSize;
	int           maxSize;
	int           mipLevels;

	TextureAggregator::ResetNotifyFunc resetNotifyFunc;
};

typedef TextureAggregatorPimpl::SlotPtr SlotPtr;

namespace // anonymous
{

// Named constants
const int c_MinSize           = 128;
const int c_MaxSize           = 2048;
const int c_MipLevels         = 4;
const int c_TileBorder        = 1;
const float c_RepackThreshold = 0.35f;

// Helper functions
int sizeToLog2(int size);
int log2ToSize(int log2);
TextureAggregatorPimpl::SlotVector subdivideSlot(SlotPtr slot);
int texWidth(TextureAggregatorPimpl::DXTexturePtr tex);
int texHeight(TextureAggregatorPimpl::DXTexturePtr tex);
void copyTile(
	int               srcWidth, 
	int               srcHeight,
	DX::BaseTexture * src, 
	const Vector2   & pSrcMin, 
	const Vector2   & pSrcMax,
	DX::Texture     * dst, 
	const Vector2   & pDstMin, 
	const Vector2   & pDstMax,
	bool              cropBorders);


} // namespace anonymous

// -----------------------------------------------------------------------------
// Section: TextureAggregator
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param	notifyFunc	pointer to reset notification function. This function
 *						will be called back whenever the aggregator is repacked.
 */
TextureAggregator::TextureAggregator(ResetNotifyFunc notifyFunc) :
	pimpl_(new TextureAggregatorPimpl)
{
	this->pimpl_->resetNotifyFunc = notifyFunc;
}

/**
 *	Destructor.
 */
TextureAggregator::~TextureAggregator()
{}

/**
 *	Adds a new tile to the aggregator.
 *
 *	@param	tex		pointer to texture being added
 *	@param	min		(out) tile's upper-left corner (in [1, 0] range).
 *	@param	max		(out) tile's lower-right corner (in [1, 0] range).
 *
 *	@return			the tile's id, or -1 if unsuccessful.
 */
int TextureAggregator::addTile(
	BaseTexturePtr  tex, 
	const Vector2 & min, 
	const Vector2 & max)
{
	BW_GUARD;
	pimpl_->repackCheck();

	int texWidth  = tex->width();
	int texHeight = tex->height();
	int tileX = int(min.x * texWidth);
	int tileY = int(min.y * texHeight);
	int tileWidth  = int((max.x-min.x) * texWidth);
	int tileHeight = int((max.y-min.y) * texHeight);

	if (tileWidth <= 0)
	{
		WARNING_MSG("TextureAggregator::addTile(): Invalid tile width.\n");
		return -1;		
	}

	// make it square
	int slotWidth = std::max(tileWidth, tileHeight);
	int log2 = sizeToLog2(slotWidth);
	SlotPtr slot = pimpl_->getSlot(log2);

	// can't find a slot, so return error
	if ( !slot )
		return -1;

	return pimpl_->registerTile(tex, min, max, tileWidth, tileHeight, slot);
}

/**
 *	Retrieves current texture coordinates of a tile in the aggregator.
 *
 *	@param	id		Identifier of the tile to retrieve co-ordinates about.
 *	@param	min		(out) will hold the coords' upper-left corner (in texels).
 *	@param	max		(out) will hold the coords' lower-right corner (in texels).
 */
void TextureAggregator::getTileCoords(int id, Vector2 & min, Vector2 & max) const
{
	BW_GUARD;
	TextureAggregatorPimpl::TilesMap::const_iterator tileIt;
	tileIt = this->pimpl_->tiles.find(id);
	IF_NOT_MF_ASSERT_DEV(tileIt != this->pimpl_->tiles.end())
	{
		min.x = 0;
		min.y = 0;
		max.x = 1;
		max.y = 1;
		return;
	}
	SlotPtr slot = tileIt->second.slot;

	min.x = float(slot->x);
	min.y = float(slot->y);
	max.x = float(slot->x + tileIt->second.width);
	max.y = float(slot->y + tileIt->second.height);
}

/**
 *	Deletes a tile from the aggregator.
 *
 *	@param	id	id of the tile to be deleted.
 */
void TextureAggregator::delTile(int id)
{
	BW_GUARD;
	TextureAggregatorPimpl::TilesMap::iterator tileIt;
	tileIt = this->pimpl_->tiles.find(id);
	IF_NOT_MF_ASSERT_DEV(tileIt != this->pimpl_->tiles.end())
	{
		return;
	}
	SlotPtr slot = tileIt->second.slot;
	this->pimpl_->tiles.erase(tileIt);
	this->pimpl_->recycleSlot(slot);

	// there is no need to repack the 
	// texture if it is not currently active
	if (this->pimpl_->texture.hasComObject())
	{
		#define DEBUG_DEL_TILE 0
		#if DEBUG_DEL_TILE
			Vector2 dstMin(
				float(slot->x), 
				float(slot->y));
			Vector2 dstMax(
				float(slot->x + slot->width), 
				float(slot->y + slot->height));

			copyTile(
				16, 16,
				NULL, Vector2(0, 0), Vector2(16, 16),
				this->pimpl_->texture.pComObject(),
				dstMin, dstMax, true);
		#endif

		int height = texHeight(this->pimpl_->texture);
		if (this->pimpl_->textureUsage() < c_RepackThreshold && 
			height > this->pimpl_->minSize)
		{
			// do it later because we are not inside 
			// a begin/end scene block at this point
			this->pimpl_->repackLater();
		}
	}
}

/**
 *	Informs the Aggregator that now is a good time to repack its textures
 */
void TextureAggregator::repack()
{
	BW_GUARD;
	this->pimpl_->repackCheck();
}

/**
 *	Returns true if the tiles have been reset due to repacking since the last
 *	tile this method was called, returning false henceforth. If called every 
 *	frame, this can be used as an alternative to the repack callback. 
 *
 *	@return		true if the aggregator has been repacked since last call,
 *				false otherwise.
 */
bool TextureAggregator::tilesReset() const
{
	BW_GUARD;
	bool result = this->pimpl_->tilesReset;
	this->pimpl_->tilesReset = false;
	return result;
}

/**
 *	Returns the aggregator texture object.
 */
DX::Texture * TextureAggregator::texture() const
{
	BW_GUARD;
	return this->pimpl_->texture.pComObject();
}

/**
 *	Returns the current matrix transform that translates the texture 
 *	coordinates returned by getTileCoords from texels to the [0, 1] range.
 */
const Matrix & TextureAggregator::transform() const
{
	return this->pimpl_->transform;
}

/**
 *	Returns the minimum size the aggregated texture can assume.
 */
int TextureAggregator::minSize() const
{
	return this->pimpl_->minSize;
}

/**
 *	Sets the minimum size the aggregated texture can assume.
 *
 *	@param	minSize	the minimum size the aggregated texture can assume.
 */
void TextureAggregator::setMinSize(int minSize)
{
	this->pimpl_->minSize = minSize;
}

/**
 *	Returns the maximum size the aggregated texture can assume.
 */
int TextureAggregator::maxSize() const
{
	return this->pimpl_->maxSize;
}

/**
 *	Sets the maximum size the aggregated texture can assume.
 *
 *	@param	maxSize	the maximum size the aggregated texture can assume.
 */
void TextureAggregator::setMaxSize(int maxSize)
{
	this->pimpl_->maxSize = maxSize;
}

/**
 *	Returns the number of mipmaps the aggregator should have.
 */
int TextureAggregator::mipLevels() const
{
	return this->pimpl_->mipLevels;
}

/**
 *	Sets the number of mipmaps the aggregator should have.
 *
 *	@param	mipLevels	number of mipmaps the aggregator should have.
 *
 *	@note	If a tile with less mipmap levels than the set here is
 *			added to the aggregator, the lower levels will be left blank.
 */
void TextureAggregator::setMipLevels(int mipLevels)
{
	this->pimpl_->mipLevels = mipLevels;
}

// -----------------------------------------------------------------------------
// Section: TextureAggregatorPimpl
// -----------------------------------------------------------------------------

/**
 *	Default constructor.
 */
TextureAggregatorPimpl::TextureAggregatorPimpl() :
	texture(NULL),
	transform(),
	freeSlots(),
	tiles(),
	nextId(0),
	tilesReset(false),
	repackPending(false),
	minSize(c_MinSize),
	maxSize(c_MaxSize),
	mipLevels(c_MipLevels),
	resetNotifyFunc(NULL)
{}

/**
 *	Returns an empity slot of size 2^log2 x 2^log2.
 *
 *	@param	log2	slot's size's base-2 log.
 *
 *	@return			a slot of the requested size.
 *
 *	@throw			DeviceError if couldn't grow texture to satisfy request.
 */
SlotPtr TextureAggregatorPimpl::getSlot(int log2)
{
	BW_GUARD;
	TextureAggregatorPimpl::SlotVectorMap::iterator slotVecIt;
	slotVecIt = freeSlots.find(log2);

	if (slotVecIt == freeSlots.end() || slotVecIt->second.empty())
	{
		if ( !makeSlots(log2) )
		{
			return NULL;
		}

		slotVecIt = freeSlots.find(log2);

		if ( slotVecIt == freeSlots.end() )
		{
			return NULL;
		}
	}

	SlotPtr slot = slotVecIt->second.back();
	slotVecIt->second.pop_back();

	return slot;
}

/**
 *	Creates slots of size 2^log2 x 2^log2. By recursively splitting slots of 
 *	bigger size. Newly created slots will be placed on the freeSlots container.
 *
 *	@param	log2	slot's size's base-2 log.
 *
 *	@returns		False if couldn't grow texture to satisfy request.
 */
bool TextureAggregatorPimpl::makeSlots(int log2)
{
	BW_GUARD;
	if ( needsToGrow(log2) )
	{
		SlotVector newSlots = growTexture(log2);

		// fail if we didn't make new slots when required.
		if ( newSlots.size() == 0 )
			return false;

		int newLog2 = sizeToLog2(newSlots[0]->width);
		SlotVector & slots = freeSlots[newLog2];
		slots.insert(slots.end(), newSlots.begin(), newSlots.end());
		if (newLog2 != log2)
		{
			return makeSlots(log2);
		}
	}
	else
	{
		SlotPtr biggerSlot = getSlot(log2+1);

		// fail if we couldn't get slot
		if ( biggerSlot == NULL )
			return false;

		SlotVector newSlots = subdivideSlot(biggerSlot);
		SlotVector & slots = freeSlots[log2];
		slots.insert(slots.end(), newSlots.begin(), newSlots.end());
	}

	return true;
}

/**
 *	Queries aggregator about the need to grow the texture in order
 *	to satisfy a given slot request.
 *
 *	@param	log2	slot's size's base-2 log.
 *
 *	@return			True is texture will need to grow, false otherwise.
 */
bool TextureAggregatorPimpl::needsToGrow(int log2) const
{
	BW_GUARD;
	return
		!this->texture.hasComObject() ||
		log2ToSize(log2) >= int(texWidth(this->texture));
}

/**
 *	Grows texture to requested size. Will not grow smaller than minSize.
 *
 *	@param	log2Hint	desired new size.
 *
 *	@return				list of new slots created when growing texture.
 *						or empty vector if growing texture failed.
 */
TextureAggregatorPimpl::SlotVector 
TextureAggregatorPimpl::growTexture(int log2Hint)
{
	BW_GUARD;
	TextureAggregatorPimpl::SlotVector result;

	if (!this->texture.hasComObject())
	{
		int sizeHint    = log2ToSize(log2Hint);
		int size        = std::max(this->minSize, sizeHint);
		this->texture   = this->createTexture(size, size);

		float divFactor = 1.0f/size;
		this->transform = Matrix::identity;
		this->transform._11 = divFactor;
		this->transform._22 = divFactor;

		result.push_back(new Slot(0, 0, size, size));
	}
	else
	{
		int scaleX = 1;
		int scaleY = 1;
		int width  = texWidth(this->texture);
		int height = texHeight(this->texture);
		if (width == height)
		{
			scaleY = 2;
		}
		else
		{
			scaleX = 2;
		}

		DXTexturePtr newTexture = 
			createTexture( width * scaleX, height * scaleY);

		// return empty slot vector if failed
		if ( newTexture )
		{
			this->copyWholeTexture(this->texture, newTexture);
			this->texture = newTexture;

			if (scaleX == 1)
			{
				result.push_back(new Slot(0, height, width, width));
				this->transform._22 *= 0.5f;
			}
			else
			{
				result.push_back(new Slot(width, width, width, width));
				result.push_back(new Slot(width, 0, width, width));
				this->transform._11 *= 0.5f;
			}
		}
	}
	return result;
}

/**
 *	Create a texture.
 *
 *	@param	width	desired texture's width.
 *	@param	height	desired texture's height.
 *
 *	@return			the new texture, if successful.
 *
 */
TextureAggregatorPimpl::DXTexturePtr TextureAggregatorPimpl::createTexture(
	int width, int height)
{
	BW_GUARD;
	DXTexturePtr	texture	= NULL;

	if (width <= this->maxSize && height <= this->maxSize)
	{
		mipLevels = std::min(this->mipLevels, sizeToLog2(this->minSize));
		texture = rc().createTexture(
			width, height, mipLevels, D3DUSAGE_RENDERTARGET, 
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, "texture/aggregator");

		if (texture)
		{
			// The status of the texture preparation.
			bool success = true;

			for (int i=0; i<mipLevels; ++i)
			{
				ComObjectWrap<DX::Surface> surface;
				HRESULT hr = texture->GetSurfaceLevel(i, &surface);
				if (SUCCEEDED(hr))
				{
					bool result = rc().pushRenderTarget();
					IF_NOT_MF_ASSERT_DEV(result)
					{
						return NULL;
					}

					//TODO: this should be using a RenderTarget
					hr = rc().setRenderTarget(0, surface);
					if(rc().mrtSupported())
						hr = rc().setRenderTarget(1, NULL);
					if (SUCCEEDED(hr))
					{
						rc().device()->SetDepthStencilSurface(NULL);
						rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 
							D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0 );
					}
					else
					{
						ERROR_MSG(
							"TextureAggregator::createTexture: "
							"Unable to set render target on device "
							"(ERRORCODE=%lx)\n", hr);
						success = false;
					}

					surface = NULL;
					rc().popRenderTarget();
				}
				else
				{
					ERROR_MSG(
						"TextureAggregator::createTexture: "
						"Could not get surface for texture aggregator"
						"(ERRORCODE=%lx)\n", hr);
					success = false;
				}

				// Don't try to continue if something went wrong.
				if ( !success ) 
					break;
			}

			// If something went wrong, don't return texture 
			if ( !success )
				texture = NULL;
		}
	}

	return texture;
}

/**
 *	Copies one texture into another.
 *
 *	@param	src		source texture.
 *	@param	dst		destination texture.
 */
void TextureAggregatorPimpl::copyWholeTexture(
	DXTexturePtr src, DXTexturePtr dst)
{
	BW_GUARD;
	if (src.hasComObject() && dst.hasComObject())
	{
		int srcWidth  = texWidth(src);
		int srcHeight = texHeight(src);

		int dstWidth  = texWidth(dst);
		int dstHeight = texHeight(dst);

		Vector2 minp(0, 0);
		Vector2 maxp(1, 1);
		Vector2 maxo((float)srcWidth, (float)srcHeight);

		copyTile(
			srcWidth, srcHeight,
			src.pComObject(), minp, maxp,
			dst.pComObject(), minp, maxo,
			false);
	}
}

/**
 *	Renders and registers a tile into aggregator.
 *
 *	@param	tex		tile's source texture.
 *	@param	min		tile's source upper-left corner.
 *	@param	max		tile's source lower-right corner.
 *	@param	width	tile's source width in pixels.
 *	@param	height	tile's source height in pixels.
 *	@param	slot	tile's destination slot.
 *
 *	@return			tile's id in aggregator.
 */
int TextureAggregatorPimpl::registerTile(
	BaseTexturePtr  tex, 
	const Vector2 & min, 
	const Vector2 & max, 
	int width, 
	int height,
	SlotPtr slot)
{
	BW_GUARD;
	this->renderTile(tex, min, max, width, height, slot);

	int id = this->nextId++; // post-increment is intentional
	this->tiles[id] = Tile(tex, min, max, width, height, slot);
	return id;
}

/**
 *	Renders a tile into aggregator.
 *
 *	@param	tex		tile's source texture.
 *	@param	min		tile's source upper-left corner.
 *	@param	max		tile's source lower-right corner.
 *	@param	width	tile's source width in pixels.
 *	@param	height	tile's source height in pixels.
 *	@param	slot	tile's destination slot.
 */
void TextureAggregatorPimpl::renderTile(
	BaseTexturePtr  tex, 
	const Vector2 & min, 
	const Vector2 & max, 
	int width, 
	int height,
	SlotPtr slot)
{
	BW_GUARD;
	if (this->texture.hasComObject())
	{
		Vector2 dstMin(
			float(slot->x), 
			float(slot->y));
		Vector2 dstMax(
			float(slot->x + width),
			float(slot->y + height));

		copyTile(
			tex->width(), tex->height(),
			tex->pTexture(), min, max,
			this->texture.pComObject(), 
			dstMin, dstMax, true);
	}
}

/**
 *	Returns a slot to list of free slots.
 *
 *	@param	slot	slot to be recycled.
 */
void TextureAggregatorPimpl::recycleSlot(SlotPtr slot)
{
	BW_GUARD;
	int log2 = sizeToLog2(slot->width);
	this->freeSlots[log2].push_back(slot);

	SlotVector slots;
	if (this->areNeightboursFree(slot, slots))
	{
		SlotVector::iterator slotIt  = slots.begin();
		SlotVector::iterator slotEnd = slots.end();
		while (slotIt != slotEnd)
		{
			this->killSlot(*slotIt);
			++slotIt;
		}
		int log2 = sizeToLog2(slot->width*2);
		SlotPtr baseSlot = slots.front();
		this->recycleSlot(new Slot(
			baseSlot->x, baseSlot->y, 
			baseSlot->width*2, baseSlot->width*2));
	}
}

/**
 *	Check if four neightbouring tiles are free. If they are, they 
 *	can be sefely collapsed into a single bigger free slot.
 *
 *	@param	slot	seed slot (will check this plus three neightbours).
 *	@param	slots	(out) will hold the four slots that have been checked.
 *
 *	@return			true if the four slots are free, false otherwise.
 *	
 */
bool TextureAggregatorPimpl::areNeightboursFree(
	SlotPtr slot, SlotVector & slots)
{
	BW_GUARD;
	int width   = slot->width;
	int log2    = sizeToLog2(width);
	int coordx  = (slot->x / (width*2))*2;
	int coordy  = (slot->y / (width*2))*2;
	bool isFree = true;
	for (int i=0; i<2; ++i)
	{
		for (int j=0; j<2; ++j)
		{
			SlotPtr slot = this->findSlot(coordx+i, coordy+j, log2);
			isFree = isFree && slot.exists() && this->isFree(slot);
			slots.push_back(slot);
		}
	}
	return isFree;
}

/**
 *	Checks if given slot is free.
 *
 *	@param	slot	slot to check.
 *
 *	@return			true slots is free, false otherwise.
 *	
 */
bool TextureAggregatorPimpl::isFree(SlotPtr slot)
{
	BW_GUARD_PROFILER( TextureAggregatorPimpl_isFree );
	int log2 = sizeToLog2(slot->width);
	SlotVector slots = this->freeSlots[log2];
	return std::find(slots.begin(), slots.end(), slot) != slots.end();
}

/**
 *	Finds slot given size at given coordinates. Slot must be of given size.
 *
 *	@param	coordx	slot's x-coordinate
 *	@param	coordy	slot's y-coordinate
 *	@param	log2	slot's size's base-2 log.
 *
 *	@return			a pointer to the slot, if found. NULL if not found.
 *
 */
SlotPtr TextureAggregatorPimpl::findSlot(int coordx, int coordy, int log2)
{
	BW_GUARD;
	int width = log2ToSize(log2);
	int x = coordx * width;
	int y = coordy * width;
	SlotPtr result = NULL;

	SlotVector slots = this->freeSlots[log2];
	SlotVector::const_iterator slotIt  = slots.begin();
	SlotVector::const_iterator slotEnd = slots.end();
	while (slotIt != slotEnd)
	{
		if ((*slotIt)->x == x && (*slotIt)->y == y)
		{
			result = *slotIt;
			break;
		}
		++slotIt;
	}
	return result;
}

/**
 *	Removes a slot from the aggregator altogether.
 *
 *	@param	slot	slot to remove.
 */
void TextureAggregatorPimpl::killSlot(SlotPtr slot)
{
	BW_GUARD;
	int log2 = sizeToLog2(slot->width);
	SlotVector & slots = this->freeSlots[log2];
	SlotVector::iterator slotIt  = slots.begin();
	SlotVector::iterator slotEnd = slots.end();
	while (slotIt != slotEnd)
	{
		if (*slotIt == slot)
		{	
			(*slotIt) = slots.back();
			slots.pop_back();
			break;
		}
		++slotIt;
	}
}

/**
 *	Queries how much real estate is currently been used on the aggregated 
 *	texture.
 *
 *	@return		a float in the [0,1] range, with 0 meaning the aggregated
 *				texture is empty, and 1 that it's fully occupied.
 */
float TextureAggregatorPimpl::textureUsage() const
{
	BW_GUARD;
	float result = 0;
	if (this->texture.hasComObject())
	{
		int width  = texWidth(this->texture);
		int height = texHeight(this->texture);

		int totalArea = width * height;
		int usedArea  = 0;

		typedef TextureAggregatorPimpl::TilesMap::const_iterator iterator;
		iterator tileIt  = this->tiles.begin();
		iterator tileEnd = this->tiles.end();
		while (tileIt != tileEnd)
		{
			usedArea += tileIt->second.slot->width * tileIt->second.slot->height;
			++tileIt;
		}
		result = float(usedArea) / totalArea;
	}	
	return result;
}

/**
 *	Schedules a texture repack for when the next opportunity arrives.
 *	Pending repacks are processed by the repackCheck method, which is 
 *	triggered whenever a new tile is added.
 */
void TextureAggregatorPimpl::repackLater()
{
	this->repackPending = true;
}

/**
 *	Checks for and processes pending repacks.
 */
void TextureAggregatorPimpl::repackCheck()
{
	BW_GUARD;
	if (this->repackPending)
	{
		// repack resets repackPending
		this->repack();
	}
}

/**
 *	Repacks tiles in the aggregator. This is used to reduce the 
 *	amount of fragmentation on the aggregated texture, and may cause it 
 *	to shrink if there is enough free space on it (the aggregator will, 
 *	in fact, shrink to minSize and grow as all tiles are read to it).
 *
 *	Calling this method will trigger the reset notification function.
 *	Clients must respond to it by rebuilding all vertex buffers whose
 *	texture coordinates refer to this aggregator.
 */
void TextureAggregatorPimpl::repack()
{
	BW_GUARD;
	texture = NULL;
	freeSlots.clear();

	typedef TextureAggregatorPimpl::TilesMap::iterator iterator;
	iterator tileIt  = this->tiles.begin();
	iterator tileEnd = this->tiles.end();
	while (tileIt != tileEnd)
	{
		int log2 = sizeToLog2(tileIt->second.slot->width);

		TextureAggregatorPimpl::SlotPtr newSlot = getSlot(log2);

		if ( newSlot == NULL )
		{
			ERROR_MSG(
				"TextureAggregatorPimpl::repack: getSlot(%d) failed. "
				"A video driver issue is the most likely cause, "
				"check error log for details.", log2);
			return;
		}

		tileIt->second.slot = newSlot;

		renderTile(
			tileIt->second.tex, 
			tileIt->second.min, tileIt->second.max, 
			tileIt->second.width, tileIt->second.height, 
			tileIt->second.slot);
		++tileIt;
	}

	if (this->resetNotifyFunc != NULL)
	{
		this->resetNotifyFunc();	
	}
	else
	{
		this->tilesReset = true;
	}
	this->repackPending = false;
}

/**
 *	Resets the texture aggregator. It must be rebuilt before it can se 
 *	used again (either by repacking it or calling createUnmanagedObjects).
 */
void TextureAggregatorPimpl::deleteUnmanagedObjects()
{
	BW_GUARD;
	this->texture = NULL;
	this->freeSlots.clear();
}

/**
 *	Rebuilds the texture aggregator.
 */
void TextureAggregatorPimpl::createUnmanagedObjects()
{
	BW_GUARD;
	rc().beginScene();
	this->repack();
	rc().endScene();
}

namespace // anonymous
{

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------

/**
 *	Converts units to log2.
 *
 *	@param	size	size in units
 *
 *	@return			size's base-2 log
 */
int sizeToLog2(int size)
{
	int result = 1;
	while ((1 << result) < size)
	{
		++result;
	}
	return result;
}

/**
 *	Converts log2 to size.
 *
 *	@param	log2	size's base-2 log
 *
 *	@return			size in units
 */
int log2ToSize(int log2)
{
	return 1 << log2;
}

/**
 *	Subdivides given slot into it's four smaller constituents quarter slots.
 *
 *	@param	slot	slot to be subdivided.
 *
 *	@return			list with the four resulting slots.
 */
TextureAggregatorPimpl::SlotVector subdivideSlot(SlotPtr slot)
{
	BW_GUARD;
	int height = slot->height >> 1;
	int width  = slot->width  >> 1;
	int x = slot->x;
	int y = slot->y;

	typedef TextureAggregatorPimpl::Slot Slot;
	SlotPtr slot1 = new Slot(x+width, y, height, width);
	SlotPtr slot2 = new Slot(x, y, height, width);
	SlotPtr slot3 = new Slot(x+width, y+height, height, width);
	SlotPtr slot4 = new Slot(x, y+height, height, width);

	TextureAggregatorPimpl::SlotVector result;
	result.push_back(slot1);
	result.push_back(slot2);
	result.push_back(slot3);
	result.push_back(slot4);
	return result;
}

/**
 *	Retrives width of given texture.
 *
 *	@param	tex		texture's from which to retrieve width
 *
 *	@return			width of given texture
 */
int texWidth(TextureAggregatorPimpl::DXTexturePtr tex)
{
	BW_GUARD;
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	return desc.Width;
}

/**
 *	Retrives height of given texture.
 *
 *	@param	tex		texture's from which to retrieve height
 *
 *	@return			height of given texture
 */
int texHeight(TextureAggregatorPimpl::DXTexturePtr tex)
{
	BW_GUARD;
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	return desc.Height;
}

/**
 *	Copies subpart of a texture into another texture.
 *
 *	@param	srcWidth	width of source texture.
 *	@param	srcHeight	height of source texture.
 *	@param	src			source texture.
 *	@param	pSrcMin		upper-left corner of source tile.
 *	@param	pSrcMax		lower-right corner of source tile.
 *	@param	dst			distination texture.
 *	@param	pDstMin		upper-left corner of destination tile.
 *	@param	pDstMax		lower-right corner of destination tile.
 *	@param	cropBorders	true if borders should be cropperd to avoid bleeding.
 */
void copyTile(
	int               srcWidth,
	int               srcHeight,
	DX::BaseTexture * src, 
	const Vector2   & pSrcMin, 
	const Vector2   & pSrcMax,
	DX::Texture     * dst, 
	const Vector2   & pDstMin, 
	const Vector2   & pDstMax,
	bool              cropBorders)
{
	BW_GUARD;
	if (!rc().checkDevice())
	{
		ERROR_MSG("TextureAggregator: device lost when trying to copy tile.\n" );
		return;
	}

	// Make sure we set all texture slots to NULL in case the
	// texture we are updating is set in one of the slots.
	for (uint32 i = 0; i < Moo::rc().maxSimTextures(); i++)
	{
		Moo::rc().setTexture( i, NULL );
	}

	float w0d2 = texWidth(dst)/2.0f;
	float h0d2 = texHeight(dst)/2.0f;

	int srcLevelCount = src != NULL ? src->GetLevelCount() : 0xffff;
	int texMipLevels = std::min(srcLevelCount, int(dst->GetLevelCount()));
	for (int i=0; i<texMipLevels; ++i)
	{
		float tileBorder = cropBorders ? float(c_TileBorder << i) : 0.0f;
		float wPixel     = tileBorder / (srcWidth << i);
		float hPixel     = tileBorder / (srcHeight << i);

		Vector2 srcMin(pSrcMin);
		Vector2 srcMax(pSrcMax);
		srcMin.x += wPixel;
		srcMin.y += hPixel;
		srcMax.x -= wPixel;
		srcMax.y -= hPixel;

		Vector2 dstMin(pDstMin);
		Vector2 dstMax(pDstMax);
		dstMin.x += tileBorder;
		dstMin.y += tileBorder;
		dstMax.x -= tileBorder;
		dstMax.y -= tileBorder;

		ComObjectWrap<DX::Surface> surface;
		HRESULT hr = dst->GetSurfaceLevel(i, &surface);
		if (SUCCEEDED(hr))
		{
			bool result = rc().pushRenderTarget();
			IF_NOT_MF_ASSERT_DEV(result)
			{
				return;
			}

			hr = rc().setRenderTarget(0, surface);
			if (SUCCEEDED(hr))
			{
				if(rc().mrtSupported())
					rc().setRenderTarget(1, NULL);

				rc().device()->SetDepthStencilSurface(NULL);

				// set view port
				D3DSURFACE_DESC desc;
				dst->GetLevelDesc(i, &desc);
				DX::Viewport vp;
				vp.X = vp.Y = 0;
				vp.MinZ = 0;
				vp.MaxZ = 1;
				vp.Width = desc.Width;
				vp.Height = desc.Height;
				rc().setViewport(&vp);

				// set render states
				rc().setVertexShader(NULL);
				rc().setPixelShader(NULL);

				rc().setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				rc().setRenderState(D3DRS_ZENABLE, FALSE);
				rc().setRenderState(D3DRS_LIGHTING, FALSE);

				// TODO: use EffectMaterial here
				Material::setVertexColour();
				rc().setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				rc().setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
				rc().setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
				rc().setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				rc().setRenderState(D3DRS_LIGHTING, FALSE);
				rc().setRenderState(D3DRS_ZWRITEENABLE, FALSE);
				rc().setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
				rc().setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

				rc().setTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
				rc().setTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

				rc().setSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				rc().setSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				rc().setSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
				rc().setSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				rc().setSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

				// set transforms
				rc().device()->SetTransform(D3DTS_VIEW, &Matrix::identity);
				rc().device()->SetTransform(D3DTS_WORLD, &Matrix::identity);
				rc().device()->SetTransform(D3DTS_TEXTURE0, &Matrix::identity);

				Matrix projection;
				projection.orthogonalProjection(
					w0d2*2.0f, h0d2*2.0f, -1.0f, +1.0f);

				rc().device()->SetTransform(D3DTS_PROJECTION, &projection);

				rc().pushRenderState(D3DRS_COLORWRITEENABLE);
				rc().setRenderState(D3DRS_COLORWRITEENABLE, 
					D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | 
					D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

				// paint background black (it
				// may contain some left-overs)
				{
					typedef VertexXYZL VertexType;
					static VertexType v[4];
					v[0].pos_    = Vector3(pDstMin.x-w0d2, h0d2-pDstMax.y, 0);
					v[0].colour_ = 0;
					v[1].pos_    = Vector3(pDstMin.x-w0d2, h0d2-pDstMin.y, 0);
					v[1].colour_ = 0;
					v[2].pos_    = Vector3(pDstMax.x-w0d2, h0d2-pDstMin.y, 0);
					v[2].colour_ = 0;
					v[3].pos_    = Vector3(pDstMax.x-w0d2, h0d2-pDstMax.y, 0);
					v[3].colour_ = 0;

					rc().setTextureStageState(0, 
						D3DTSS_COLOROP, D3DTOP_SELECTARG1);
					rc().setTextureStageState(0, 
						D3DTSS_COLORARG1, D3DTA_DIFFUSE);

					rc().setTextureStageState(0, 
						D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					rc().setTextureStageState(0, 
						D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

					rc().setFVF(VertexType::fvf());
					rc().drawPrimitiveUP(
						D3DPT_TRIANGLEFAN, 2, 
						v, sizeof(VertexType));
				}

				// set texture
				{
					rc().setTexture(0, src);

					// render quad
					typedef VertexXYZDUV VertexType;
					static VertexType v[4];
					float offsetX       = 0.5f/srcWidth;
					float offsetY       = 0.5f/srcHeight;
					v[0].pos_    = Vector3(dstMin.x-w0d2, h0d2-dstMax.y, 0);
					v[0].uv_     = Vector2(srcMin.x+offsetX, (srcMax.y+offsetY));
					v[0].colour_ = 0;
					v[1].pos_    = Vector3(dstMin.x-w0d2, h0d2-dstMin.y, 0);
					v[1].uv_     = Vector2(srcMin.x+offsetX, (srcMin.y+offsetY));
					v[1].colour_ = 0;
					v[2].pos_    = Vector3(dstMax.x-w0d2, h0d2-dstMin.y, 0);
					v[2].uv_     = Vector2(srcMax.x+offsetX, (srcMin.y+offsetY));
					v[2].colour_ = 0;
					v[3].pos_    = Vector3(dstMax.x-w0d2, h0d2-dstMax.y, 0);
					v[3].uv_     = Vector2(srcMax.x+offsetX, (srcMax.y+offsetY));
					v[3].colour_ = 0;

					rc().setTextureStageState(0, 
						D3DTSS_COLOROP, D3DTOP_SELECTARG1);
					rc().setTextureStageState(0, D3DTSS_COLORARG1, 
						src != NULL 
							? D3DTA_TEXTURE
							: D3DTA_DIFFUSE);

					rc().setTextureStageState(0, 
						D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					rc().setTextureStageState(0, D3DTSS_ALPHAARG1,
						src != NULL 
							? D3DTA_TEXTURE
							: D3DTA_DIFFUSE);

					rc().setFVF(VertexType::fvf());
					rc().drawPrimitiveUP(
						D3DPT_TRIANGLEFAN, 2, 
						v, sizeof(VertexType));
				}

				rc().popRenderState();
			}
			else
			{
				WARNING_MSG(
					"TextureAggregator::renderTile : "
					"Unable to set render target on device\n");
			}
			surface = NULL;
			rc().popRenderTarget();
		}
		else
		{
			WARNING_MSG(
				"TextureAggregator::renderTile : "
				"Could not get surface for texture aggregator\n");
		}
	}
}

} // namespace anonymous

} // namespace Moo

BW_END_NAMESPACE

// texture_aggregator.cpp
