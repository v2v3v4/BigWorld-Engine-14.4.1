/*
 * A class that handles the processing of a file as a DataResource.
 */

#include "pch.hpp"

// Standard MF Library Headers.
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include <iostream>
#include <fstream>

// Application Specific Headers.
#include "dataresource.hpp"
#include "xml_section.hpp"
#include "cstdmf/debug.hpp"

#ifdef _WIN32
	#include <direct.h>
#else
	#include <dirent.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif


DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: XMLHandle declaration.
// -----------------------------------------------------------------------------

/**
 *	A class that handles the opening and parsing of an XML file.
 *	It can be treated as a factory that reads in a single XML file and produces
 *	handles to accessing the various section of the XML file parsed.
 */
class XMLHandle : public DataHandle
{
public:
	///	@name Constructor(s).
	//@{
	///	Constructor for the XMLHandle class.
	XMLHandle( const BW::string &fileName, bool empty = false );
	//@}

	///	@name Methods associated with XML elements and sections.
	//@{
	/// This method returns a pointer to a DataSection representing the top of
	///	the parse tree.
	DataSectionPtr getRootSection();

	/// This method will reload the resource from the path specified.
	DataHandle::Error reload();

	/// This method will save the resource to the path specified.
	DataHandle::Error save( const BW::string &fileName );
	//@}

	/// This method returns whether or not this handle is in a valid state.
	bool isGood() const				{ return pRootSection_.hasObject(); }

	///	@name Destructor.
	//@{
	///	The destructor for the XMLHandle class.
	~XMLHandle();
	//@}

private:
	XMLSectionPtr pRootSection_;
};


// -----------------------------------------------------------------------------
// Section: Constructor(s) for DataHandle.
// -----------------------------------------------------------------------------

/**
 *	This constructor simply initialises the error state variables.
 *
 *	@return None.
 */
DataHandle::DataHandle( const BW::string &fileName )
{
	lastError_			= DataHandle::DHE_NoError;
	lastErrorMessage_	= "*** No Error.\n";
	fileName_ = fileName;
}

// -----------------------------------------------------------------------------
// Section: Interface Methods to the DataResource for DataHandle.
// -----------------------------------------------------------------------------

/**
 *	This method returns the file-name.
 *
 *	@return A const string containing the file name of the resource.
 */
const BW::string &DataHandle::fileName()
{
	return fileName_;
}

// -----------------------------------------------------------------------------
// Section: Methods associated with error handling and debugging for DataHandle.
// -----------------------------------------------------------------------------

/**
 *	This method is used to quietly obtain some user-meaningful text describing
 *	the error that has occurred.
 *
 *	@return A string containing the error message.
 */
const BW::string &DataHandle::errorMessage()
{
	// The default dummy no-error message.
	static BW::string noError = "*** No error.\n";

	// Only return the message if there was an error.
	if (lastError_ == DataHandle::DHE_NoError)
	{
		return noError;
	}
	else
	{
		return lastErrorMessage_;
	}
}

/**
 *	This method is used to determine if the last operation resulted in success
 *	or a failure. The actual return value signifies the error that occurred.
 *
 *	@return The error code.
 */
inline DataHandle::Error DataHandle::errorState()
{
	return lastError_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for DataResource.
// -----------------------------------------------------------------------------

/**
 *	The default constructor produces a DataResource on stand-by. The resource
 *	on stand-by still needs to be loaded using the load() method.
 */
DataResource::DataResource() : pResource_( (DataHandle *)NULL )
{
}

/**
 *	This constructor creates a DataResource based on the extension of the
 *	filename supplied.
 *
 *	@param	fileName	The full path and name of the file to be opened as
 *						a resource.
 */
DataResource::DataResource( const BW::string &fileName )
{
	load( fileName );
}

/**
 *	This constructor forces the file to be treated as an XML file, regardless
 *	of the filename extension. It also allows the user to specify exact XML
 *	settings for the DOM API.
 *
 *	@param fileName		The full path and name of the file to be opened as
 *						a resource.
 *	@param type			Indicates the type of the data resource.
 *	@param createIfInvalid	Should the resource be created if the provided
 *							filename appears to be an invalid section.
 */
DataResource::DataResource( const BW::string &fileName,
		DataResourceType type, bool createIfInvalid )
{
	switch ( type )
	{
		case RESOURCE_TYPE_XML:
		{
			this->ensureFileExists( fileName );

			XMLHandle * pHandle = new XMLHandle( fileName );

			if (pHandle->isGood())
			{
				pResource_ = pHandle;
			}
			else
			{
				bw_safe_delete( pHandle );

				if (createIfInvalid)
				{
					pHandle = new XMLHandle( fileName, true );
					if (pHandle->isGood())
					{
						pResource_ = pHandle;
					}
					else
					{
						WARNING_MSG( "DataResource::DataResource: "
								"Failed to create blank file %s\n",
							fileName.c_str() );
						bw_safe_delete( pHandle );
					}
				}
				else
				{
					WARNING_MSG( "DataResource::DataResource: "
							"Loading %s failed\n",
						fileName.c_str() );
				}
			}
		}
		break;

		default:
			ERROR_MSG( "DataResource::DataResource: "
						"Unknown resource type %d\n",
					int( type ) );
		break;
	}
}


// -----------------------------------------------------------------------------
// Section: Interface Methods to the DataResource.
// -----------------------------------------------------------------------------

/**
 *	This method returns a pointer to a DataSection representing the top of
 *	the parse tree for the resource.
 *
 *	@return A pointer to the root node of the resource. NULL if there was
 *			a problem.
 */
DataSectionPtr DataResource::getRootSection()
{
	if (pResource_)
	{
		return pResource_->getRootSection().getObject();
	}
	else
	{
		return (DataSection *)NULL;
	}
}


/**
 *	This method will load the resource from the path specified. If already
 *	opened, it does nothing.
 *
 *	@param	fileName	The full path and name of the file to be opened as
 *						a resource.
 *	@param type			The type of the data resource to load.
 *
 *	@return DataHandle::DHE_NoError if there was no problem, otherwise it
 *			returns the DataHandle::Error code.
 */
DataHandle::Error DataResource::load( const BW::string &fileName,
                                      DataResourceType type,
									  bool createIfInvalid )
{
	TRACE_MSG( "DataResource::load: %s\n", fileName.c_str() );

    DataHandle::Error result = DataHandle::DHE_LoadFailed;

	// If not already loaded.
	if ( !pResource_ )
	{
        switch ( type )
        {
            case RESOURCE_TYPE_XML:
            {
				XMLHandle * pHandle = new XMLHandle( fileName );

				if (pHandle->isGood())
				{
					pResource_ = pHandle;

                    result = DataHandle::DHE_NoError;
				}
				else
				{
					bw_safe_delete(pHandle);

					if (createIfInvalid)
					{
						pHandle = new XMLHandle( fileName, true );
						if (pHandle->isGood())
						{
							pResource_ = pHandle;

							result = DataHandle::DHE_NoError;
						}
						else
						{
							WARNING_MSG( "DataResource::DataResource: "
								"Failed to create blank file %s\n",
								fileName.c_str() );
							bw_safe_delete( pHandle );
						}
					}
					else
					{
						WARNING_MSG( "DataResource::DataResource: "
							"Loading %s failed\n",
							fileName.c_str() );
					}
				}

                break;
			}

            default:
                // It is not a file type we're aware of.
    			pResource_ = (DataHandle *)NULL;
                break;
        }
	}

    return result;
}

/**
 *	This method will reload the resource from the path specified.
 *
 *	@return DataHandle::DHE_NoError if there was no problem, otherwise it
 *			returns the DataHandle::Error code.
 */
DataHandle::Error DataResource::reload()
{
	if ( pResource_ )
	{
		return pResource_->reload();
	}
	else
	{
		return DataHandle::DHE_LoadFailed;
	}
}

/**
 *	This method will save the resource to the path specified.
 *
 *	@param	fileName	The path and name of the file to be saved to.
 *
 *	@return DataHandle::DHE_NoError if there was no problem, otherwise it
 *			returns the DataHandle::Error code.
 */
DataHandle::Error DataResource::save( const BW::string &fileName )
{
	if ( pResource_ )
	{
        //Ensure the file exists
        //ensureFileExists( fileName );

        //Now save the file
		return pResource_->save( fileName );
	}
	else
	{
		return DataHandle::DHE_SaveFailed;
	}
}

/**
 *	This method returns the file-name.
 *
 *	@return A const string containing the file name of the resource.
 */
const BW::string &DataResource::fileName()
{
	static const BW::string emptyFileName = "";
	if ( pResource_ )
	{
		return pResource_->fileName();
	}
	else
	{
		return emptyFileName;
	}
}

/**
 *	This method returns true if the DataResource has been loaded.
 *
 *	@return True, if the DataResource has been loaded successfully.
 *			False, otherwise.
 */
bool DataResource::loaded()
{
	return (bool) pResource_;
}

// -----------------------------------------------------------------------------
// Section: Methods associated with error handling and debugging of DataResource
// -----------------------------------------------------------------------------

/**
 *	This method is used to quietly obtain some user-meaningful text
 *	describing the error that has occurred.
 *
 *	@return A string containing the error message.
 */
const BW::string &DataResource::errorMessage()
{
	static BW::string nullMsg =
		"*** Fatal Error: Could not open resource file.\n";

	if ( pResource_ )
	{
		return pResource_->errorMessage();
	}
	else
	{
		return nullMsg;
	}
}


/**
 *	This method is used to determine if the last operation resulted in success
 *	or a failure. The actual return value signifies the error that occurred.
 *
 *	@return The error code.
 */
DataHandle::Error DataResource::errorState()
{
	if ( pResource_ )
	{
		return pResource_->errorState();
	}
	else
	{
		return DataHandle::DHE_LoadFailed;
	}
}

/**
 * This method ensures that the specified file exists.
 * It will create paths, and a blank file if necessary.
 */
void DataResource::ensureFileExists( const BW::string& fileName )
{
    // does the file exist
    if ( !BWResource::fileExists( fileName ) )
    {
	    BW::string subDir;

	    for ( int i = 1; i < (int)fileName.size(); i++ )
    	{
            if ( fileName[i] == '\\' || fileName[i] == '/' )
            {
				BWResource::instance().fileSystem()->makeDirectory( fileName.substr( 0, i ) );
            }
        }

        // make a blank xml file
        FILE* fh = BWResource::instance().fileSystem()->posixFileOpen( fileName, "w" );

        if ( fh )
        {
            fprintf( fh, "<root>\n</root>\n" );
            fclose( fh );
        }
    }
}

// -----------------------------------------------------------------------------
// Section: Constructor(s) for XMLHandle.
// -----------------------------------------------------------------------------

/**
 *	Constructor for the XMLHandle class. It requires an XML filename to be
 *	supplied. The work done is handled by the loadDocument() method.
 *
 *	@param fileName		Full path and name of the XML file.
 *	@param isEmpty		Is this an empty file.
 */
XMLHandle::XMLHandle( const BW::string &fileName, bool isEmpty ) :
	DataHandle( fileName ),
	pRootSection_( (XMLSection *)NULL )
{
	if (isEmpty)
	{
		pRootSection_ = new XMLSection( "root" );
	}
	else
	{
		this->reload();
	}
}

// -----------------------------------------------------------------------------
// Section: Methods associated with XML elements and sections.
// -----------------------------------------------------------------------------

/**
 *	This method returns a pointer to an XMLSection representing the top of the
 *	parse tree. The root section has the prolog, epilog and the Document
 *	Element as sibling.
 *	Accessing the prolog and epilog of an XML file is done with this method.
 *
 *	@return A pointer to the newly allocated DataSection. A NULL value is
 *			returned if there was an error in the parsing or opening of the
 *			file.
 */
DataSectionPtr XMLHandle::getRootSection()
{
	return pRootSection_.getObject();
}


/**
 *	This method will reload the resource from the path specified. Note: The
 *	use of this method may invalidate any XMLSectionPtr and XMLSections
 *	pointing to the XMLResource still in use.
 *
 *	@return DataHandle::DHE_NoError if there was no problem, otherwise it
 *			returns the DataHandle::Error code.
 */
DataHandle::Error XMLHandle::reload()
{
	pRootSection_ = XMLSection::createFromFile( fileName_.c_str() );

	return (pRootSection_.hasObject()) ?
		DataHandle::DHE_NoError : DataHandle::DHE_LoadFailed;
}


/**
 *	This method will save the XML resource to the path specified.
 *
 *	@param	fileName	The path and name of the file to be saved to.
 *
 *	@return DataHandle::DHE_NoError if there was no problem, otherwise it
 *			returns the DataHandle::Error code.
 */
DataHandle::Error XMLHandle::save( const BW::string &fileName )
{
    bool useThisFileName = ( fileName.compare( "" ) == 0 );

#ifdef WIN32
	std::ofstream stream( useThisFileName ? bw_utf8tow( fileName_ ).c_str() : bw_utf8tow( fileName ).c_str() );
#else
	std::ofstream stream( useThisFileName ? fileName_.c_str() : fileName.c_str() );
#endif

	bool wasSuccessful = stream.good();

	if (pRootSection_ && wasSuccessful)
	{
		wasSuccessful = pRootSection_->writeToStream( stream );
	}

	return wasSuccessful ? DataHandle::DHE_NoError : DataHandle::DHE_SaveFailed;
}

// -----------------------------------------------------------------------------
// Section: Destructor for XMLHandle.
// -----------------------------------------------------------------------------

/**
 *	The destructor for the XMLHandle class. There is an instance count of
 *	the XMLHandle class. If this count drops to zero, the XML platform
 *	utilities are uninitialised.
 */
XMLHandle::~XMLHandle()
{
	// pRootSection_ is a smart pointer and will be removed automatically.
}

BW_END_NAMESPACE

// dataresource.cpp
