
#ifndef SPEEDTREE_THUMB_PROV_HPP
#define SPEEDTREE_THUMB_PROV_HPP

// Module Interface
#include "speedtree_config_lite.hpp"

#if SPEEDTREE_SUPPORT // -------------------------------------------------------

// Base class
#include "ual/thumbnail_manager.hpp"


BW_BEGIN_NAMESPACE

namespace speedtree {

class SpeedTreeRenderer;

/**
 *	Implements a thumb nail provider for SPT files. This class is 
 *	automatically linked into all BW editors that link with speedtree.lib. 
 */
class SpeedTreeThumbProv : public ThumbnailProvider
{
	DECLARE_THUMBNAIL_PROVIDER()

public:
	SpeedTreeThumbProv() : tree_( 0 ) {};
	virtual bool isValid(const ThumbnailManager& manager, const BW::wstring& file);
	virtual bool prepare(const ThumbnailManager& manager, const BW::wstring & file);
	virtual bool render( const ThumbnailManager& manager, const BW::wstring & file, Moo::RenderTarget * rt );
private:
	SpeedTreeRenderer* tree_;
};

} // namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_SUPPORT
#endif // SPEEDTREE_THUMB_PROV_HPP
