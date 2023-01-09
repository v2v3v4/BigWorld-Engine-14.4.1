/**
 *	Model Thumbnail Provider
 */


#ifndef MODEL_THUMB_PROV_HPP
#define MODEL_THUMB_PROV_HPP

#include "cstdmf/bw_set.hpp"

#include "thumbnail_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements an Model Thumbnail Provider, used for .model files.
 */
class ModelThumbProv : public ThumbnailProvider
{
public:
	ModelThumbProv();
	~ModelThumbProv();
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size );
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt );

private:
	DECLARE_THUMBNAIL_PROVIDER()

	Moo::LightContainerPtr lights_;
	Moo::VisualPtr visual_;
	BW::set< BW::string > errorModels_;
};

BW_END_NAMESPACE

#endif // MODEL_THUMB_PROV_HPP
