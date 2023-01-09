
#include "pch.hpp"
#include "post_proc_property_editor.hpp"
#include "base_post_processing_node.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	This local function splits "instr" into words separated by "separator"
	 *	and puts each word as an element of the return vector "retTokens".
	 *
	 *	@param instr	String with a list of items separated by "separator".
	 *	@param separator	Separator character.
	 *	@param retTokens	Return param, vector with the items.
	 */
	void split( const BW::string & instr, const BW::string & separator, 
				BW::vector< BW::string > & retTokens )
	{
		BW_GUARD;

		BW::string::size_type start = 0;
		BW::string::size_type end = 0;

		while ((end = instr.find( separator, start )) != BW::wstring::npos)
		{
			retTokens.push_back( instr.substr( start, end - start ) );
			start = end + separator.length();
		}

		retTokens.push_back( instr.substr( start ) );
	}


	/**
	 *	This local function retrieves the list of filename extensions found in
	 *	a GeneralProperty file spec string.
	 *
	 *	@param filespec		GeneralProperty file spec string.
	 *	@param retExtensions	List of filename extensions found in filespec.
	 */
	void filespecExtensions( const BW::string & filespec, 
							BW::vector< BW::string > & retExtensions )
	{
		BW_GUARD;

		if (filespec.empty())
		{
			return;
		}

		BW::vector< BW::string > specSections;
		split( filespec, "|", specSections );

		if (specSections.size() >= 2)
		{
			BW::vector< BW::string > exts;
			split( specSections[ 1 ], ";", exts );

			for (BW::vector< BW::string >::iterator it = exts.begin();
				it != exts.end(); ++it)
			{
				BW::string::size_type dotPos = (*it).rfind( '.' );
				if (dotPos != BW::wstring::npos)
				{
					retExtensions.push_back( (*it).substr( dotPos + 1 ) );
				}
			}
		}
	}


	/**
	 *	This local function returns whether or not "filespec" is a texture.
	 *
	 *	@param filespec		GeneralProperty file spec string.
	 *	@return Whether or not "filespec" is a texture.
	 */
	bool isTexture( const BW::string & filespec )
	{
		BW_GUARD;

		bool ret = false;

		BW::vector< BW::string > exts;
		filespecExtensions( filespec, exts );

		for (BW::vector< BW::string >::iterator it = exts.begin();
			it != exts.end(); ++it)
		{
			if ((*it) == "tga" ||
				(*it) == "bmp" ||
				(*it) == "dds" ||
				(*it) == "jpg" ||
				(*it) == "png" ||
				(*it) == "texanim")
			{
				ret = true;
				break;
			}
		}

		return ret;
	}


	/**
	 *	This local function returns whether or not "filespec" is a render
	 *	target.
	 *
	 *	@param filespec		GeneralProperty file spec string.
	 *	@return Whether or not "filespec" is a render target.
	 */
	bool isRenderTarget( const BW::string & filespec )
	{
		BW_GUARD;

		bool ret = false;

		BW::vector< BW::string > exts;
		filespecExtensions( filespec, exts );

		for (BW::vector< BW::string >::iterator it = exts.begin();
			it != exts.end(); ++it)
		{
			if ((*it) == "rt")
			{
				ret = true;
				break;
			}
		}

		return ret;
	}


	/**
	 *	This local function returns whether or not "filespec" is a shader file.
	 *
	 *	@param filespec		GeneralProperty file spec string.
	 *	@return Whether or not "filespec" is a shader file.
	 */
	bool isShader( const BW::string & filespec )
	{
		BW_GUARD;

		bool ret = false;

		BW::vector< BW::string > exts;
		filespecExtensions( filespec, exts );

		for (BW::vector< BW::string >::iterator it = exts.begin();
			it != exts.end(); ++it)
		{
			if ((*it) == "fx")
			{
				ret = true;
				break;
			}
		}

		return ret;
	}

} // anonymous namespace


/**
 *	Constructor.
 *
 *	@param node	Post processing node this editor will be editing.
 */
PostProcPropertyEditor::PostProcPropertyEditor( BasePostProcessingNodePtr node ) :
	node_( node )
{
	BW_GUARD;

	node->edEdit( this );

	constructorOver_ = true;
}


/**
 *	This method returns the node's properties of a type in "types".
 *
 *	@param types	A combination of TEXTURES, RENDER_TARGETS and/or SHADERS.
 *	@param retProps	Return param, properties found of a type in "types".
 */
void PostProcPropertyEditor::getProperties( int types, BW::vector< BW::string > & retProps ) const
{
	BW_GUARD;

	for (PropList::const_iterator it = properties_.begin(); it != properties_.end(); ++it)
	{
		if (((types & TEXTURES) && isTexture( (*it)->fileFilter().c_str() )) ||
			((types & RENDER_TARGETS) && isRenderTarget( (*it)->fileFilter().c_str() )) ||
			((types & SHADERS) && isShader( (*it)->fileFilter().c_str() )))
		{
			retProps.push_back( (*it)->name().c_str() );
		}
	}
}


/**
 *	This method sets the value of a node's property.
 *
 *	@param name		Name of the property to change.
 *	@param value	New value for the property.
 */
void PostProcPropertyEditor::setProperty( const BW::string & name, const BW::string & value )
{
	BW_GUARD;

	for (PropList::iterator it = properties_.begin(); it != properties_.end(); ++it)
	{
		if ((*it)->name() == name)
		{
			PyObject * pyVal = NULL;
			if ((*it)->valueType().fromString( value, pyVal ))
			{
				(*it)->pySet( pyVal );
				Py_DECREF( pyVal );
			}
			else
			{
				ERROR_MSG( "Error changing post processing property '%s'\n",
							(*it)->name() );
			}
			break;
		}
	}
}

BW_END_NAMESPACE

