#include "pch.hpp"

#include "awesomium_provider.hpp"

#if ENABLE_AWESOMIUM

#include "Awesomium/WebCore.h"
#include "Awesomium/BitmapSurface.h"
#include "Awesomium/STLHelpers.h"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "moo/render_context.hpp"
#include "moo/device_callback.hpp"

#include "input/vk_map.hpp"

#include <DelayImp.h>

#pragma comment (lib, "awesomium.lib" )


BW_BEGIN_NAMESPACE

//static
BW::set<PyAwesomiumProvider*> PyAwesomiumProvider::s_providers_;
bool PyAwesomiumProvider::s_webCoreInitialised_ = false;
BWResourceInterceptor* PyAwesomiumProvider::pResourceInterceptor_( NULL );
HMODULE PyAwesomiumProvider::s_awesomiumModule_ = NULL;

static const char* AWESOMIUM_DLL = "awesomium.dll";
static const char* AWESOMIUM_RES_PATH = "awesomium/";


PY_TYPEOBJECT( PyAwesomiumProvider )
PY_BEGIN_METHODS( PyAwesomiumProvider )
	PY_METHOD( updateTexture )
	PY_METHOD( goBack )
	PY_METHOD( goForward )
	PY_METHOD( stop )
	PY_METHOD( focus )
	PY_METHOD( unfocus )
	PY_METHOD( loadURL )
	PY_METHOD( reload )
	PY_METHOD( resize )
	PY_METHOD( injectKeyEvent )
	PY_METHOD( injectMouseWheelEvent )
	PY_METHOD( injectMouseMoveEvent )
	PY_METHOD( executeJavascript )
	PY_METHOD( executeJavascriptWithResult )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAwesomiumProvider )
	/*~ attribute AwesomiumProvider.isLoading
	 *
	 *	Determines whether or not the page is currently being loaded.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( isLoading )

	/*~ attribute AwesomiumProvider.url
	 *
	 *	Determines the currently loaded url.
	 *
	 *	@type string
	 */
	PY_ATTRIBUTE( url )

	/*~ attribute AwesomiumProvider.width
	 *
	 *	The width of the web view in pixels. Use setSize to resize.
	 *
	 *	@type int
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute AwesomiumProvider.height
	 *
	 *	The height of the web view in pixels. Use setSize to resize.
	 *
	 *	@type int
	 */
	PY_ATTRIBUTE( height )
	/*~ attribute AwesomiumProvider.script
	 *
	 *	Attaches a script object to be used for callback events. This can be
	 *	any Python class which optionally implements the following callbacks:
	 *
	 *	@{
	 *		def onChangeTitle( self, title ):
	 *		def onChangeAddressBar( self, url ):
	 *		def onChangeTooltip( self, tooltip ):
	 *		def onChangeTargetURL( self, url ):
	 *		def onFinishLoadingFrame( self, frameId, isMainFrame, url ):
	 *		def onDocumentReady( self, url ):
	 *	@}
	 *
	 *	@type PyObject
	 */
	PY_ATTRIBUTE( script )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PyAwesomiumProvider, "AwesomiumProvider", BigWorld )

using namespace Awesomium;

namespace {

MouseButton convertMouseButton( KeyCode::Key key )
{
	if (key == KeyCode::KEY_LEFTMOUSE)
	{
		return kMouseButton_Left;
	}
	else if (key == KeyCode::KEY_RIGHTMOUSE)
	{
		return kMouseButton_Right;
	}
	else if (key == KeyCode::KEY_MIDDLEMOUSE)
	{
		return kMouseButton_Middle;
	}
	else
	{
		MF_ASSERT( !"convertMouseButton: non-mouse code given" );
		return kMouseButton_Left;
	}
}

int convertModifiers( const KeyEvent& event )
{
	int m = 0;
	if (event.isAltDown())
	{
		m |= WebKeyboardEvent::kModAltKey;
	}
	if (event.isCtrlDown())
	{
		m |= WebKeyboardEvent::kModControlKey;
	}
	if (event.isShiftDown())
	{
		m |= WebKeyboardEvent::kModShiftKey;
	}

	return m;
}

BW::string bwString( const WebString& str ) 
{
	BW::string result;

	if (str.IsEmpty())
	{
		return BW::string();
	}

	unsigned int len = str.ToUTF8( NULL, 0 );

	result.resize( len, ' ' );
	str.ToUTF8( &result[0], len );

	return result;
}

const char* extensionToMime( const BW::StringRef& ext )
{
	// Not exhaustive, but should cover common cases.
	if (ext == "html" || ext == "htm" || ext == "htmls")
	{
		return "text/html";
	}
	else if (ext == "css")
	{
		return "text/css";
	}
	else if (ext == "xml" || ext == "xhtml")
	{
		return "text/xml";
	}
	else if (ext == "js")
	{
		return "text/javascript";
	}
	else if (ext == "jpeg" || ext == "jpg")
	{
		return "image/jpeg";
	}
	else if (ext == "gif")
	{
		return "image/gif";
	}
	else if (ext == "png")
	{
		return "image/png";
	}
	else if (ext == "swf")
	{
		return "application/x-shockwave-flash";
	}
	else
	{
		// Sensible default?
		return "text/plain";
	}
}


} // anonymous namespace


// ----------------------------------------------------------------------------
// Section: BWResourceInterceptor
// ----------------------------------------------------------------------------

class BWResourceInterceptor : public ResourceInterceptor
{
	Awesomium::ResourceResponse* OnRequest(
		Awesomium::ResourceRequest* request)
	{
		if (request->url().scheme().Compare( WSLit("file") ) == 0)
		{
			BW::string path = bwString( request->url().path() );
			
			BinaryPtr blob = 
				BWResource::instance().fileSystem()->readFile( path );
			if (blob)
			{
				const char* mimeType = 
					extensionToMime( BWResource::getExtension(path) );

				return ResourceResponse::Create(
					blob->len(),
					(unsigned char*)blob->data(), 
					WSLit( mimeType ) );
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}
};

// ----------------------------------------------------------------------------
// Section: AwesomiumTexture
// ----------------------------------------------------------------------------
class AwesomiumTexture : public Moo::BaseTexture, Moo::DeviceCallback
{
public:
	AwesomiumTexture( int width, int height ) 
		: width_(width), height_(height), forceUpdate_(true)
	{
		BW_GUARD;
		this->createUnmanagedObjects();
	}

	~AwesomiumTexture()
	{
		BW_GUARD;
		this->deleteUnmanagedObjects();
	}

	void resize( int width, int height )
	{
		BW_GUARD;
		this->deleteUnmanagedObjects();
		width_ = width;
		height_ = height;
		this->createUnmanagedObjects();
	}

	bool recreateForD3DExDevice() const
	{
		return true;
	}

	void deleteUnmanagedObjects()
	{
		BW_GUARD;
		pTex_ = NULL;
	}

	void createUnmanagedObjects()
	{
		BW_GUARD;
		MF_ASSERT( !pTex_.hasComObject() );

		pTex_ = Moo::rc().createTexture( width_, height_, 1, D3DUSAGE_DYNAMIC,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, "AwesomiumTexture" );

		D3DLOCKED_RECT lockedRect;
		HRESULT hr = pTex_->LockRect( 0, &lockedRect,
			NULL, D3DLOCK_DISCARD );
		if (hr == S_OK)
		{
			memset( lockedRect.pBits, 0xFFFFFFFF, lockedRect.Pitch*height_ );
			pTex_->UnlockRect( 0 );
		}

		forceUpdate_ = true;
	}

	void copyWebView( WebView* pView )
	{
		BW_GUARD;

		BitmapSurface* pSurface =
			static_cast<Awesomium::BitmapSurface*>( pView->surface() );
		if (pTex_ && pSurface && (pSurface->is_dirty() || forceUpdate_))
		{
			MF_ASSERT( pSurface->width() == width_ );
			MF_ASSERT( pSurface->height() == height_ );

			D3DLOCKED_RECT lockedRect;
			HRESULT hr = pTex_->LockRect( 0, &lockedRect,
				NULL, D3DLOCK_DISCARD );
			if (hr == S_OK)
			{
				pSurface->CopyTo( (unsigned char*)lockedRect.pBits, 
					lockedRect.Pitch, 4, false, false );
				pTex_->UnlockRect( 0 );
			}
			else
			{
				ERROR_MSG( "AwesomiumTexture::copyWebView: "
						"failed to lock D3D texture (%s).\n",
					DX::errorAsString( hr ).c_str() );
			}

			forceUpdate_ = false;
		}
	}

	DX::BaseTexture* pTexture()  { return pTex_.pComObject(); }
	uint32 width() const  { return width_; }
	uint32 height() const  { return height_; }
	D3DFORMAT format() const { return D3DFMT_A8R8G8B8; }
	uint32 textureMemoryUsed() const { return width_ * height_ * 4; }

private:
	ComObjectWrap<DX::Texture> pTex_;
	int width_;
	int height_;
	bool forceUpdate_;
};

// ----------------------------------------------------------------------------
// Section: PyAwesomiumProvider::Listener
// ----------------------------------------------------------------------------
class PyAwesomiumProvider::Listener : 
	public WebViewListener::View, 
	public WebViewListener::Load
{
public:
	Listener( PyAwesomiumProvider* provider ) : provider_( provider )
	{
		BW_GUARD;
		MF_ASSERT( provider != NULL );
	}

	~Listener()
	{
	}

	const PyObjectPtr& pObject() 
	{
		return pObject_; 
	}

	void pObject( const PyObjectPtr& pObject )
	{
		pObject_ = pObject;
	}

	// WebViewListener::View
	virtual void OnChangeTitle( Awesomium::WebView* caller,
		const Awesomium::WebString& title )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string str = bwString( title );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onChangeTitle" ),
				Py_BuildValue( "(s)", str.c_str() ),
				"PyAwesomiumProvider::onChangeTitle: ",
				true );
		}
	}

	virtual void OnChangeAddressBar( Awesomium::WebView* caller,
		const Awesomium::WebURL& url )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onChangeAddressBar" ),
				Py_BuildValue( "(s)", spec.c_str() ),
				"PyAwesomiumProvider::onChangeAddressBar: ",
				true );
		}
	}

	virtual void OnChangeTooltip( Awesomium::WebView* caller,
		const Awesomium::WebString& tooltip )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string str = bwString( tooltip );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onChangeTooltip" ),
				Py_BuildValue( "(s)", str.c_str() ),
				"PyAwesomiumProvider::onChangeTooltip: ",
				true );
		}
	}

	virtual void OnChangeTargetURL( Awesomium::WebView* caller,
		const Awesomium::WebURL& url )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onChangeTargetURL" ),
				Py_BuildValue( "(s)", spec.c_str() ),
				"PyAwesomiumProvider::onChangeTargetURL: ",
				true );
		}
	}

	virtual void OnChangeCursor( Awesomium::WebView* caller,
		Awesomium::Cursor cursor )
	{
		BW_GUARD;
		if (pObject_)
		{
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onChangeCursor" ),
				Py_BuildValue( "(i)", cursor ),
				"PyAwesomiumProvider::onChangeCursor: ",
				true );
		}
	}

	virtual void OnChangeFocus( Awesomium::WebView* caller,
		Awesomium::FocusedElementType focused_type )
	{
		BW_GUARD;
	}

	virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
		Awesomium::WebView* new_view,
		const Awesomium::WebURL& opener_url,
		const Awesomium::WebURL& target_url,
		const Awesomium::Rect& initial_pos,
		bool is_popup )
	{
		BW_GUARD;
	}

	// WebViewListener::Load
	virtual void OnBeginLoadingFrame( Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url,
		bool is_error_page )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onBeginLoadingFrame" ),
				Py_BuildValue( "(Lis)", frame_id, is_main_frame, spec.c_str() ),
				"PyAwesomiumProvider::onBeginLoadingFrame: ",
				true );
		}
	}

	virtual void OnFailLoadingFrame( Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url,
		int error_code,
		const Awesomium::WebString& error_desc )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onFailLoadingFrame" ),
				Py_BuildValue( "(Liis)", frame_id, is_main_frame, error_code, spec.c_str() ), 
				"PyAwesomiumProvider::onFailLoadingFrame: ",
				true );
		}
	}

	virtual void OnFinishLoadingFrame( Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url )
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call(
				PyObject_GetAttrString( pObject_.get(), "onFinishLoadingFrame" ),
				Py_BuildValue( "(Lis)", frame_id, is_main_frame, spec.c_str() ), 
				"PyAwesomiumProvider::onFinishLoadingFrame: ",
				true );
		}
	}

	virtual void OnDocumentReady( Awesomium::WebView* caller,
		const Awesomium::WebURL& url ) 
	{
		BW_GUARD;
		if (pObject_)
		{
			BW::string spec = bwString( url.spec() );
			Script::call( 
				PyObject_GetAttrString( pObject_.get(), "onDocumentReady" ),
				Py_BuildValue( "(s)", spec.c_str() ), 
				"PyAwesomiumProvider::onDocumentReady: ",
				true );
		}
	}

private:
	PyAwesomiumProvider* provider_;
	PyObjectPtr pObject_;
};

// ----------------------------------------------------------------------------
// Section: PyAwesomiumProvider
// ----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PyAwesomiumProvider::PyAwesomiumProvider( int width,
									 int height,
									 PyTypePlus * pType ) :
	PyTextureProvider( pType ),
	pWebView_( NULL ),
	pListener_( NULL )
{
	BW_GUARD;

	if (!PyAwesomiumProvider::checkWebCore())
	{
		return;
	}

	WebCore* pCore = WebCore::instance();
	if (pCore)
	{
		pWebView_ = pCore->CreateWebView( width, height );
		if (pWebView_)
		{
			pListener_ = new Listener( this );

			pWebView_->set_view_listener( pListener_ );
			pWebView_->set_load_listener( pListener_ );

			pTexture_ = new AwesomiumTexture( width, height );
			s_providers_.insert( this );
		}
		else
		{
			ERROR_MSG( "PyAwesomiumProvider::PyAwesomiumProvider: "
				"Failed to create %dx%d web view.\n", width, height );
		}
	}
	else
	{
		ERROR_MSG( "PyAwesomiumProvider::PyAwesomiumProvider: "
			"Awesomium not initialisd.\n" );
	}
}

/**
 *	Destructor.
 */
PyAwesomiumProvider::~PyAwesomiumProvider()
{
	BW_GUARD;
	this->cleanup();
	s_providers_.erase( this );
}

/*~ function AwesomiumProvider.updateTexture
 *
 *	Updates the texture with the latest web view content.
 *	This should never have to be called manually.
 *
 */
void PyAwesomiumProvider::updateTexture()
{
	BW_GUARD;
	if (!pWebView_ || !pTexture_)
	{
		return;
	}

	AwesomiumTexture* pAwesomiumTexture =
		static_cast<AwesomiumTexture*>( pTexture_.get() );
	MF_ASSERT( pAwesomiumTexture != NULL );

	pAwesomiumTexture->copyWebView( pWebView_ );
}

/** 
 *	PyTextureProvider access to BaseTexture.
 */
Moo::BaseTexturePtr PyAwesomiumProvider::texture()
{
	BW_GUARD;
	this->updateTexture();
	return PyTextureProvider::texture();
}

/*~ function AwesomiumProvider.loadURL
 *
 *	Navigates the web view to the given URL. Note, it must include
 *	the full protocol prefix in order for this to work. e.g.
 *
 *	"http://www.google.com/" not "www.google.com"
 *
 *	@param url	Full URL to load.
 */
void PyAwesomiumProvider::loadURL( const BW::string& url )
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->LoadURL( WebURL( WSLit( url.c_str() ) ) );
	}
}

/** 
 *	Accesssor for isLoading attribute.
 */
bool PyAwesomiumProvider::isLoading()
{
	BW_GUARD;
	return pWebView_ ? pWebView_->IsLoading() : false;
}

/** 
 *	Accesssor for url attribute.
 */
BW::string PyAwesomiumProvider::url()
{
	BW_GUARD;
	if (pWebView_)
	{
		WebURL url = pWebView_->url();
		return bwString( url.spec() );
	}
	else
	{
		return "";
	}
}

/** 
 *	Accesssor for width attribute.
 */
uint32 PyAwesomiumProvider::width()
{
	BW_GUARD;
	if (pTexture_)
	{
		AwesomiumTexture* pAwesomiumTexture =
			static_cast<AwesomiumTexture*>( pTexture_.get() );
		return pAwesomiumTexture->width();
	}
	else
	{
		return 0;
	}
}

/** 
 *	Accesssor for height attribute.
 */
uint32 PyAwesomiumProvider::height()
{
	BW_GUARD;
	if (pTexture_)
	{
		AwesomiumTexture* pAwesomiumTexture =
			static_cast<AwesomiumTexture*>( pTexture_.get() );
		return pAwesomiumTexture->height();
	}
	else
	{
		return 0;
	}
}

/*~ function AwesomiumProvider.stop
 *
 *	Stop loading the currently loading page.
 */
void PyAwesomiumProvider::stop()
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->Stop();
	}
}

/*~ function AwesomiumProvider.reload
 *
 *	Reloads the currently loaded page.
 *
 *	@param ignoreCache	Whether or not to ignore the local browser cache.
 */
void PyAwesomiumProvider::reload( bool ignoreCache )
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->Reload( ignoreCache );
	}
}

/*~ function AwesomiumProvider.goBack
 *
 *	Navigates backward in the browsing history.
 */
void PyAwesomiumProvider::goBack()
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->GoBack();
	}
}

/*~ function AwesomiumProvider.goForward
 *
 *	Navigates forward in the browsing history.
 */
void PyAwesomiumProvider::goForward()
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->GoForward();
	}
}

/*~ function AwesomiumProvider.focus
 *
 *	Sets the input focus flag on the web view for rendering purposes (e.g.
 *	set blinking cursor). This should be called before injecting any
 *	key events.
 */
void PyAwesomiumProvider::focus()
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->Focus();
	}
}

/*~ function AwesomiumProvider.focus
 *
 *	Unsets the input focus flag on the web view for rendering purposes.
 *	This is usually called when the user clicks outside the web view.
 */
void PyAwesomiumProvider::unfocus()
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->Unfocus();
	}
}

/*~ function AwesomiumProvider.resize
 *
 *	Resizes the web view and its corresponding texture.
 *
 *	@param width	Desired width in pixels
 *	@param height	Desired height in pixels
 *
 */
void PyAwesomiumProvider::resize( int width, int height )
{
	BW_GUARD;
	AwesomiumTexture* pAwesomiumTexture =
		static_cast<AwesomiumTexture*>( pTexture_.get() );
	if (pAwesomiumTexture)
	{
		pAwesomiumTexture->resize( width, height );
	}

	if (pWebView_)
	{
		pWebView_->Resize( width, height );
	}
}

/*~ function AwesomiumProvider.injectKeyEvent
 *
 *	Injects a key event into the web view (including keyboard and mouse).
 *
 *	@param event	PyKeyEvent instance for the event
 *
 */
void PyAwesomiumProvider::injectKeyEvent( const KeyEvent& event )
{
	BW_GUARD;
	if (!pWebView_)
	{
		return;
	}

	if (event.isMouseButton())
	{
		MouseButton webButton = convertMouseButton( event.key() );
		if (event.isKeyDown())
		{
			pWebView_->InjectMouseDown( webButton );
		}
		else
		{
			pWebView_->InjectMouseUp( webButton );
		}
	}
	else
	{
		WebKeyboardEvent webEvent;
		if (event.isKeyDown())
		{
			webEvent.type = WebKeyboardEvent::kTypeKeyDown;
		}
		else
		{
			webEvent.type = WebKeyboardEvent::kTypeKeyUp;
		}
		
		webEvent.is_system_key = false;
		webEvent.virtual_key_code = VKMap::toVKey( event.key() );
		webEvent.modifiers = convertModifiers( event );

		char* buf = new char[20];
		GetKeyIdentifierFromVirtualKeyCode( webEvent.virtual_key_code, &buf );
		strcpy( webEvent.key_identifier, buf );
		delete[] buf;

		webEvent.native_key_code =
			MapVirtualKey( webEvent.virtual_key_code, MAPVK_VK_TO_VSC );

		// TODO: somehow work this out? does it really matter?
		webEvent.is_system_key = false;

		pWebView_->InjectKeyboardEvent( webEvent );

		// Insert a separate event for characters
		const wchar_t* chr = event.utf16Char();
		if (chr[0])
		{
			webEvent = WebKeyboardEvent(
				WM_CHAR, MAKEWPARAM( chr[0], chr[1] ), 0 );
			pWebView_->InjectKeyboardEvent( webEvent );
		}
	}
}

/*~ function AwesomiumProvider.injectMouseMoveEvent
 *
 *	Injects a mouse move event into the Awesomium web view.
 *
 *	@param x	Horizontal position in web view coordinates.
 *	@param y	Vertical position in web view coordinates.
 *
 */
void PyAwesomiumProvider::injectMouseMoveEvent( int x, int y )
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->InjectMouseMove( x, y );
	}
}

/*~ function AwesomiumProvider.injectMouseWheelEvent
 *
 *	Injects a mouse wheel event into the Awesomium web view.
 *
 *	@param dz	Distance to scroll in pixels.
 *
 */
void PyAwesomiumProvider::injectMouseWheelEvent( int dz )
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->InjectMouseWheel( dz, 0 );
	}
}

/*~ function AwesomiumProvider.cleanup
 *
 *	Allows the scripts to force cleanup of the web view and texture.
 *	The provider becomes unusable after this is called.
 */
void PyAwesomiumProvider::cleanup()
{
	BW_GUARD;
	pTexture_ = NULL;
	if (pWebView_)
	{
		pWebView_->set_load_listener( NULL );
		pWebView_->set_view_listener( NULL );
		pWebView_->Destroy();
		pWebView_ = NULL;
	}

	bw_safe_delete( pListener_ );
}

/**
 *	Access to the script object.
 */
void PyAwesomiumProvider::script( const PyObjectPtr& object )
{
	BW_GUARD;
	if (pListener_)
	{
		pListener_->pObject( object );
	}
}

/**
 *	Access to the script object.
 */
PyObjectPtr PyAwesomiumProvider::script()
{
	return pListener_ ? pListener_->pObject() : NULL;
}

/*~ function AwesomiumProvider.executeJavascript
 *
 *	Executes some JavaScript asynchronously on the page.
 *
 *	@param script The string of JavaScript to execute.
 *	@param frameXPath The xpath of the frame to execute within; 
 *						leave this blank to execute in the main frame.
 */
void PyAwesomiumProvider::executeJavascript( const BW::string& script, 
											const BW::string& frameXPath )
{
	BW_GUARD;
	if (pWebView_)
	{
		pWebView_->ExecuteJavascript( 
			WSLit( script.c_str() ), 
			WSLit( frameXPath.c_str() ) );
	}
}

/*~ function AwesomiumProvider.executeJavascriptWithResult
 *
 *	Executes some JavaScript synchronously on the page and returns a result.
 *
 *	@param script The string of JavaScript to execute.
 *	@param frameXPath The xpath of the frame to execute within; 
 *						leave this blank to execute in the main frame.
 */
BW::string PyAwesomiumProvider::executeJavascriptWithResult( const BW::string& script, 
														   const BW::string& frameXPath )
{
	BW_GUARD;
	if (pWebView_)
	{
		JSValue result = pWebView_->ExecuteJavascriptWithResult(
			WSLit( script.c_str() ),
			WSLit( frameXPath.c_str() ) );

		// TODO: we could probably be smarter and convert to native Python.
		return bwString( result.ToString() );
	}
	else
	{
		return "";
	}
}


/**
 *	Static python factory method
 */
// static
PyObject * PyAwesomiumProvider::pyNew( PyObject * args )
{
	BW_GUARD;
	int w, h;
	if (!PyArg_ParseTuple( args, "ii", &w, &h ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.PyAwesomiumProvider() "
			"expects width and height integer arguments" );
		return NULL;
	}

	return new PyAwesomiumProvider( w, h );
}

/**
 *	We don't actually do anything in this init function, as the WebCore
 *	is initialised lazily on its first use. This is to allow delayed DLL
 *	load on the Awesomium library and avoid loading the DLL at all on
 *	distributions that don't need it (at the same time avoiding #ifdef's).
 *	This function is just here for symmetry with fini().
 */
//static 
void PyAwesomiumProvider::init()
{
	BW_GUARD;
}

/**
 *	Static function called to check WebCore is initialised.
 *	Used for lazy loading of Awesomium on the first time a provider is created.
 */
//static 
bool PyAwesomiumProvider::checkWebCore()
{
	BW_GUARD;
	if (s_webCoreInitialised_)
	{
		return true;
	}

	// We support having Awesomimum located either next to the 
	// client executable, or within the res path.
	BW::string workingDirPath = BWResource::appDirectory();

	if (!BWResource::fileAbsolutelyExists( workingDirPath + AWESOMIUM_DLL ))
	{
		BW::string relDllPath = BW::string( AWESOMIUM_RES_PATH ) + AWESOMIUM_DLL;
		if (!BWResource::fileExists( relDllPath ))
		{
			ERROR_MSG( "Awesomium failed to initialise: '%s' not found.\n",
				AWESOMIUM_DLL );
			return false;
		}

		// get full path without trailing slash
		workingDirPath = BWResolver::resolveFilename( relDllPath );
		std::replace( workingDirPath.begin(), workingDirPath.end(), '/', '\\' );

		uint idx = workingDirPath.find_last_of( '\\' );
		if (idx == workingDirPath.npos)
		{
			ERROR_MSG( "Awesomium failed to initialise.\n" );
			return false;
		}

		workingDirPath = workingDirPath.substr( 0, idx );
	}

	return PyAwesomiumProvider::checkWebCoreImpl( workingDirPath );
}

/**
 *	Actual implementation of checking function which does attempt to load DLL
 */
//static 
bool PyAwesomiumProvider::checkWebCoreImpl( const BW::string& basePath )
{
	BW_GUARD;

	BW::string dllPath = basePath + "\\awesomium.dll";
	BW::string processPath = basePath + "\\awesomium_process.exe";
	BW::string logPath = basePath + "\\awesomium.log";

	// Do a LoadLibrary to force DLL loading from a fixed path without having
	// to set the current working directory.
	s_awesomiumModule_ = LoadLibraryA( dllPath.c_str() );
	if (!s_awesomiumModule_)
	{
		CRITICAL_MSG( "Could not load '%s'\n", dllPath.c_str() );
		return false;
	}

	WebConfig cfg;
	cfg.log_level = kLogLevel_Normal;
	cfg.child_process_path = WSLit( processPath.c_str() );
	cfg.log_path = WSLit( logPath.c_str() );
	cfg.plugin_path = WSLit( basePath.c_str() );
	cfg.package_path = WSLit( basePath.c_str() );

	if (!WebCore::Initialize( cfg ))
	{
		ERROR_MSG( "Awesomium failed to initialise.\n" );
		s_webCoreInitialised_ = false;
	}
	else
	{
		MF_ASSERT( pResourceInterceptor_ == NULL );
		pResourceInterceptor_ = new BWResourceInterceptor();
		WebCore::instance()->set_resource_interceptor( pResourceInterceptor_ );

		INFO_MSG( "Awesomium initialised: version %s.\n",
			WebCore::instance()->version_string() );
		s_webCoreInitialised_ = true;
	}
	
	return s_webCoreInitialised_;
}

/**
 *	Static fini method. Should be called on app shutdown.
 */
void PyAwesomiumProvider::fini()
{
	BW_GUARD;
	if (s_webCoreInitialised_ && 
		WebCore::instance() != NULL)
	{
		// Force cleanup on any outstanding providers.
		for (ProviderSet::iterator it = s_providers_.begin();
			it != s_providers_.end(); ++it)
		{
			(*it)->cleanup();
		}

		WebCore::instance()->set_resource_interceptor( NULL );
		WebCore::Shutdown();
	}
	bw_safe_delete( pResourceInterceptor_ );
	s_webCoreInitialised_ = false;

	if (s_awesomiumModule_)
	{
		FreeLibrary( s_awesomiumModule_ );
		s_awesomiumModule_ = NULL;
	}
}

/**
 *	Static per frame tick for all active Awesomium providers.
 */
void PyAwesomiumProvider::tick()
{
	BW_GUARD;
	if (!s_webCoreInitialised_)
	{
		return;
	}

	WebCore* pCore = WebCore::instance();
	if (pCore)
	{
		pCore->Update();
	}

	for (ProviderSet::iterator it = s_providers_.begin();
		it != s_providers_.end(); ++it)
	{
		(*it)->updateTexture();
	}
}

BW_END_NAMESPACE

#endif // ENABLE_AWESOMIUM

// awesomium_provider.cpp
