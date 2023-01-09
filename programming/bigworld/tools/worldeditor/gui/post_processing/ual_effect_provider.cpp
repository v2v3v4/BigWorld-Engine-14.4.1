
#include "pch.hpp"
#include "ual_effect_provider.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/bw_string.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

int UalEffectProv_token;


namespace
{
	/**
	 *	This local class stores information about a single post-processing
	 *	Effect in the Asset Browser.
	 */
	class Effect : public BW::wstring
	{
	public:
		Effect(const BW::wstring& name,const BW::wstring& desc):
		  BW::wstring(name),
		  description_(desc)
		{}

		BW::wstring	description_;
	};


	/**
	 *	This local class stores information about all post-processing Effects
	 *	for use in the Asset Browser.
	 */
	class Effects : public BW::vector< Effect >
	{
	public:
		/**
		 *	This method initialises the Effects list when first called.
		 */
		void init()
		{
			BW_GUARD;

			if (s_count_ == 0)
			{
				this->reset();
			}
			++s_count_;
		}


		/**
		 *	This method clears the Effects list when last called.
		 */
		void fini()
		{
			BW_GUARD;

			--s_count_;
			if (s_count_ == 0)
			{
				this->clear();
			}
		}


		/**
		 *	This method retrieves all the post processing Effect using the
		 *	appropriate Python APIs to the PostProcessing Python module.
		 */
		void reset()
		{
			BW_GUARD;

			bool ok = false;

			this->clear();
			PyObject * pPP = PyImport_AddModule( "PostProcessing" );
			if (pPP)
			{
				PyObject * pEffectsAttr = PyObject_GetAttrString( pPP, "getEffectNames" );
				PyObject * pEffects = NULL;
				if (pEffectsAttr)
				{
					pEffects = Script::ask( pEffectsAttr, PyTuple_New(0) );
				}

				if (pEffects && PySequence_Check( pEffects ))
				{
					for (int i = 0; i < PySequence_Size( pEffects ); ++i)
					{
						PyObjectPtr pEffectInfo( PySequence_GetItem( pEffects, i ), PyObjectPtr::STEAL_REFERENCE );
						if (pEffectInfo)
						{
							if ( PySequence_Check(pEffectInfo.get()) )
							{
								PyObjectPtr pEffectName( PySequence_GetItem(pEffectInfo.get(),0), PyObjectPtr::STEAL_REFERENCE );
								PyObjectPtr pDescription( PySequence_GetItem(pEffectInfo.get(),1), PyObjectPtr::STEAL_REFERENCE );
								this->push_back( Effect(
									bw_utf8tow( PyString_AsString(pEffectName.get()) ),
									bw_utf8tow( PyString_AsString(pDescription.get()) ) ) );
								ok = true;
							}
							else
							{
								this->push_back( Effect (
									bw_utf8tow( PyString_AsString(pEffectInfo.get()) ),
									bw_utf8tow( PyString_AsString(pEffectInfo.get()) ) ) );
								ok = true;
							}
						}
					}
				}
				Py_XDECREF( pEffects );
			}

			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
			
			if (!ok)
			{
				INFO_MSG( "UalEffectProvider: Could not find any Post-Processing Effect.\n" );
			}
		}


		/**
		 *	This method returns the Asset Browser's AssetInfo struct for the
		 *	Effect at position "idx".
		 *
		 *	@param idx	Index to the desired Effect's info.
		 *	@return AssetInfo struct for the Effect at position "idx".
		 */
		AssetInfo assetInfo( int idx )
		{
			BW_GUARD;

			return AssetInfo( L"PostProcessingEffect", (*this)[idx], L"effects:" + (*this)[idx], L"", (*this)[idx].description_  );
		}

	private:
		static int s_count_;
	};
	/*static*/ int Effects::s_count_ = 0;

	Effects s_effects;

} // anonymous namespace



///////////////////////////////////////////////////////////////////////////////
// Section: EffectThumbProv
///////////////////////////////////////////////////////////////////////////////

/**
 *	Effect thumbnail provider.
 */
class EffectThumbProv : public ThumbnailProvider
{
public:
	/**
	 *	This method returns true if this provider can handle the specified
	 *	file.
	 */
	bool isValid( const ThumbnailManager& manager, const BW::wstring& file )
	{
		BW_GUARD;

		return
			std::find( s_effects.begin(), s_effects.end(), file ) != s_effects.end();
	}

	/**
	 *	This method returns false always for this provider, no need to do any
	 *	background thumbnail generation.
	 */
	bool needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size )
	{
		BW_GUARD;

		thumb = imageFile_;
		return false;
	}


	/**
	 *	This method returns false and asserts for this provider, no need to do
	 *	any background thumbnail generation.
	 */
	bool prepare( const ThumbnailManager& manager, const BW::wstring& file )
	{
		BW_GUARD;

		// should never get called
		MF_ASSERT( false );
		return false;
	}


	/**
	 *	This method returns false and asserts for this provider, no need to do
	 *	any background thumbnail generation.
	 */
	bool render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt  )
	{
		BW_GUARD;

		// should never get called
		MF_ASSERT( false );
		return false;
	}


	// Set the global thumbnail icon for an Effect in the Asset Browser.
	static void imageFile( const BW::wstring& file ) { imageFile_ = file; }

private:
	static BW::wstring imageFile_;

	DECLARE_THUMBNAIL_PROVIDER()
};

IMPLEMENT_THUMBNAIL_PROVIDER( EffectThumbProv )
/*static*/ BW::wstring EffectThumbProv::imageFile_;



///////////////////////////////////////////////////////////////////////////////
// Section: EffectVFolderProvider
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
EffectVFolderProvider::EffectVFolderProvider( const BW::string& thumb ) :
	index_( 0 ),
	thumb_( thumb )
{
	BW_GUARD;

	s_effects.init();
}


/**
 *	Destructor.
 */
EffectVFolderProvider::~EffectVFolderProvider()
{
	BW_GUARD;

	s_effects.fini();
}


/**
 *	This method is called to prepare the enumerating of items in a VFolder
 *	or subfolder.
 *
 *	@param parent	Parent VFolder, if any.
 *	@return		True of there are items in it, false if empty.
 */
bool EffectVFolderProvider::startEnumChildren( const VFolderItemDataPtr parent )
{
	BW_GUARD;

	index_ = 0;
	return !s_effects.empty();
}


/**
 *	This method is called to iterate to and get the next item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param img		Returns the thumbnail for the item, if available.
 *	@return		Next item in the provider.
 */
VFolderItemDataPtr EffectVFolderProvider::getNextChild(
								ThumbnailManager& manager, CImage& img )
{
	BW_GUARD;

	if ( index_ >= (int)s_effects.size() )
		return NULL;

	AssetInfo info = s_effects.assetInfo( index_ );

	if (info.text().empty() || info.longText().empty())
	{
		return NULL;
	}

	VFolderItemDataPtr newItem = new VFolderItemData(
							this, info, VFolderProvider::GROUP_ITEM, false );

	getThumbnail( manager, newItem, img );

	index_++;

	return newItem;
}


/**
 *	This method creates the thumbnail for an item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param data		Item data.
 *	@param img		Returns the thumbnail for the item, if available.
 */
void EffectVFolderProvider::getThumbnail(
										ThumbnailManager& manager, 
										VFolderItemDataPtr data, CImage& img )
{
	BW_GUARD;

	if ( !data )
		return;

	if ( img_.IsNull() )
	{
		// The image has not been cached yet, so load it into the img_ cache
		// member. Note the 'loadDirectly' param set to true, to load the image
		// directly. This will be done only once, the first time it's
		// requested.
		manager.create(
			bw_utf8tow( BWResource::getFilePath( bw_wtoutf8( UalManager::instance().getConfigFile() ) ) + thumb_ ),
			img_, 16, 16, NULL, true/*loadDirectly*/ );
	}
	// blit the cached image to the return image.
	img.Create( 16, 16, 32 );
	CDC* pDC = CDC::FromHandle( img.GetDC() );
	img_.BitBlt( pDC->m_hDC, 0, 0 );
	img.ReleaseDC();
}


/**
 *	This method returns a text description for the item, good for the dialog's
 *	status bar.
 *
 *	@param data		Item data.
 *	@param numItems	Total number of items, to include in the descriptive text.
 *	@param finished	True if loading of items has finished, false if not, in
 *					which case it is noted in the descriptive text.
 *	@return		Descriptive text for the item.
 */
const BW::wstring EffectVFolderProvider::getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished )
{
	BW_GUARD;

	if ( !data )
		return L"";

	if ( data->isVFolder() )
	{
		// it's the root effect VFolder, so build the appropriate text.
		return Localise(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/EFFECTS_PROVIDER_INFO", s_effects.size());
	}
	else
	{
		// it's an item ( effect ), so return it's editor file.
		return data->assetInfo().longText();
	}
}


/**
 *	This method returns all the information needed to initialise the
 *	appropriate ListProvider for this folder when the folder is clicked for
 *	example.
 *
 *	@param data		Item data.
 *	@param retInitIdString	Return param, used as an id/key for this provider
 *							and its configuration.
 *	@param retListProvider	Return param, list provider matching this VFolder
 *							provider.
 *	@param retItemClicked	Return param, true to call the click callback.
 *	@return		True if there is a valid list provider for this VFolder item.
 */
bool EffectVFolderProvider::getListProviderInfo(
	VFolderItemDataPtr data,
	BW::wstring& retInitIdString,
	ListProviderPtr& retListProvider,
	bool& retItemClicked )
{
	BW_GUARD;

	if ( !data )
		return false;

	retItemClicked = !data->isVFolder();

	retInitIdString = L"";

	if ( listProvider_ )
	{
		// filter the list provider to force a refresh.
		listProvider_->setFilterHolder( filterHolder_ ); 
		listProvider_->refresh();
	}

	retListProvider = listProvider_;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Section: EffectListProvider
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
EffectListProvider::EffectListProvider( const BW::string& thumb ) :
	thumb_( thumb )
{
	BW_GUARD;

	s_effects.init();
}

	
/**
 *	Destructor.
 */
EffectListProvider::~EffectListProvider()
{
	BW_GUARD;

	s_effects.fini();
}


/**
 *	This method starts a rescan for files.
 */
void EffectListProvider::refresh()
{
	BW_GUARD;

	filterItems();
}


/**
 *	This method returns true if the scan thread has finished scanning.
 *
 *	@return		Always true, this provider doesn't do background processing.
 */
bool EffectListProvider::finished()
{
	BW_GUARD;

	return true; // it's not asyncronous
}


/**
 *	This method returns the number of items found during scanning.
 *
 *	@return		Number of items currently in found during scanning.
 */
int EffectListProvider::getNumItems()
{
	BW_GUARD;

	return (int)searchResults_.size();
}


/**
 *	This method returns info object for the item at the given position.
 *
 *	@param index	Index of the item in the list.
 *	@return		Asset info object corresponding to the item.
 */
const AssetInfo EffectListProvider::getAssetInfo( int index )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return AssetInfo();

	return searchResults_[ index ];
}


/**
 *	This method returns the Effect thumbnail.
 *
 *	@param manager	Reference to the thumbnail manager object in use.
 *	@param index	Index of the item in the list.
 *	@param img		Returns here the thumbnail for the item.
 *	@param w		Desired width for the thumbnail.
 *	@param h		Desired height for the thumbnail.
 *	@param updater	Thumbnail creation callback object.
 */
void EffectListProvider::getThumbnail(
			ThumbnailManager& manager,
			int index, CImage& img, int w, int h, ThumbnailUpdater* updater )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return;

	if ( img_.IsNull() || img_.GetWidth() != w || img_.GetHeight() != h )
	{
		// The image has not been cached yet, so load it into the img_ cache
		// member. Note the 'loadDirectly' param set to true, to load the image
		// directly. This will be done only once, the first time it's
		// requested.
		manager.create(
			bw_utf8tow( BWResource::getFilePath( bw_wtoutf8( UalManager::instance().getConfigFile() ) ) + thumb_ ),
			img_, w, h, NULL, true/*loadDirectly*/ );
	}
	// blit the cached image to the return image
	if ( !img_.IsNull() )
	{
		img.Create( w, h, 32 );
		CDC* pDC = CDC::FromHandle( img.GetDC() );
		img_.BitBlt( pDC->m_hDC, 0, 0 );
		img.ReleaseDC();
	}
}


/**
 *	This method filters the list of items found during scanning by the filters
 *	if the filters and/or search text are active.
 */
void EffectListProvider::filterItems()
{
	BW_GUARD;

	searchResults_.clear();

	if ( s_effects.size() == 0 )
		return;

	searchResults_.reserve( s_effects.size() );

	// fill the results vector with the filtered items from the effects
	for( int i = 0; i < (int)s_effects.size(); ++i )
	{
		AssetInfo info = s_effects.assetInfo( i );
		if ( filterHolder_ && filterHolder_->filter( info.text(), info.longText() ) )
		{
			searchResults_.push_back( info );
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Section: UalEffectVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	Effect loader class 'load' method.
 */
VFolderPtr UalEffectVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return NULL;

	beginLoad( dlg, section, customData, 2/*big icon*/ );

	bool showItems = section->readBool( "showItems", false );
	BW::string thumb = section->readString( "thumbnail", "" );
	
	// Set the Effect thumbnail provider's image file name
	EffectThumbProv::imageFile( bw_utf8tow( 
		BWResource::getFilePath(
			bw_wtoutf8( UalManager::instance().getConfigFile() ) ) +
		thumb ) );

	// create VFolder and List providers, specifying the thumbnail to use
	EffectVFolderProvider* prov = new EffectVFolderProvider( thumb );
	prov->setListProvider( new EffectListProvider( thumb ) );

	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );
	ret->setSortSubFolders( true );
	return ret;
}
static UalVFolderLoaderFactory effectFactory( new UalEffectVFolderLoader() );

BW_END_NAMESPACE

