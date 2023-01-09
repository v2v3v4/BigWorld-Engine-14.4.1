#include "pch.hpp"

#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/primitive_file.hpp"
#include "moo/primitive_file_structs.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "me_error_macros.hpp"

#include "mutant.hpp"

BW_BEGIN_NAMESPACE

/**
 * This is a helper function which strip two strings down to their differing roots, e.g.
 *   strA = "sets/test/images"  strB = "sets/new_test/images"
 * becomes
 *   strA = "sets/test"         strB = "sets/new_test"

 *  @param	strA			The first string
 *	@param	strB			The second string
 **/
void Mutant::clipToDiffRoot( BW::string& strA, BW::string& strB )
{
	BW_GUARD;

	BW::string::size_type posA = strA.find_last_of("/");
	BW::string::size_type posB = strB.find_last_of("/");

	while ((posA != BW::string::npos) &&
		(posB != BW::string::npos) &&
		( strA.substr( posA ) == strB.substr( posB )))
	{
		strA = strA.substr( 0, posA );
		strB = strB.substr( 0, posB );

		posA = strA.find_last_of("/");
		posB = strB.find_last_of("/");
	}
}

/**
 * This method checks whether a texture could be an animated texture and fixes the animtex if needed.
 *
 *  @param	texRoot			The texture to test
 *	@param	oldRoot			The old root to replace
 *	@param	newRoot			The new root to replace with
 */
void Mutant::fixTexAnim( const BW::string& texRoot, const BW::string& oldRoot, const BW::string& newRoot )
{
	BW_GUARD;

	BW::string texAnimName = texRoot.substr( 0, texRoot.find_last_of(".") ) + ".texanim";

	DataSectionPtr texAnimFile =  BWResource::openSection( texAnimName );
	if (texAnimFile != NULL)
	{
		bool changed = false;

		BW::vector< DataSectionPtr > textures;
		texAnimFile->openSections( "texture", textures );
		for (unsigned i=0; i<textures.size(); i++)
		{
			BW::string textureName = textures[i]->asString("");
			if (textureName != "")
			{
				if (BWResource::fileExists( textureName )) continue;
				BW::string::size_type pos = textureName.find( oldRoot );
				if (pos != BW::string::npos)
				{
					textureName = textureName.replace( pos, oldRoot.length(), newRoot );
					textures[i]->setString( textureName );
					changed = true;
				}
			}
		}
		if (changed)
		{
			texAnimFile->save();
		}
	}
}

/**
 * This method fixes the texture paths of a <material> data section if needed.
 *
 *  @param	mat				The <material> data section to fix
 *	@param	oldRoot			The old root to replace
 *	@param	newRoot			The new root to replace with
 *
 *	@return Whether any texture paths were changed.
 */
bool Mutant::fixTextures( DataSectionPtr mat, const BW::string& oldRoot, const BW::string& newRoot )
{
	BW_GUARD;

	bool changed = false;
	
	BW::string newMfm = mat->readString("mfm","");
	if (newMfm != "")
	{
		if (!BWResource::fileExists( newMfm ))
		{
			BW::string::size_type pos = newMfm.find( oldRoot );
			if (pos != BW::string::npos)
			{
				newMfm = newMfm.replace( pos, oldRoot.length(), newRoot );
				mat->writeString( "mfm", newMfm );
				changed = true;
			}
		}
	}
	
	instantiateMFM( mat );
						
	BW::vector< DataSectionPtr > effects;
	mat->openSections( "fx", effects );
	for (unsigned i=0; i<effects.size(); i++)
	{
		BW::string newFx = effects[i]->asString("");
		if (newFx != "")
		{
			if (BWResource::fileExists( newFx )) continue;
			BW::string::size_type pos = newFx.find( oldRoot );
			if (pos != BW::string::npos)
			{
				newFx = newFx.replace( pos, oldRoot.length(), newRoot );
				effects[i]->setString( newFx );
				changed = true;
			}
		}
	}

	BW::vector< DataSectionPtr > props;
	mat->openSections( "property", props );
	for (unsigned i=0; i<props.size(); i++)
	{
		BW::string newTexture = props[i]->readString( "Texture", "" );
		if (newTexture != "")
		{
			if (BWResource::fileExists( newTexture )) continue;
			BW::string::size_type pos = newTexture.find( oldRoot );
			if (pos != BW::string::npos)
			{
				newTexture = newTexture.replace( pos, oldRoot.length(), newRoot );
				props[i]->writeString( "Texture", newTexture );
				fixTexAnim( newTexture, oldRoot, newRoot );
				changed = true;
			}
		}
		else // Make sure to handle texture feeds as well
		{
			newTexture = props[i]->readString( "TextureFeed/default", "" );
			if (newTexture != "")
			{
				if (BWResource::fileExists( newTexture )) continue;
				BW::string::size_type pos = newTexture.find( oldRoot );
				if (pos != BW::string::npos)
				{
					newTexture = newTexture.replace( pos, oldRoot.length(), newRoot );
					props[i]->writeString( "TextureFeed/default", newTexture );
					fixTexAnim( newTexture, oldRoot, newRoot );
					changed = true;
				}
			}
		}
	}

	return changed;
}

/*static*/ BW::vector< BW::string > Mutant::s_missingFiles_;
/**
 * Static method to clear the list of files which couldn't be found
 */
/*static*/ void Mutant::clearFilesMissingList()
{
	BW_GUARD;

	s_missingFiles_.clear();
}

/**
 * This method tries to locate the file requested.
 *
 *  @param	fileName		Path of the missing file.
 *  @param	modelName		Path of the model trying to be opened.
 *  @param	ext				The extension of the file type to find (e.g. "model", "visual", etc)
 *  @param	what			What operation is being exectuted (e.g. "load", "add", etc)
 *  @param	criticalMsg		A string for whether the model can load without locating the missing file

 *	@param	oldRoot			The old root to replace
 *	@param	newRoot			The new root to replace with
 *
 *	@return Whether the file was located.
 */
bool Mutant::locateFile( BW::string& fileName, BW::string modelName, const BW::string& ext, const BW::string& what, const BW::string& criticalMsg /* = "" */ )
{
	BW_GUARD;

	BW::vector< BW::string >::iterator entry = std::find( s_missingFiles_.begin(), s_missingFiles_.end(), fileName );

	if (entry != s_missingFiles_.end()) return false;
	
	s_missingFiles_.push_back( fileName );
		
	int response = ::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
		Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/CANT_LOCATE_Q", ext, fileName, ext, criticalMsg ), 
		Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOCATE", ext), MB_ICONERROR | MB_YESNO );
	
	if (response == IDYES)
	{
		BW::string oldFileName = fileName.substr( fileName.find_last_of( "/" ) + 1 ) + "." + ext;

		BW::string fileFilter = ext +" (*."+ext+")|*."+ext+"||";
		BWFileDialog fileDlg (
			true, L"", bw_utf8tow( oldFileName ).c_str(),
			BWFileDialog::FD_WRITABLE_AND_EXISTS, bw_utf8tow( fileFilter ).c_str() );

		modelName = BWResource::resolveFilename( modelName );
		std::replace( modelName.begin(), modelName.end(), '/', '\\' );
		BW::wstring wmodelName = bw_utf8tow( modelName );
		fileDlg.initialDir( wmodelName.c_str() );

		if (fileDlg.showDialog())
		{
			fileName = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ));
			fileName = fileName.substr( 0, fileName.rfind(".") );
			return true;
		}
	}
	
	return false;
}

/**
 * This method tests to see if the given file is read only, and display a message box.
 *
 *  @param	file			Path of the file to test for read only.
 *
 *	@return Whether the file is read only.
 */
bool Mutant::isFileReadOnly( const BW::string& file ) const
{
	BW_GUARD;

	BW::wstring fullPath = BWResource::resolveFilenameW( file );
	DWORD att = GetFileAttributes( fullPath.c_str() );
	if (att & FILE_ATTRIBUTE_READONLY)
	{
		if (!CStdMf::checkUnattended())
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/READ_ONLY", file ), 
				Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/READ_ONLY_TITLE" ),
				MB_ICONEXCLAMATION | MB_OK );
		}

		return true;
	}
	return false;
}

/**
 * This method tests to see if any of the files associated with the model are read only.
 * Checks .model, .visual, .primitives.
 * .animation files can be checked separately.
 * Displays a message box if a one of the files is read only.
 *
 *  @param	modelName		Path of the model to test for read only.
 *	@param	visualName		The path of the visual file to test for read only
 *	@param	primitivesName	The path of the primitives file to test for read only
 *
 *	@return Whether any of the files associated with the model are read only.
 */
bool Mutant::testReadOnly( const BW::string& modelName, const BW::string& visualName, const BW::string& primitivesName ) const
{
	BW_GUARD;

	// Test the main files
	if ( isFileReadOnly( modelName ) )
		return true;
		
	if ( isFileReadOnly( visualName ) )
		return true;
		
	if ( isFileReadOnly( primitivesName ) )
		return true;

	return false;
}

/**
 * This method tests to see if an animation file is read only.
 *
 *	@param	animName		Name of the animation (without file extension).
 *
 *	@return Whether the animation file is read only.
 */
bool Mutant::testReadOnlyAnim( const DataSectionPtr& pData ) const
{
	BW_GUARD;

	// Get filename (without ".animation" extension)
	BW::string animFile = pData->readString( "nodes", "" );

	// No file name - mark as read only
	if ( animFile.empty() )
	{
		//ME_INFO_MSGW(
		//	Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOCATE",
		//	animFile) );
		INFO_MSG( "File name empty" );
		return true;
	}

	// Can't find file - mark as read only
	animFile += ".animation";
	if (!BWResource::fileExists( animFile ))
	{
		//ME_INFO_MSGW(
		//	Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOCATE",
		//	animFile) );
		INFO_MSG( ("File not found " + animFile).c_str() );
		return true;
	}

	// Found file - check if read only
	BW::wstring fullPath = BWResource::resolveFilenameW( animFile );
	DWORD att = GetFileAttributes( fullPath.c_str() );
	if (att & FILE_ATTRIBUTE_READONLY)
	{
		//ME_INFO_MSGW(
		//	Localise(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/READ_ONLY",
		//	animFile) );
		INFO_MSG( ("File is read only: " + animFile).c_str() );
		return true;
	}

	// Read/write
	return false;
}

/**
 * This method tests the validity of the model requested.
 *
 *  @param	name			Path of the model to test for validity.
 *  @param	what			What operation is being exectuted (e.g. "load", "add", etc)

 *  @param	model			The datasection of the model to return
 *  @param	visual			The datasection of the visual to return
 *	@param	visualName		The path of the visual file to return
 *	@param	primitivesName	The path of the primitives file to return
 *	@param	readOnly		Returns true if any of the model's associated files are read only
 *
 *	@return Whether the file was valid.
 */
bool Mutant::ensureModelValid( const BW::string& name, const BW::string& what,
	DataSectionPtr* model /* = NULL */, DataSectionPtr* visual /* = NULL */,
	BW::string* visualName /* = NULL */, BW::string* primitivesName /* = NULL */,
	bool* readOnly /* = NULL */)
{
	BW_GUARD;

	//This will get the data section for the visual file,
	// this is used for the node heirarchy
	DataSectionPtr testModel = BWResource::openSection( name, false );
	DataSectionPtr testVisual = NULL;
	BW::string testRoot = "";

	BW::string definedPrimitivesName;

	if (testModel)
	{
		testRoot = testModel->readString("nodefullVisual","");
		BW::string oldRoot;
		BW::string newRoot;

		if (testRoot != "")
		{
			testVisual = BWResource::openSection( testRoot + ".visual", false );

			if (!testVisual)
			{
				oldRoot = testRoot;

				if (!locateFile( testRoot, name, "visual", what, LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOAD_OTHERWISE") ))
					return false;

				newRoot = testRoot;
				testVisual = BWResource::openSection( testRoot + ".visual", false );
				testModel->writeString( "nodefullVisual", testRoot );

				newRoot = newRoot.substr( 0, newRoot.find_last_of( "/" ));
				oldRoot = oldRoot.substr( 0, oldRoot.find_last_of( "/" ));
				clipToDiffRoot( newRoot, oldRoot );

				//Fix all the animation references
				BW::vector< DataSectionPtr > anims;
				testModel->openSections( "animation", anims );

				for (unsigned i=0; i<anims.size(); i++)
				{
					BW::string animRoot = anims[i]->readString( "nodes", "" );
					if (BWResource::fileExists( animRoot + ".animation" )) continue;
					BW::string::size_type pos = animRoot.find( oldRoot );
					if (pos != BW::string::npos)
					{
						animRoot = animRoot.replace( pos, oldRoot.length(), newRoot );
						anims[i]->writeString( "nodes", animRoot );
					}
				}

				//Fix all the tint references
				BW::vector< DataSectionPtr > dyes;
				testModel->openSections( "dye", dyes );
				for (unsigned i=0; i<dyes.size(); i++)
				{
					BW::vector< DataSectionPtr > tints;
					dyes[i]->openSections( "tint", tints );
					for (unsigned j=0; j<tints.size(); j++)
					{
						fixTextures( tints[j]->openSection("material"), oldRoot, newRoot );
					}
				}

#if ENABLE_RELOAD_MODEL
				// We don't want the model to be reloaded.
				BWResource::instance().ignoreFileModification( name.c_str(),
					ResourceModificationListener::ACTION_MODIFIED, true );
#endif//ENABLE_RELOAD_MODEL
				testModel->save();
			}
		}
		else
		{
			testRoot = testModel->readString("nodelessVisual","");

			if (testRoot != "")
			{
				testVisual = BWResource::openSection( testRoot + ".visual", false );

				if (!testVisual)
				{
					oldRoot = testRoot;

					if (!locateFile( testRoot, name, "visual", what, LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOAD_OTHERWISE") ))
						return false;
					
					newRoot = testRoot;
					testVisual = BWResource::openSection( testRoot + ".visual", false );
					testModel->writeString( "nodelessVisual", testRoot );
					
					newRoot = newRoot.substr( 0, newRoot.find_last_of( "/" ));
					oldRoot = oldRoot.substr( 0, oldRoot.find_last_of( "/" ));
					clipToDiffRoot( newRoot, oldRoot );

#if ENABLE_RELOAD_MODEL
					// We don't want the model to be reloaded.
					BWResource::instance().ignoreFileModification( name.c_str(),
						ResourceModificationListener::ACTION_MODIFIED, true );
#endif//ENABLE_RELOAD_MODEL
					testModel->save();
				}
			}
			else
			{
				testRoot = testModel->readString("billboardVisual","");

				if (testRoot != "")
				{
					testVisual = BWResource::openSection( testRoot + ".visual", false );

					if (!testVisual)
					{
						oldRoot = testRoot;

						if (!locateFile( testRoot, name, "visual", what, LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_VALIDATION/UNABLE_LOAD_OTHERWISE") ))
							return false;

						newRoot = testRoot;
						testVisual = BWResource::openSection( testRoot + ".visual", false );
						testModel->writeString( "billboardVisual", testRoot );

						newRoot = newRoot.substr( 0, newRoot.find_last_of( "/" ));
						oldRoot = oldRoot.substr( 0, oldRoot.find_last_of( "/" ));
						clipToDiffRoot( newRoot, oldRoot );

#if ENABLE_RELOAD_MODEL
						// We don't want the model to be reloaded.
						BWResource::instance().ignoreFileModification( name.c_str(),
							ResourceModificationListener::ACTION_MODIFIED, true );
#endif//ENABLE_RELOAD_MODEL
						testModel->save();
					}
				}
				else
				{
					ERROR_MSG( "Cannot determine the visual type of the model\"%s\".\n"
						"Unable to %s model.", name.c_str(), what.c_str() );
					return false;
				}
			}
		}

		// Update any references in the visual file if needed
		if (newRoot != oldRoot)
		{
			bool visualChange = false;

			BW::vector< DataSectionPtr > renderSets;
			testVisual->openSections( "renderSet", renderSets );
			for (unsigned i=0; i<renderSets.size(); i++)
			{
				BW::vector< DataSectionPtr > geometries;
				renderSets[i]->openSections( "geometry", geometries );
				for (unsigned j=0; j<geometries.size(); j++)
				{
					BW::string::size_type pos;
					BW::string newVerts = geometries[j]->readString("vertices","");
					pos = newVerts.find( oldRoot );
					if (pos != BW::string::npos)
					{
						newVerts = newVerts.replace( pos, oldRoot.length(), newRoot );
						geometries[j]->writeString( "vertices", newVerts );
						visualChange = true;
					}

					BW::string newStream = geometries[j]->readString("stream","");
					pos = newStream.find( oldRoot );
					if (pos != BW::string::npos)
					{
						newStream = newStream.replace( pos, oldRoot.length(), newRoot );
						geometries[j]->writeString( "stream", newStream );
						visualChange = true;
					}

					BW::string newPrims = geometries[j]->readString("primitive","");
					pos = newPrims.find( oldRoot );
					if (pos != BW::string::npos)
					{
						newPrims = newPrims.replace( pos, oldRoot.length(), newRoot );
						geometries[j]->writeString( "primitive", newPrims );
						visualChange = true;
					}

					BW::vector< DataSectionPtr > primGroups;
					geometries[j]->openSections( "primitiveGroup", primGroups );
					for (unsigned k=0; k<primGroups.size(); k++)
					{
						BW::vector< DataSectionPtr > materials;
						primGroups[k]->openSections( "material", materials );
						for (unsigned l=0; l<materials.size(); l++)
						{
							visualChange = fixTextures( materials[l], oldRoot, newRoot ) || visualChange;
						}
					}
				}
			}
		
			if (visualChange)
			{
				testVisual->save();
			}
		}

		definedPrimitivesName = 
			testVisual->readString( "primitivesName", testRoot );
		definedPrimitivesName.append ( ".primitives" );

		if ( ! BWResource::openSection( definedPrimitivesName ) )
        {
			ERROR_MSG( "Unable to find the primitives file for \"%s\".\n\n"
				"Make sure that \"%s\" exists.\n\n"
				"Unable to %s model.\n", name.c_str(), definedPrimitivesName.c_str(), what.c_str() );

			return false;
        }

		// Make sure the parent model exists and can be found
		BW::string oldParentRoot = testModel->readString( "parent", "" );
		BW::string parentRoot = oldParentRoot;
		if (oldParentRoot != "")
		{
			DataSectionPtr testParentModel = BWResource::openSection( oldParentRoot + ".model", false );
			if (testParentModel == NULL)
			{
				parentRoot = oldParentRoot;
				if (locateFile( parentRoot, name, "model", what ))
				{
					testModel->writeString( "parent", parentRoot );
#if ENABLE_RELOAD_MODEL
					// We don't want the model to be reloaded.
					BWResource::instance().ignoreFileModification( name.c_str(),
						ResourceModificationListener::ACTION_MODIFIED, true );
#endif//ENABLE_RELOAD_MODEL
					testModel->save();
				}
			}

			if (!ensureModelValid( parentRoot + ".model", what, NULL, NULL, NULL, NULL, readOnly )) return false;
		}
	}
	else
	{
		// this means that no model file could be opened.
		return false;
	}

	if (definedPrimitivesName.empty())
	{
		definedPrimitivesName = testRoot + ".primitives";
	}
	
	if ( isFormatDepreciated( testVisual, definedPrimitivesName ) )
	{
		ERROR_MSG( "The model: \"%s\" is using a depreciated vertex format.\n"
				   "Please re-export this model and try again.\n", name.c_str() );
		return false;
	}
	// Update any parameters requested
	
	if (model)
		*model = testModel;

	if (visual)
		*visual = testVisual;

	if (visualName)
		*visualName = testRoot + ".visual";

	if (primitivesName)
		*primitivesName = definedPrimitivesName;

	if (readOnly)
		*readOnly |= testReadOnly( name, testRoot + ".visual", definedPrimitivesName );
	
	return true;
}

/**
 *	This method checks to see whether the model being load is using a depreciated vertex format.
 *
 *	@param visual A DataSectionPtr to the visual of this model.
 *	@param primitivesName The name of the primitives file.
 *
 *	@return Returns true if the vertex format used by the model is depreciated, false otherwise.
 */
bool Mutant::isFormatDepreciated( DataSectionPtr visual, const BW::string& primitivesName )
{
	BW_GUARD;

	DataSectionPtr primFile = PrimitiveFile::get( primitivesName );
	if (!primFile)
	{
		primFile = new BinSection( 
			primitivesName, new BinaryBlock( 0, 0, "BinaryBlock/FormatTest" ) );
	}

	// iterate through our rendersets
	BW::vector< DataSectionPtr >	pRenderSets;
	visual->openSections( "renderSet", pRenderSets );
	BW::vector< DataSectionPtr >::iterator renderSetsIt = pRenderSets.begin();
	BW::vector< DataSectionPtr >::iterator renderSetsEnd = pRenderSets.end();
	while (renderSetsIt != renderSetsEnd)
	{
		DataSectionPtr renderSet = *renderSetsIt++;

		// iterate through our geometries
		BW::vector< DataSectionPtr >	pGeometries;
		renderSet->openSections( "geometry", pGeometries );
		BW::vector< DataSectionPtr >::iterator geometriesIt = pGeometries.begin();
		BW::vector< DataSectionPtr >::iterator geometriesEnd = pGeometries.end();
		while (geometriesIt != geometriesEnd)
		{
			DataSectionPtr geometries = *geometriesIt++;

			BW::string vertGroup = geometries->readString( "vertices", "" );
			if ( vertGroup.find_first_of( '/' ) < vertGroup.size() )
			{
				BW::string fileName, partName;
				splitOldPrimitiveName( vertGroup, fileName, partName );
				vertGroup = partName;
			}
			// get the vertices information
			BinaryPtr pVertices = primFile->readBinary( vertGroup );
			if (pVertices)
			{
				const Moo::VertexHeader * vh = 
					reinterpret_cast< const Moo::VertexHeader* >( pVertices->data() );
				BW::string format = BW::string( vh->vertexFormat_ );

				if ((format == "xyznuvi") || (format == "xyznuvitb"))
				{
					return true;
				}
			}
		}
	}
	return false;
}
BW_END_NAMESPACE

