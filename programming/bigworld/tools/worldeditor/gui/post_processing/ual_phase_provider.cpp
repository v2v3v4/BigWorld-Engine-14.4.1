
#include "pch.hpp"
#include "ual_phase_provider.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/bw_string.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

int UalPhaseProv_token;


namespace
{
	/**
	 *	This local class stores information about a single post-processing
	 *	Phase in the Asset Browser.
	 */
	class Phase : public BW::wstring
	{
	public:
		Phase(const BW::wstring& name,const BW::wstring& desc):
		  BW::wstring(name),
		  description_(desc)
		{};
		BW::wstring	description_;
	};


	/**
	 *	This local class stores information about all post-processing Phases
	 *	for use in the Asset Browser.
	 */
	class Phases : public BW::vector< Phase >
	{
	public:
		/**
		 *	This method initialises the Phases list when first called.
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
		 *	This method clears the Phases list when last called.
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
		 *	This method retrieves all the post processing Phase using the
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
				PyObject * pPhasesAttr = PyObject_GetAttrString( pPP, "getPhaseNames" );
				PyObject * pPhases = NULL;
				if (pPhasesAttr)
				{
					pPhases = Script::ask( pPhasesAttr, PyTuple_New(0) );
				}

				if (pPhases && PySequence_Check( pPhases ))
				{
					for (int i = 0; i < PySequence_Size( pPhases ); ++i)
					{
						PyObjectPtr pPhaseInfo( PySequence_GetItem( pPhases, i ), PyObjectPtr::STEAL_REFERENCE );
						if (pPhaseInfo)
						{
							if ( PySequence_Check(pPhaseInfo.get()) )
							{
								PyObjectPtr pPhaseName( PySequence_GetItem(pPhaseInfo.get(),0), PyObjectPtr::STEAL_REFERENCE );
								PyObjectPtr pDescription( PySequence_GetItem(pPhaseInfo.get(),1), PyObjectPtr::STEAL_REFERENCE );
								this->push_back( Phase(
									bw_utf8tow( PyString_AsString(pPhaseName.get()) ),
									bw_utf8tow( PyString_AsString(pDescription.get()) ) ) );
								ok = true;
							}
							else
							{
								this->push_back( Phase (
									bw_utf8tow( PyString_AsString(pPhaseInfo.get()) ),
									bw_utf8tow( PyString_AsString(pPhaseInfo.get()) ) ) );
								ok = true;
							}
						}
					}
				}

				Py_XDECREF( pPhases );
			}

			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
			
			if (!ok)
			{
				INFO_MSG( "UalPhaseProvider: Could not find any Post-Processing Phases.\n" );
			}
		}


		/**
		 *	This method returns the Asset Browser's AssetInfo struct for the
		 *	Phase at position "idx".
		 *
		 *	@param idx	Index to the desired Phase's info.
		 *	@return AssetInfo struct for the Phase at position "idx".
		 */
		AssetInfo assetInfo( int idx )
		{
			BW_GUARD;

			return AssetInfo( L"PostProcessingPhase", (*this)[idx], L"phases:" + (*this)[idx], L"", (*this)[idx].description_ );
		}

	private:
		static int s_count_;
	};
	/*static*/ int Phases::s_count_ = 0;

	Phases s_phases;

} // anonymous namespace



///////////////////////////////////////////////////////////////////////////////
// Section: PhaseThumbProv
///////////////////////////////////////////////////////////////////////////////

/**
 *	Phase thumbnail provider.
 */
class PhaseThumbProv : public ThumbnailProvider
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
			std::find( s_phases.begin(), s_phases.end(), file ) != s_phases.end();
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

IMPLEMENT_THUMBNAIL_PROVIDER( PhaseThumbProv )
/*static*/ BW::wstring PhaseThumbProv::imageFile_;



///////////////////////////////////////////////////////////////////////////////
// Section: PhaseVFolderProvider
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
PhaseVFolderProvider::PhaseVFolderProvider( const BW::string& thumb ) :
	index_( 0 ),
	thumb_( thumb )
{
	BW_GUARD;

	s_phases.init();
}


/**
 *	Destructor.
 */
PhaseVFolderProvider::~PhaseVFolderProvider()
{
	BW_GUARD;

	s_phases.fini();
}


/**
 *	This method is called to prepare the enumerating of items in a VFolder
 *	or subfolder.
 *
 *	@param parent	Parent VFolder, if any.
 *	@return		True of there are items in it, false if empty.
 */
bool PhaseVFolderProvider::startEnumChildren( const VFolderItemDataPtr parent )
{
	BW_GUARD;

	index_ = 0;
	return !s_phases.empty();
}


/**
 *	This method is called to iterate to and get the next item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param img		Returns the thumbnail for the item, if available.
 *	@return		Next item in the provider.
 */
VFolderItemDataPtr PhaseVFolderProvider::getNextChild(
								ThumbnailManager& manager, CImage& img )
{
	BW_GUARD;

	if ( index_ >= (int)s_phases.size() )
		return NULL;

	AssetInfo info = s_phases.assetInfo( index_ );

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
void PhaseVFolderProvider::getThumbnail(
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
const BW::wstring PhaseVFolderProvider::getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished )
{
	if ( !data )
		return L"";

	if ( data->isVFolder() )
	{
		// it's the root phase VFolder, so build the appropriate text.
		return Localise(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PHASES_PROVIDER_INFO", s_phases.size());
	}
	else
	{
		// it's an item ( phase ), so return it's editor file.
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
bool PhaseVFolderProvider::getListProviderInfo(
	VFolderItemDataPtr data,
	BW::wstring& retInitIdString,
	ListProviderPtr& retListProvider,
	bool& retItemClicked )
{
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
// Section: PhaseListProvider
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
PhaseListProvider::PhaseListProvider( const BW::string& thumb ) :
	thumb_( thumb )
{
	s_phases.init();
}

	
/**
 *	Destructor.
 */
PhaseListProvider::~PhaseListProvider()
{
	s_phases.fini();
}


/**
 *	This method starts a rescan for files.
 */
void PhaseListProvider::refresh()
{
	filterItems();
}


/**
 *	This method returns true if the scan thread has finished scanning.
 *
 *	@return		Always true, this provider doesn't do background processing.
 */
bool PhaseListProvider::finished()
{
	return true; // it's not asyncronous
}


/**
 *	This method returns the number of items found during scanning.
 *
 *	@return		Number of items currently in found during scanning.
 */
int PhaseListProvider::getNumItems()
{
	return (int)searchResults_.size();
}


/**
 *	This method returns info object for the item at the given position.
 *
 *	@param index	Index of the item in the list.
 *	@return		Asset info object corresponding to the item.
 */
const AssetInfo PhaseListProvider::getAssetInfo( int index )
{
	if ( index < 0 || getNumItems() <= index )
		return AssetInfo();

	return searchResults_[ index ];
}


/**
 *	This method returns the Phase thumbnail.
 *
 *	@param manager	Reference to the thumbnail manager object in use.
 *	@param index	Index of the item in the list.
 *	@param img		Returns here the thumbnail for the item.
 *	@param w		Desired width for the thumbnail.
 *	@param h		Desired height for the thumbnail.
 *	@param updater	Thumbnail creation callback object.
 */
void PhaseListProvider::getThumbnail(
			ThumbnailManager& manager,
			int index, CImage& img, int w, int h, ThumbnailUpdater* updater )
{
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
void PhaseListProvider::filterItems()
{
	searchResults_.clear();

	if ( s_phases.size() == 0 )
		return;

	searchResults_.reserve( s_phases.size() );

	// fill the results vector with the filtered items from the phases
	for( int i = 0; i < (int)s_phases.size(); ++i )
	{
		AssetInfo info = s_phases.assetInfo( i );
		if ( filterHolder_ && filterHolder_->filter( info.text(), info.longText() ) )
		{
			searchResults_.push_back( info );
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Section: UalPhaseVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	Phase loader class 'load' method.
 */
VFolderPtr UalPhaseVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	if ( !dlg || !section || !test( section->sectionName() ) )
		return NULL;

	beginLoad( dlg, section, customData, 2/*big icon*/ );

	bool showItems = section->readBool( "showItems", false );
	BW::string thumb = section->readString( "thumbnail", "" );
	
	// Set the Phase thumbnail provider's image file name
	PhaseThumbProv::imageFile( bw_utf8tow( 
		BWResource::getFilePath(
			bw_wtoutf8( UalManager::instance().getConfigFile() ) ) +
		thumb ) );

	// create VFolder and List providers, specifying the thumbnail to use
	PhaseVFolderProvider* prov = new PhaseVFolderProvider( thumb );
	prov->setListProvider( new PhaseListProvider( thumb ) );

	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );
	ret->setSortSubFolders( true );
	return ret;
}
static UalVFolderLoaderFactory phaseFactory( new UalPhaseVFolderLoader() );
BW_END_NAMESPACE

