#include "pch.hpp"

#include "cstdmf/debug_message_categories.hpp"

#include "network/event_dispatcher.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "urlrequest/manager.hpp"
#include "urlrequest/response.hpp"
#include "urlrequest/string_list.hpp"

BW_BEGIN_NAMESPACE

int PyURLRequest_token = 1;

namespace
{


// -----------------------------------------------------------------------------
// Section: PyResponse
// -----------------------------------------------------------------------------

class PyURLResponse : public PyObjectPlus
{
	Py_Header( PyURLResponse, PyObjectPlus )

public:
	PyURLResponse( const BW::string & contentType, const BW::string & body,
			int responseCode ) :
		PyObjectPlus( &s_type_ ),
		contentType_( contentType ),
		body_( body ),
		responseCode_( responseCode )
	{
		this->incRef();
	}
private:
	BW::string contentType_;
	BW::string body_;
	int responseCode_;

	PY_RO_ATTRIBUTE_DECLARE( contentType_, contentType )
	PY_RO_ATTRIBUTE_DECLARE( body_, body )
	PY_RO_ATTRIBUTE_DECLARE( responseCode_, responseCode )
};

PY_TYPEOBJECT( PyURLResponse )

PY_BEGIN_METHODS( PyURLResponse )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyURLResponse )
	PY_ATTRIBUTE( contentType )
	PY_ATTRIBUTE( body )
	PY_ATTRIBUTE( responseCode )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Response
// -----------------------------------------------------------------------------

/**
 *	This class handles a response from getting a URL.
 */
class Response : public URL::SimpleResponse
{
public:
	Response( PyObjectPtr pCallback );
	virtual ~Response() {}

	virtual bool onContentTypeHeader( const BW::string & contentType );
	virtual void onDone( int responseCode, const char * error );

private:
	PyObjectPtr pCallback_;
	BW::string contentType_;
};


/**
 *	Constructor.
 */
Response::Response( PyObjectPtr pCallback ) :
	pCallback_( pCallback ),
	contentType_()
{
	this->incRef();
}


/*
 *	Override from URL::Response.
 */
bool Response::onContentTypeHeader( const BW::string & contentType )
{
	contentType_ = contentType;
	return true;
}


/**
 *	This method is called when the URL resource has been received.
 */
void Response::onDone( int response, const char * error )
{
	if (error)
	{
		SCRIPT_ERROR_MSG( "Response::onDone: %s\n", error );
	}

	PyObject * pPyResponse = new PyURLResponse( contentType_, body_.str(),
		response );

	PyObject * pResult = PyObject_CallFunctionObjArgs( pCallback_.get(),
				pPyResponse, NULL );

	Py_DECREF( pPyResponse );

	if (!pResult)
	{
		PyErr_Print();
	}
	else
	{
		Py_DECREF( pResult );
	}

	this->decRef();
}


// -----------------------------------------------------------------------------
// Section: Python methods
// -----------------------------------------------------------------------------

/*~	function BigWorld.fetchURL
 *	@components{ base, cell, db, client }
 *	This function fetches a URL asynchronously. The result is returned to the
 *	callback.
 *
 *	Keyword arguments are supported for this function.
 *
 *	@param url	The URL to retrieve.
 *	@param callback	The callable object that receives the response object. The
 *		response object has <i>body</i> and <i>responseCode</i> properties.
 *	@param headers An optional sequence of strings to be added to the request's
 *			HTTP headers.
 *	@param timeout Optional argument for the number of seconds before this call
 *			should time out.
 *	@param method "GET", "PUT", "POST", "PATCH" or "DELETE" to indicate
 *			the HTTP method.
 *	@param postData Optional argument for the data posted along with the
 *			POST/PUT/PATCH request. If it is a Python dict, then this method will
 *			encode the data to MULTIPART/FORM-DATA format. Otherwise, if it is
 *			a Python string, the data will be posted to the HTTP server directly.
 *			In this case, the user of this method needs to make sure that
 *			the data is encoded in the right format(i.e. JSON/XML).
 */
PyObject * py_fetchURL( PyObject * args, PyObject * kwargs )
{
	if (!URL::Manager::instance())
	{
		PyErr_SetString( PyExc_EnvironmentError,
				"PyURLRequest has not been initialised" );
		return NULL;
	}

	char * url = NULL;
	PyObject * pCallback = NULL;
	PyObject * pPyHeaders = NULL;
	PyObject * pPyPostData= NULL;
	float timeout = 0.f;
	char * methodStr = NULL;

	const char * keywords[] = {
		"url",
		"callback",
		"headers",
		"timeout",
		"method",
		"postdata",
		NULL };

	if (!PyArg_ParseTupleAndKeywords( args, kwargs,
				"sO|OfsO", const_cast< char ** >( keywords ),
				&url, &pCallback, &pPyHeaders, &timeout, &methodStr, &pPyPostData ))
	{
		return NULL;
	}

	if (!pCallback || !PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_ValueError, "Callback is not callable" );

		return NULL;
	}

	URL::Headers headers;
	URL::Headers * pHeaders = NULL;

	if (pPyHeaders && (pPyHeaders != Py_None))
	{
		pHeaders = &headers;

		if (!PySequence_Check( pPyHeaders ))
		{
			PyErr_SetString( PyExc_ValueError,
					"headers argument is not a sequence" );
			return NULL;
		}

		Py_ssize_t n = PySequence_Size( pPyHeaders );

		for (int i = 0; i < n; ++i)
		{
			PyObject * pItem = PySequence_GetItem( pPyHeaders, i );

			PyObject * pStr = PyObject_Str( pItem );

			headers.push_back( PyString_AsString( pStr ) );

			Py_DECREF( pStr );
			Py_DECREF( pItem );
		}
	}

	URL::PostData postData;
	URL::PostData * pPostData= NULL;

	if (pPyPostData && (pPyPostData != Py_None))
	{
		pPostData = &postData;

		if (PyString_Check( pPyPostData ))
		{
			postData.encodingType = URL::ENCODING_RAW; 
			postData.rawData = PyString_AsString( pPyPostData );
		}

		else if (PyMapping_Check( pPyPostData ))
		{
			postData.encodingType = URL::ENCODING_MULTIPART_FORM_DATA;
			Py_ssize_t n = PyMapping_Length( pPyPostData );
			PyObject* pPyItems = PyMapping_Items(pPyPostData);

			for (int i = 0; i < n; ++i)
			{
				PyObject * pPyTuple = PySequence_GetItem( pPyItems, i );
				PyObject * pPyKey = PyTuple_GetItem( pPyTuple, 0 );
				PyObject * pPyValue = PyTuple_GetItem( pPyTuple, 1 );
				PyObject * pPyStrKey = PyObject_Str( pPyKey );
				PyObject * pPyStrValue = PyObject_Str( pPyValue );
					
				char * key = PyString_AsString( pPyStrKey );
				char * value = PyString_AsString( pPyStrValue );
				postData.formData[ key ] = value;

				Py_DECREF( pPyStrValue );
				Py_DECREF( pPyStrKey );
				Py_DECREF( pPyValue );
				Py_DECREF( pPyKey );
				Py_DECREF( pPyTuple );
			}
		}
		else
		{

			PyErr_SetString( PyExc_ValueError,
					"postData argument should be a dict or string" );
			return NULL;
		}
	}


	URL::Method method = URL::METHOD_GET;

	if (methodStr)
	{
		if (strcmp( methodStr, "GET" ) == 0)
		{
			method = URL::METHOD_GET;
		}
		else if (strcmp( methodStr, "PUT" ) == 0)
		{
			method = URL::METHOD_PUT;
		}
		else if (strcmp( methodStr, "POST" ) == 0)
		{
			method = URL::METHOD_POST;
		}
		else if (strcmp( methodStr, "DELETE" ) == 0)
		{
			method = URL::METHOD_DELETE;
		}
		else if (strcmp( methodStr, "PATCH" ) == 0)
		{
			method = URL::METHOD_PATCH;
		}
		else
		{
			PyErr_Format( PyExc_ValueError,
					"Invalid method %s. Should be 'GET', 'PUT', 'POST', 'DELETE' or 'PATCH''", methodStr );
			return NULL;
		}
	}

	URL::Manager::instance()->fetchURL( url, new Response( pCallback ),
			pHeaders, method, timeout, pPostData );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION_WITH_KEYWORDS_WITH_DOC( fetchURL, BigWorld,
		"This method asynchronously fetches a URL" )
} // anonymous namespace


BW_END_NAMESPACE

// py_urlrequest.cpp
