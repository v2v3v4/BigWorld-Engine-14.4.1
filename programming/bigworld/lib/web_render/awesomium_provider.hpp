#ifndef AWESOMIUM_PROVIDER_HPP
#define AWESOMIUM_PROVIDER_HPP

#ifdef _WIN64
#define ENABLE_AWESOMIUM	0
#else
#define ENABLE_AWESOMIUM	BW_AWESOMIUM_SUPPORT
#endif

/////
#if ENABLE_AWESOMIUM

#include "romp/py_texture_provider.hpp"

#include "input/input.hpp"
#include "input/py_input.hpp"
#include "cstdmf/bw_set.hpp"

namespace Awesomium {
	class WebView;
}

BW_BEGIN_NAMESPACE

class BWResourceInterceptor;

/*~ class BigWorld.AwesomiumProvider
 *	@components{ client }
 *
 *	Provides a thin wrapper around the Awesomium WebView interface, exposed via
 *	a standard PyTextureProvider interface. It is up to scripts to inject user
 *	input via the injectKeyEvent, injectMouseMove, and injectMouseWheel methods.
 *
 *	See the Awesomium documentation for details about API usage.
 *
 *	The Awesomium DLL and subprocess executable should be located next to the
 *	client executable.
 *
 */
class PyAwesomiumProvider : public PyTextureProvider
{
public:
	Py_Header( PyAwesomiumProvider, PyTextureProvider )

public:
	PyAwesomiumProvider( int width, int height, PyTypePlus * pType = &s_type_ );
	~PyAwesomiumProvider();

	void updateTexture();

	void goBack();
	void goForward();
	void stop();
	void focus();
	void unfocus();
	void loadURL( const BW::string& url );
	void reload( bool ignoreCache );
	void resize( int width, int height );
	void cleanup();

	bool isLoading();
	BW::string url();
	uint32 width();
	uint32 height();
	
	void injectKeyEvent( const KeyEvent& event );
	void injectMouseMoveEvent( int x, int y );
	void injectMouseWheelEvent( int dz );

	void executeJavascript( const BW::string& script, const BW::string& frameXPath );
	BW::string executeJavascriptWithResult( const BW::string& script, const BW::string& frameXPath );

	void script( const PyObjectPtr& object );
	PyObjectPtr script();

	PY_FACTORY_DECLARE()

	PY_RO_ATTRIBUTE_DECLARE( this->isLoading(), isLoading )
	PY_RO_ATTRIBUTE_DECLARE( this->url(), url )
	PY_RO_ATTRIBUTE_DECLARE( this->width(), width )
	PY_RO_ATTRIBUTE_DECLARE( this->height(), height )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyObjectPtr, script, script )

	PY_AUTO_METHOD_DECLARE( RETVOID, updateTexture, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, goBack, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, goForward, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, stop, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, focus, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, unfocus, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, loadURL, ARG( BW::string, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, reload, ARG( bool, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, resize, ARG( int, ARG( int, END ) ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, cleanup, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, injectKeyEvent, ARG( KeyEvent, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, injectMouseWheelEvent, ARG( int, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, injectMouseMoveEvent, ARG( int, ARG( int, END ) ) );

	PY_AUTO_METHOD_DECLARE( RETVOID, executeJavascript, ARG( BW::string, ARG( BW::string, END ) ) );
	PY_AUTO_METHOD_DECLARE( RETDATA, executeJavascriptWithResult, ARG( BW::string, ARG( BW::string, END ) ) );

	virtual Moo::BaseTexturePtr texture();

public:
	static void init();
	static void fini();
	static void tick();

private:
	static bool checkWebCore();
	static bool checkWebCoreImpl( const BW::string& basePath );

	static HMODULE s_awesomiumModule_;
	static bool s_webCoreInitialised_;
	static BWResourceInterceptor* pResourceInterceptor_;

	typedef BW::set<PyAwesomiumProvider*> ProviderSet;
	static ProviderSet s_providers_;

private:
	Awesomium::WebView* pWebView_;

	class Listener;
	Listener* pListener_;
};

BW_END_NAMESPACE

#endif // ENABLE_AWESOMIUM

#endif // AWESOMIUM_PROVIDER_HPP
