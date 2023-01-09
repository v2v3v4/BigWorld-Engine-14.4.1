#include "pch.hpp"
#include "res_mgr_script.hpp"

#include "py_data_section.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

#include "cstdmf/bw_util.hpp"


#ifdef EDITOR_ENABLED
#include "resmgr/multi_file_system.hpp"
#endif // EDITOR_ENABLED


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "res_mgr_script.ipp"
#endif

/*~ module ResMgr
 *	@components{ all }
 */

// -----------------------------------------------------------------------------
// Section: Method definitions
// -----------------------------------------------------------------------------

/*~ function ResMgr.isDir
 *	@components{ all }
 *
 *	This function returns true if the specified path name is a directory,
 *	false otherwise.
 *
 *	@param	pathname	The path name to check.
 *
 *	@return				True (1) if it is a directory, false (0) otherwise.
 */
/**
 *	This function implements a script function. It checks if a path name
 *	is a directory.
 */
static int isDir( const BW::string & pathName )
{
	return BWResource::isDir( pathName );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isDir,	ARG( BW::string, END ), ResMgr )

/*~ function ResMgr.isFile
 *	@components{ all }
 *
 *	This function returns true if the specified path name is a file,
 *	false otherwise.
 *
 *	@param	pathname	The path name to check.
 *
 *	@return				True (1) if it is a file, false (0) otherwise.
 */
/**
 *	This function implements a script function. It checks if a path name
 *	is a file.
 */
static int isFile( const BW::string & pathName )
{
	return BWResource::isFile( pathName );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isFile,	ARG( BW::string, END ), ResMgr )

/*~ function ResMgr.openSection
 *	@components{ all }
 *
 *	This function opens the specified resource as a DataSection.  If the
 *	resource is not found, then it returns None. A new section can optionally
 *	be created by specifying true in the optional second argument.
 *
 *	Resources live in a res tree and include directories, xml files, xml nodes,
 *	normal data files, binary section data file nodes, etc.
 *
 *	@param	resourceID	the id of the resource to open.
 *	@param	newSection	Boolean value indicating whether to create this
 *						as a new section, default is False.
 *
 *	@return				the DataSection that was loaded, or None if the
 *						id was not found.
 */
/**
 *	This function implements a script function. It converts a data section into
 *	a hierarchy of Python maps (aka a DataSection)
 */
static PyObject * openSection( const BW::string & resourceID,
	bool makeNewSection = false )
{
	DataSectionPtr pSection = BWResource::openSection(
		resourceID, makeNewSection );

	if (!pSection)
	{
		if (!makeNewSection)
		{
			Py_RETURN_NONE;	// just return None if there is no section
		}
		else
		{
			PyErr_Format( PyExc_ValueError, "ResMgr.openSection(): "
				"Could not make new section '%s'", resourceID.c_str() );
			return NULL;
		}
	}

	return new PyDataSection( pSection );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, openSection,
	ARG( BW::string, OPTARG( bool, false, END ) ), ResMgr )

/*~ function ResMgr.save
 *	@components{ all }
 *
 *	This function saves the previously loaded section with the specified
 *	path.  If no section with that id is still in memory, then an IO error
 *	occurs, otherwise, the section is saved.
 *
 *	@param	resourceID	the filepath of the DataSection to save.
 *
 *	@return				True if successful, False otherwise.
 */
/**
 *	This function implements a script function. It saves this section at the
 *	input path.
 */
static bool save( const BW::string & resourceID )
{
	if (BWResource::instance().save( resourceID )) return true;

	PyErr_Format( PyExc_IOError, "Save of %s failed", resourceID.c_str() );
	return false;
}
PY_AUTO_MODULE_FUNCTION( RETOK, save, ARG( BW::string, END ), ResMgr )


/*~ function ResMgr.purge
 *	@components{ all }
 *
 *	This function purges the previously loaded section with the specified
 *	path from the cache and census. Optionally, all child sections can also
 *	be purged (only useful if the resource is a DirSection), by specifying
 *	true in the optional second argument.
 *
 *	@param	resourceID	the id of the resource to purge.
 *	@param	recurse		Boolean value indicating whether to recursively
 *						purge any subsections. default is False.
 */
/**
 *	This function implements a script function. It purges the given section
 *	from the cache, enabling the copy on disk to be read in.
 */
static void purge( const BW::string & resourceID, bool recurse = false )
{
	BWResource::instance().purge( resourceID, recurse );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, purge,
	ARG( BW::string, OPTARG( bool, false, END ) ), ResMgr )


#ifdef EDITOR_ENABLED


/*~ function ResMgr.remove
 *	@components{ tools }
 *
 *	This function deletes a file.
 *
 *	@param	resID	the filepath of the file to delete.
 *
 *	@return				True if successful, False otherwise.
 */
/**
 *	This function implements a script function. It deletes this file.
 */
static bool remove( const BW::string & resID )
{
	if (BWResource::instance().fileSystem()->eraseFileOrDirectory( resID ))
	{
		return true;
	}

	PyErr_Format( PyExc_IOError, "Removing of %s failed", resID.c_str() );
	return false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, remove, ARG( BW::string, END ), ResMgr )


/*~ function ResMgr.rename
 *	@components{ tools }
 *
 *	This function renames a file.
 *
 *	@param	oldResID	the old filepath.
 *	@param	newResID	the new filepath.
 *
 *	@return				True if successful, False otherwise.
 */
/**
 *	This function implements a script function. It renames this file.
 */
static bool rename( const BW::string & oldResID,
												const BW::string & newResID )
{
	if (BWResource::instance().fileSystem()->moveFileOrDirectory( oldResID,
																	newResID ))
	{
		return true;
	}

	PyErr_Format( PyExc_IOError, "Renaming of %s to %s failed",
										oldResID.c_str(), newResID.c_str() );
	return false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, rename,
						ARG( BW::string, ARG( BW::string, END ) ), ResMgr )


/*~ function ResMgr.copy
 *	@components{ tools }
 *
 *	This function copies a file.
 *
 *	@param	srcResID	the source filepath.
 *	@param	dstResID	the dest filepath.
 *
 *	@return				True if successful, False otherwise.
 */
/**
 *	This function implements a script function. It copies this file.
 */
static bool copy( const BW::string & srcResID,
												const BW::string & dstResID )
{
	BinaryPtr fileData =
					BWResource::instance().fileSystem()->readFile( srcResID );

	if (fileData)
	{
		if (BWResource::instance().fileSystem()->writeFile(
													dstResID, fileData, true ))
		{
			return true;
		}
	}

	PyErr_Format( PyExc_IOError, "Copying of %s to %s failed",
										srcResID.c_str(), dstResID.c_str() );
	return false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, copy,
						ARG( BW::string, ARG( BW::string, END ) ), ResMgr )


#endif // EDITOR_ENABLED

/*~ function ResMgr.resolveToAbsolutePath
 *	@components{ all }
 *
 *	This function resolves a path relative to the res tree to an absolute path.
 *	If the file does not exist in any of the resource paths, then it will be
 *	resolved against the first resource path.
 *
 *	@return The absolute path
 */
static PyObject * resolveToAbsolutePath( const BW::string & path )
{
	BW::string resolvedPath( path );
	if (BWResource::resolveToAbsolutePath( resolvedPath ) ==
			IFileSystem::FT_NOT_FOUND)
	{
		if (BWUtil::isAbsolutePath( path ))
		{
			resolvedPath = path;
		}
		else
		{
			resolvedPath = BWResource::getPath( 0 ) + "/" + path;
		}
	}

	return Script::getData( BWUtil::normalisePath( resolvedPath ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, resolveToAbsolutePath,
		ARG( BW::string, END ), ResMgr )


/*~ function ResMgr.localise
 *	@components{ all }
 *
 *	This function localises a string and expand its params
 *
 *	@param	string		The string to localise
 *	@param	params		The params used to expand
 *
 *	@return				localised and expanded string
 */
/**
 *	This function implements a script function. It checks if a path name
 *	is a directory.
 */
static PyObject* py_localise( PyObject* args )
{
	const char* str = "";
	const char* s[8] = { "", "", "", "", "", "", "", "" };

	if (!PyArg_ParseTuple( args, "s|ssssssss", &str,
		&s[0] ,&s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7] ))
	{
		return NULL;
	}

	return Script::getData( LocaliseUTF8(
		bw_utf8tow( str ).c_str(),
		bw_utf8tow( s[0] ).c_str(),
		bw_utf8tow( s[1] ).c_str(),
		bw_utf8tow( s[2] ).c_str(),
		bw_utf8tow( s[3] ).c_str(),
		bw_utf8tow( s[4] ).c_str(),
		bw_utf8tow( s[5] ).c_str(),
		bw_utf8tow( s[6] ).c_str(),
		bw_utf8tow( s[7] ).c_str() ) );
}
PY_MODULE_FUNCTION( localise, ResMgr )


// -----------------------------------------------------------------------------
// Section: Initialisation
// -----------------------------------------------------------------------------

#if !defined( BW_BLOB_CONFIG )
extern int PyDataSection_token;
extern int ResourceTable_token;
int ResMgr_token = PyDataSection_token | ResourceTable_token;
#else
int ResMgr_token = true;
#endif // !(defined( BW_BLOB_CONFIG )

/*~ attribute ResMgr.root
 *	@components{ all }
 *
 *	This is the root data section of the resource tree.
 *
 *	@type	DataSection
 */
/**
 *	This init time job adds the root attribute to the ResMgr module
 */
PY_MODULE_ATTRIBUTE( ResMgr, root,
	new PyDataSection( BWResource::instance().rootSection() ) );

BW_END_NAMESPACE

// res_mgr_script.cpp
