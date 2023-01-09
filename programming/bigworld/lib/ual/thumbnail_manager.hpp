/**
 *	ThumbnailManager: Thumbnail generator class
 */

#ifndef THUMBNAIL_MANAGER_HPP
#define THUMBNAIL_MANAGER_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/render_target.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/concurrency.hpp"

namespace ATL
{
	class CImage;
};


BW_BEGIN_NAMESPACE

// Forward declarations
class BoundingBox;
class ThumbnailManager;

/**
 *  Thumbnail Provider base class
 *	Derived classes must have a default constructor, or declare+implement
 *	the factory static themselves instead of using the macros.
 */
class ThumbnailProvider : public ReferenceCount
{
public:

	virtual bool needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size );

	virtual void zoomToExtents( const BoundingBox& bb, const float scale = 1.f );

	/**
	 *  This method is called by the thumbnail manager class to find out if
	 *  the provider supports this file type. If the provider returns true,
	 *  no other providers will be iterated on, so this provider should handle
	 *  the thumbnail.
	 *  NOTE: THIS METHOD IS PERFORMANCE-CRITICAL
	 *  @param manager ThumbnailManager that is requesting the thumbnail.
	 *  @param file full path of the file
	 *  @return true if the file can be handled by the provider, false if not
	 */
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file ) = 0;
	
	/**
	 *  This method is called by the thumbnail manager class to prepare an
	 *  asset before rendering. It's called from a separate thread, so be
	 *  careful with what calls you make. After this method returns, the main
	 *  thread will be notified and the create method of the provider will be
	 *  called.
	 *  NOTE: this method shouldn't get called frequently, only for new items
	 *  or items that require a new thumbnail.
	 *  @param manager ThumbnailManager that is requesting the thumbnail.
	 *  @param file full path of the file
	 *  @return true if successful
	 */
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file ) = 0;

	/**
	 *  This method is called by the thumbnail manager class to render the last
	 *  loaded thumbnail in the provider. A render target is passed as a param
	 *  for the provider to render it's results. If this method returns true,
	 *  the Thumbnail manager class will save the render context to disk to a
	 *  file named as the string "thumb" passed to the "needsCreate" method.
	 *  NOTE: this method shouldn't get called frequently, only for new items
	 *  or items that require a new thumbnail.
	 *  @param manager ThumbnailManager that is requesting the thumbnail.
	 *  @param file full path of the file (the provider shoudn't need it)
	 *  @param rt render target where the primitives will be rendered and later
	 *  saved to disk
	 *  @return true if successful
	 */
	virtual bool render( const ThumbnailManager& manager,
		const BW::wstring& file, Moo::RenderTarget* rt ) = 0;

};
typedef SmartPointer<ThumbnailProvider> ThumbnailProviderPtr;


/**
 *	Interface class for classes that need to receive thumbnail updates
 */
class ThumbnailUpdater
{
public:
	virtual void thumbManagerUpdate( const BW::wstring& longText ) = 0;
};


// Thumbnail manager class
typedef SmartPointer<ThumbnailManager> ThumbnailManagerPtr;

/**
 *	This class implements a manager for creating thumbnails for different asset
 *	types with minimum stalling of the main thread.
 */
class ThumbnailManager : public ReferenceCount
{
public:
	ThumbnailManager();	
	virtual ~ThumbnailManager();

	static void registerProvider( ThumbnailProviderPtr provider );

	void resetPendingRequests( ThumbnailUpdater* updater );
	void stop();

	BW::wstring postfix() const { return postfix_; };
	BW::wstring folder() const { return folder_; };
	int size() const { return size_; };
	COLORREF backColour() const { return backColour_; };

	void postfix( const BW::wstring& postfix ) { postfix_ = postfix; };
	void folder( const BW::wstring& folder ) { folder_ = folder; };
	void size( int size ) { size_ = size; };
	void backColour( COLORREF backColour ) { backColour_ = backColour; };

	void create( const BW::wstring& file, ATL::CImage& img, int w, int h,
		ThumbnailUpdater* updater, bool loadDirectly = false );

	void tick();

	void recalcSizeKeepAspect( int w, int h, int& origW, int& origH ) const;

	// legacy methods
	void stretchImage( ATL::CImage& img, int w, int h, bool highQuality ) const;

private:
	ThumbnailManager( const ThumbnailManager& );
	ThumbnailManager& operator=( const ThumbnailManager& );

	/**
	 *	Helper class that contains data useful for a thread
	 */
	class ThreadData : public SafeReferenceCount
	{
	public:
		ThreadData( const BW::wstring& f, const BW::wstring& t,
			const BW::wstring& p, int w, int h,
			ThumbnailUpdater* updater ) :
			file_( f ),
			thumb_( t ),
			path_( p ),
			memFile_( NULL ),
			provider_( NULL ),
			w_( w ),
			h_( h ),
			updater_( updater )
		{}
		BW::wstring file_;
		BW::wstring thumb_;
		BW::wstring path_;
		LPD3DXBUFFER memFile_;
		ThumbnailProviderPtr provider_;
		int w_;		// actual width of the final image
		int h_;		// actual height of the final image
		ThumbnailUpdater* updater_; // called when the thumb is ready
	};
	typedef SmartPointer<ThreadData> ThreadDataPtr;	


	/**
	 *	Helper class that contains results from a thread
	 */
	class ThreadResult : public SafeReferenceCount
	{
	public:
		ThreadResult( const BW::wstring& file, ATL::CImage* image, ThumbnailUpdater* updater ) :
			file_( file ),
			image_( image ),
			updater_( updater )
		{}
		~ThreadResult();
		BW::wstring file_;
		ATL::CImage* image_;
		ThumbnailUpdater* updater_; // here only used to identify the request
	};
	typedef SmartPointer<ThreadResult> ThreadResultPtr;


	// Member variables
	static BW::vector<ThumbnailProviderPtr> * s_providers_;

	BW::wstring postfix_;
	BW::wstring folder_;
	int size_;
	COLORREF backColour_;

	SimpleThread* thread_;
	SimpleMutex mutex_;
	ThreadDataPtr renderData_;		// used to render in the main thread
	SmartPointer< Moo::RenderTarget > renderRT_;	// used to render in the main thread
	bool renderRequested_;			// used to render in the main thread
	int renderSize_;				// render size that the provider requests
	BW::list<ThreadDataPtr> pending_;
	SimpleMutex pendingMutex_;
	BW::list<ThreadResultPtr> results_;
	SimpleMutex resultsMutex_;
	BW::list<ThreadResultPtr> ready_;
	BW::set<BW::wstring> errorFiles_;
	bool stopThreadRequested_;
	bool updating_;

	// methods used to control thread consumption / production
	bool pendingAvailable();
	bool resultsAvailable();
	// render methods, used to control rendering in the main thread
	void requestRender( int size );
	bool renderRequested();
	bool renderDone();
	void render();
	// thread methods
	static void s_startThread( void *extraData );
	void startThread();
	void stopThread();
	void stopThreadRequest( bool set );
	bool stopThreadRequested();

	// default lights used for render thumbs
	Moo::LightContainerPtr pNewLights_;
};


/**
 *  This class implements a thumbnail provider factory, used to choose and 
 *	generate the appropriate thumbnail for an asset type.
 */
class ThumbProvFactory
{
public:
	ThumbProvFactory( ThumbnailProviderPtr provider )
	{
		ThumbnailManager::registerProvider( provider );
	};
};


/**
 *	This macro is used to declare a class as a thumbnail provider. It is used
 *	to declare the factory functionality. It should appear in the declaration
 *	of the class.
 *
 *	Classes using this macro should also use the IMPLEMENT_THUMBNAIL_PROVIDER
 *  macro.
 */
#define DECLARE_THUMBNAIL_PROVIDER()										\
	static ThumbProvFactory s_factory_;


/**
 *	This macro is used to implement the thumbnail provider factory
 *  functionality.
 */
#define IMPLEMENT_THUMBNAIL_PROVIDER( CLASS )								\
	ThumbProvFactory CLASS::s_factory_( new CLASS() );


BW_END_NAMESPACE

#endif // THUMBNAIL_MANAGER_HPP
