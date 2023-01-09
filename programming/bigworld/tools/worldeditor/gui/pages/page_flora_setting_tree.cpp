#include "pch.hpp"
#include "cstdmf/string_utils.hpp"
#include "common/string_utils.hpp"
#include "ual_manager.hpp"
#include "list_file_provider.hpp"
#include "list_xml_provider.hpp"
#include "page_flora_setting.hpp"
#include "page_flora_setting_tree.hpp"
#include "common/user_messages.hpp"
#include "cstdmf/message_box.hpp" 
#include "romp/ecotype_generators.hpp"
#include "resmgr/auto_config.hpp"
#include "ual/ual_dialog.hpp"
#include "panel_manager.hpp"
#include "moo/visual_manager.hpp"
#include "moo/texture_manager.hpp"
#include "page_terrain_texture.hpp"
#include "romp/flora.hpp"
#include "chunk/chunk_manager.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_notFoundTexture( "system/notFoundBmp" );
#define FLOAT_ITEM_FORMAT_SPLIT  L" : "
#define IMAGE_SIZE 16
#define TREE_ITEM_HIGHLIGHT 1
#define EXPAND_SECTION "editorOnly/expand"


/////////////FloraSettingTreeSection//////////////////////////////

FloraSettingTreeItemParam::FloraSettingTreeItemParam():
	pSection_( NULL),
	error_( 0 )
{
	BW_GUARD;
}


VisualParam::VisualParam():
	textureName_( ""),
	numVertices_( -1 )
{
	BW_GUARD;
}


VisualGeneratorParam::VisualGeneratorParam():
	hDerivedVisualItem_( NULL )
{
	BW_GUARD;
}


EcotypeParam::EcotypeParam():
	hDerivedVisualGeneratorItem_( NULL )
{
	BW_GUARD;
}



TextureParam::TextureParam()
{
	BW_GUARD;
	extension_[0] = 0;
}

void TextureParam::setExtension( const char* extention )
{
	BW_GUARD;
	bw_str_copy( extension_, ARRAY_SIZE( extension_ ), extention );
}

/**
 *	This method create a tree item control
 *	@param	parentItem		the parent item in the TreeCtrl
 *	@param	pDataSection	if specified, we read the value from it, 
 *							otherwise use default value.
 *	@param	hInsertAfter	Handle of the item after which the new item
 *							is to be inserted
*/
/*virtual*/ HTREEITEM FloraSettingTreeSection::createItem( 
							HTREEITEM parentItem,
							DataSectionPtr pDataSection /*= NULL*/, 
							HTREEITEM hInsertAfter /*= TVI_LAST*/ )
{
	BW_GUARD;
	// Insert the new one.
	BW::string text; 
	bool gotText = false;
	if (pDataSection && parentItem != NULL)
	{
		FloraSettingTreeSection* pParentSection = tree().getItemSection( parentItem );
		//not ie: <generator> chooseMax
		if (pParentSection && 
			!pParentSection->isValidChildSection( pDataSection->asString() )&& 
			!pParentSection->isValidChildSection( pDataSection->sectionName() ))
		{
			// ie. <grass> <stone> , whose
			// sectionName is <ecotype> but item text is different.
			text = pDataSection->sectionName();
			gotText = true;
		}
	}

	if (!gotText && pDataSection && this->nameByDataSectionName())
	{
		// ie:<texture>	maps/landscape/a_desertsand01.dds </texture>
		BW::string value = pDataSection->asString();
		std::replace( value.begin(), value.end(), '\\', '/' );
		if (value != "" && !this->isValidChildSection( value ))
		{
			text = value;
			gotText = true;
		}
	}
	if (!gotText)
	{
		text = name();//normal section
	}

	wchar_t wText[256];
	bw_utf8tow( text, wText, 256 );
	MF_ASSERT( wcslen( wText ) < 256 );
	HTREEITEM newItem =tree().InsertItem( wText, parentItem, hInsertAfter );
	this->createItemParam( newItem );
	return newItem;
}


/**
 *	This method create an item param for the item
 */
void FloraSettingTreeSection::createItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	// To do: probably should only new an object for the nodes who have their
	// private data, the other nodes can share same object.
	FloraSettingTreeItemParam* pParam = new FloraSettingTreeItemParam();
	pParam->pSection_ = this;
	tree().SetItemData( hItem, (DWORD_PTR)pParam );
}


// put as inline later.
FloraSettingTree& FloraSettingTreeSection::tree()
{
	BW_GUARD;
	return pPageWnd_->tree(); 
}

/**
 *	This method create an item param for the item
 */
void FloraSettingTreeSection::deleteItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	// To do: probably should only new an object for the nodes who have their
	// private data, the other nodes can share same object.
	FloraSettingTreeItemParam* pParam = 
		(FloraSettingTreeItemParam*)tree().GetItemData( hItem );
	delete pParam;
	tree().SetItemData( hItem, NULL );
}


/**
 *	This method check if sectionName is qualified as my child
 *	if yes return the correct FloraSettingTreeSection
 *	@param	pRetNameBySectionName		if not NULL, and if it's a valid child,
 *										return it' nameBySectionName_.
 *	@param	pRetIsRequired				if not NULL, and if it's a valid child,
 *										return it's required_
 */
bool FloraSettingTreeSection::isValidChildSection( 
										const BW::string& name,				 
										bool* pRetNameBySectionName /*= NULL*/, 
										bool* pRetIsRequired /*=NULL*/ )
{
	BW_GUARD;
	BW::vector< ChildSection >::iterator it = childSections_.begin();
	for (; it != childSections_.end(); ++it)
	{
		if (name == (*it).name())
		{
			if (pRetNameBySectionName)
			{
				*pRetNameBySectionName = (*it).nameByDataSectionName();
			}
			if (pRetIsRequired)
			{
				*pRetIsRequired = (*it).required_;
			}
			return true;
		}
	}
	return false;
}


/**
 *	This method check if hItem is deletable.
 */
/*virtual*/ bool FloraSettingTreeSection::isDeletable( HTREEITEM hItem )
{
	BW_GUARD;
	HTREEITEM hParentItem =tree().GetParentItem( hItem );
	if (hParentItem != NULL && hParentItem != tree().GetRootItem() )
	{
		//We don't delete flora.xml and the direct child of flora.xml
		FloraSettingTreeSection* pParentSection = 
									tree().getItemSection( hParentItem );
		MF_ASSERT( pParentSection );
		bool bySectionName = false;
		bool isRequired = false;
		pParentSection->isValidChildSection( name(), &bySectionName, &isRequired );
		if (!isRequired)
		{
			return true;
		}
	}
	return false;
}


/**
 *	This method create a default, tree item control, if pDefaultValue
 *	is specified, then set it's value as *pDefaultValue,  if createChildren
 *	@param	parentItem		the parent item in the TreeCtrl
 *	@param	hInsertAfter	Handle of the item after which 
 *							the new item is to be inserted
 */
/*virtual*/HTREEITEM FloraSettingTreeSection::createDefaultItem(
								HTREEITEM parentItem, 
								HTREEITEM hInsertAfter /*= TVI_LAST*/   )
{
	BW_GUARD;
	// Insert the new one.
	HTREEITEM newItem = this->createItem( parentItem, NULL, hInsertAfter );
	

	//children
	BW::vector< ChildSection >::iterator it = childSections_.begin();
	for (; it != childSections_.end(); ++it)
	{
		ChildSection& childSection = (*it);
		if (childSection.required_)
		{
			FloraSettingTreeSection* pSection =tree().section( 
				childSection.name(), childSection.nameByDataSectionName() );
			MF_ASSERT( pSection );
			pSection->createDefaultItem( newItem );
		}
	}

	return newItem;
}


/**
 *	This method is a bit hack, 
 *	it tells when addition/deletion/moveupdown
 *	happen on this section's item, should ecotype's
 *  be notified to update texture. also when click
 *  on this section and toggle visual in scene is 
 *  on, engine need high light the visuals.
 *	it also gives the index path so that Flora can 
 *  find right VisualsEcotype::VisualCopy to high light.
 *	@param	includeEcotypeSection		if true include the ecotype section.
 *	@param	pIndexPathDependent			if not NULL, we return if index path
 *											depend on it or not.
 */
bool FloraSettingTreeSection::isEcotypeVisualDependent(
					bool includeEcotypeSection /*= false*/,
					bool* pIndexPathDependent /*= NULL*/ )
{
	BW_GUARD;
	struct VisualDependent
	{
		VisualDependent( FloraSettingTreeSection* pSection, bool indexDependent ):
			pSection_( pSection), indexPathDependent_( indexDependent ){}
		FloraSettingTreeSection* pSection_;
		bool indexPathDependent_;
	};
	static VisualDependent ecotypeTextureDependentSections[8] = {
		VisualDependent( tree().section( "ecotype", false ), true ),
		VisualDependent( tree().section( "generator", true ), false ),
		VisualDependent( tree().section( "chooseMax", false ), false ),
		VisualDependent( tree().section( "noise", true ), true ),
		VisualDependent( tree().section( "random", true ), true ),
		VisualDependent( tree().section( "fixed", true ), true ),
		VisualDependent( tree().section( "visual", false ), false ),
		VisualDependent( tree().section( "visual", true ) , true )
	};
	VisualDependent* pEnd = &(ecotypeTextureDependentSections[7]);
	VisualDependent* p = ecotypeTextureDependentSections;
	if (!includeEcotypeSection)
	{
		p++;
	}	
	while (p <= pEnd)
	{
		if (this == p->pSection_)
		{
			if (pIndexPathDependent)
			{
				*pIndexPathDependent = p->indexPathDependent_;
			}
			return true;
		}
		p++;
	}
	return false;
}


/**
 *	This method happens when an item is deleted and update texture the ecotype 
 *	it belongs to.
 *	@param	hParentItem			the parent item whose child has just been deleted.
 *	@param	pDeletedItemSection	the section of the item who was just deleted.
 */
/*virtual*/ void FloraSettingTreeSection::postChildDeleted( 
								HTREEITEM hParentItem,
								FloraSettingTreeSection* pDeletedItemSection )
{
	BW_GUARD;
	// Notify the ecotype item to update it's texture.
	EcotypeSection *pEcotypeSection = 
							(EcotypeSection*)tree().section( "ecotype", false );
	MF_ASSERT( pEcotypeSection );

	if (pDeletedItemSection->isEcotypeVisualDependent())
	{
		HTREEITEM hEcotypeItem = 
				tree().findFirstParentBySection( hParentItem, pEcotypeSection );
		if (hEcotypeItem!= NULL )
		{
			pEcotypeSection->updateVisualTexture( hEcotypeItem );
		}
	}

}



/**
 *	This method happens when an item is about to be deleted.
 *	@param	hParentItem			the parent item
 *	@param	hChildItem			the child item
 */
/*virtual*/ void FloraSettingTreeSection::preChildDeleted( 
								HTREEITEM hParentItem,	HTREEITEM hChildItem )
{
	BW_GUARD;
	EcotypeSection *pEcotypeSection = 
							(EcotypeSection*)tree().section( "ecotype", false );
	MF_ASSERT( pEcotypeSection );
	// if it's the ecotype, we need count on the changes caused by it's child 
	// texture items. maybe we need do this for every section, 
	// but to be efficient we only do this for ecotype atm.
	if (tree().getItemSection( hChildItem ) == pEcotypeSection )
	{
		HTREEITEM hGrandChildItem =tree().GetChildItem( hChildItem );
		while (hGrandChildItem != NULL)
		{
			pEcotypeSection->preChildDeleted( hChildItem, hGrandChildItem );

			hGrandChildItem =tree().GetNextItem( hGrandChildItem, TVGN_NEXT );
		}
	}
}

/**
 *	This method happens when an item is added.
 *	@param	hParentItem			the parent item.
 *	@param	hChildItem			the child item
 */
/*virtual*/ void FloraSettingTreeSection::postChildAdded( HTREEITEM hParentItem,
							HTREEITEM hChildItem, bool moveOnly /*= false*/ )
{
	BW_GUARD;
	// If it's any of the ecotype's texture dependent item, Notify the ecotype 
	// item to update it's texture.
	EcotypeSection *pEcotypeSection = 
							(EcotypeSection*)tree().section( "ecotype", false );
	MF_ASSERT( pEcotypeSection );

	FloraSettingTreeSection* pChildSection = tree().getItemSection( hChildItem );
	MF_ASSERT( pChildSection );
	if (pChildSection->isEcotypeVisualDependent())
	{
		HTREEITEM hEcotypeItem = 
				tree().findFirstParentBySection( hParentItem, pEcotypeSection );
		if (hEcotypeItem!= NULL )
		{
			pEcotypeSection->updateVisualTexture( hEcotypeItem );
		}
	}

	// if it's the ecotype, we need count on the changes caused by it's 
	// child texture items. maybe we need do this for every section, 
	// but to be efficient we only do this for ecotype atm.
	if (tree().getItemSection( hChildItem ) == pEcotypeSection )
	{
		HTREEITEM hGrandChildItem =tree().GetChildItem( hChildItem );
		while (hGrandChildItem != NULL)
		{
			pEcotypeSection->postChildAdded( hChildItem, hGrandChildItem, moveOnly );

			hGrandChildItem =tree().GetNextItem( hGrandChildItem, TVGN_NEXT );
		}

	}
}


/*virtual*/ bool FloraSettingTreeSection::prepareEorrorMessage( 
									HTREEITEM hItem, BW::wstring& errorMessage )
{
	BW_GUARD;
	errorMessage = L"";
	FloraSettingTreeItemParam* pParam = 
						(FloraSettingTreeItemParam*)(tree().GetItemData( hItem ));

	uint32 errorMask = MIN_ERROR;
	while (errorMask <= MAX_ERROR)
	{
		BW::wstring error = L"";
		switch( pParam->error_ & errorMask )
		{
		case VISUAL_TEXTURE_WRONG:
			{
				error = Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/VISUAL_TEXTURE_WRONG" );
				break;
			}
		case FILE_MISSING:
			{
				error = Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/FILE_MISSING" );
				break;
			}
		case TERRAIN_USED_IN_OTHER:
			{
				error = Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/TERRAIN_USED_IN_OTHER" );
				break;
			}
		}
		if (error != L"")
		{
			if (errorMessage != L"")
			{
				errorMessage += L"; ";
			}
			errorMessage += error;
		}

		errorMask = errorMask << 1;
	}
	return errorMessage != L"";
}

/**
 *	This method init this Section from the sections in flora_setting_page.xml
 */
/*virtual*/ void FloraSettingTreeSection::init( DataSectionPtr pConfigSection, 
													PageFloraSetting* pPageWnd )
{ 
	BW_GUARD;
	pPageWnd_ = pPageWnd;
	if (pConfigSection)
	{
		this->name() = pConfigSection->sectionName();
		this->nameByDataSectionName() = pConfigSection->readBool( 
														"by_section_name", true );


		//possible child section(s)
		childSections_.clear();
		BW::vector<DataSectionPtr> childSections;
		pConfigSection->openSections( "provider/childSection" , childSections );
		BW::vector<DataSectionPtr>::iterator it = childSections.begin();
		for (; it != childSections.end(); ++it)
		{
			ChildSection childSection;
			childSection.name() = (*it)->asString();
			childSection.nameByDataSectionName() = (*it)->readBool( 
														"by_section_name", true );
			childSection.displayInPropertyWindow_ = (*it)->readBool( 
											"display_in_property_window", false );
			childSection.required_ = (*it)->readBool( "required", false );
			childSections_.push_back( childSection );
		}
	}

}


/**
 *	This method save hItem into pDataSection
 *	@param	pParentDataSection		The data section as our parent
 *	@param	hItem					the item to be saved.
 *	@param	pParentSection			the FloraSettingTreeSection of our parent item
*									can be NULL if we are saving for undo/redo
 *	@param	pHighLightVisuals		It tells which visuals need save <highlight>
 *										just for WE to high light visual in 3d scene
*/
/*virtual*/  DataSectionPtr FloraSettingTreeSection::save( 
						DataSectionPtr pParentDataSection, 
						HTREEITEM hItem, FloraSettingTreeSection* pParentSection,
						BW::vector<HTREEITEM>* pHighLightVisuals /*= NULL*/)
{
	BW_GUARD;
	DataSectionPtr pMySection;
	BW::string itemText;
	bw_wtoutf8(tree().GetItemText( hItem ), itemText );
	if (this->nameByDataSectionName())
	{
		if (!pParentDataSection)
		{
			pMySection = new XMLSection( this->name() );
		}
		else
		{
			pMySection = pParentDataSection->newSection( this->name() );
		}

		// For something like: <visual> dry_grass/grasslands_tall_4tris.visual
		if (itemText != this->name() )
		{
			pMySection->setString( itemText );
		}
	}
	// ecotype like grass, stone
	else if (pParentSection && !pParentSection->isValidChildSection( itemText ))
	{
		if (!pParentDataSection)
		{
			pMySection = new XMLSection( itemText );
		}
		else
		{
			pMySection = pParentDataSection->newSection( itemText );
		}
	}
	else // ie: chooseMax, visual(generator)
	{
		if (!pParentDataSection)
		{
			pParentDataSection = new XMLSection( pParentSection->name() );
		}
		pMySection = pParentDataSection;
		pMySection->setString( name() );
	}

	if (itemText != "ecotypes" && // don't save for ecotypes, otherwise engine will think it's a ecotype
		TVIS_EXPANDED &tree().GetItemState( hItem, TVIS_EXPANDED ))
	{
		pMySection->writeBool( EXPAND_SECTION, true );
	}

	// For children.
	HTREEITEM hChildItem =tree().GetChildItem( hItem );
	while (hChildItem != NULL)
	{
		FloraSettingTreeSection* pSection =tree().getItemSection( hChildItem );
		MF_ASSERT( pSection );
		pSection->save( pMySection, hChildItem, this, pHighLightVisuals );

		hChildItem =tree().GetNextItem( hChildItem, TVGN_NEXT );
	}

	return pMySection;
}


/**
 *	This method read the section name from pChildDataSection
 *	and check if it's our legal child section.
 *	@param	pChildDataSection  	The child data section of this section
 */
/*virtual */FloraSettingTreeSection* FloraSettingTreeSection::childSection(
											DataSectionPtr pChildDataSection )
{
	BW_GUARD;
	BW::string sectionName = pChildDataSection->sectionName();
	bool retNameBySectionName;
	if (this->isValidChildSection( sectionName, &retNameBySectionName ))
	{
		MF_ASSERT( retNameBySectionName == true );
		return tree().section( sectionName, retNameBySectionName );
	}
	return NULL;
}


/**
 *	This method populate the tree from the parentItem
 *	@param	pDataSection	The data section that contains the data we need.
 *	@param	parentItem		the parent item in the TreeCtrl
 *	@return					the created item
 */
/*virtual*/HTREEITEM FloraSettingTreeSection::populate(
							DataSectionPtr pDataSection, HTREEITEM parentItem )
{
	BW_GUARD;

	if (!pDataSection)
	{
		return NULL;
	}
	
	HTREEITEM thisItem = this->createItem( parentItem, pDataSection );

	// My title (==asString()) is the child section name
	if (this->nameByDataSectionName())
	{
		BW::string title = pDataSection->asString();
		if (title != "" && this->isValidChildSection( title ))
		{
			FloraSettingTreeSection* pSection = 
						tree().section ( pDataSection->asString(), false );
			MF_ASSERT( pSection );
			HTREEITEM hChildItem = pSection->populate( pDataSection, thisItem );

			this->postChildAdded( thisItem, hChildItem );

			if (pDataSection->readBool( EXPAND_SECTION, false ) ==  true)
			{
				tree().expand( thisItem, false );
			}
			return thisItem;
		}
	}

	if (pDataSection->countChildren() == 0)
	{
		return thisItem;
	}

	// Read my children
	bool creationMark[64];
	memset( creationMark, 0, sizeof(creationMark) );

	// hack, legacy support - visuals directly in the section
	bool hasLoadLegacyVisual = false;
	BW::string hackLegacyVisualParentString = "";

	for (int i = 0; i < pDataSection->countChildren(); ++i)
	{
		DataSectionPtr pChild = pDataSection->openChild( i );
		if (pChild->sectionName() == "editorOnly")
		{
			continue;
		}
		BW::string sectionName;
		FloraSettingTreeSection* pSection = this->childSection( pChild );
		if (pSection)
		{
			sectionName = pChild->sectionName();
		}
		else if ( !hasLoadLegacyVisual && 
				pChild->sectionName() == "visual" &&
				this->isValidChildSection( "generator" ))
		{
			//legacy support - visuals directly in the section
			pSection =tree().section( "generator", true );
			thisItem = pSection->createItem( thisItem );

			// hack here.
			pChild = pDataSection;
			hackLegacyVisualParentString = pChild->asString();
			// Fake a legal visual data section.
			// maybe do pChild->Copy( pDataSection )
			pChild->setString( "visual" );
			pSection =tree().section( "visual", false );
			hasLoadLegacyVisual = true;

			sectionName = "generator";

		}
		if (pSection)
		{
			// Mark the exist required ones so later if missed we will create them
			size_t i = 0;
			BW::vector< ChildSection >::iterator it = childSections_.begin();
			for (; it != childSections_.end(); ++it, ++i)
			{
				ChildSection& childSection = (*it);
				if ( childSection.required_ && 
					sectionName == childSection.name())
				{
					creationMark[ i ] = true;
				}
			}

			HTREEITEM hChildItem = pSection->populate( pChild, thisItem );
			this->postChildAdded( thisItem, hChildItem );
		}
	}

	if (hasLoadLegacyVisual)
	{
		pDataSection->setString( hackLegacyVisualParentString );
	}
		
	// We check if all required childSections are presented,
	// if not create a default section
	size_t i = 0;
	BW::vector< ChildSection >::iterator it = childSections_.begin();
	for (; it != childSections_.end(); ++it, ++i)
	{
		ChildSection& childSection = (*it);
		if ( childSection.required_ && creationMark[ i ] == 0)
		{
			FloraSettingTreeSection* pSection =tree().section( 
				childSection.name(), childSection.nameByDataSectionName() );
			MF_ASSERT( pSection );
			HTREEITEM hChildItem = pSection->createDefaultItem( thisItem );
			this->postChildAdded( thisItem, hChildItem );
		}
	}

	if (pDataSection->readBool( EXPAND_SECTION, false ) ==  true)
	{
		tree().expand( thisItem, false );
	}
	return thisItem;
}


/**
 *	This method consume the dropped assetsInfo
 *	@param assetsInfo	List of items being dragged.
 */
/*virtual*/	void ListProviderSection::consumeAssets( 
					HTREEITEM hParentItem, BW::vector<AssetInfo>& assetsInfo )
{
	BW_GUARD;

	bool changed = false;

	int numInserted = 0;
	HTREEITEM hNewItem;
	FloraSettingTreeSection* pParentSection =tree().getItemSection( hParentItem );

	BW::vector<AssetInfo>::iterator it = assetsInfo.begin();
	for (; it != assetsInfo.end(); it++)
	{
		wchar_t itemText[256];
		if (this->generateItemText( (*it), itemText, 256 ))
		{
			HTREEITEM hChildItem = NULL;
			// Only insert if not exist.
			if (mouseDragAction_ == replaceChild || 
				mouseDragAction_ == addToChildrenNoRepeat)
			{
				if (tree().findChildItem( hParentItem, itemText ) != NULL)
				{
					//already exist, do nothing.
					continue;
				}
			}

			// Need delete the old one before inserting new one.
			if (mouseDragAction_ == replaceChild &&
				tree().ItemHasChildren( hParentItem ))
			{
				hChildItem =tree().GetChildItem( hParentItem );
				if (!tree().deleteItem( hChildItem, true ))
				{
					continue;
				}
			}

			// Insert the new one.
			if (!changed)
			{
				pPageWnd_->preChange();
				changed = true;
			}
			FloraSettingTreeSection* pSection = this->childSection( (*it) );
			MF_ASSERT(pSection);
			hNewItem = pSection->createDefaultItem( hParentItem, TVI_FIRST );
			tree().SetItemText( hNewItem, itemText );

			// Update parent Item
			if (pParentSection)
			{
				pParentSection->postChildAdded( hParentItem, hNewItem );
			}
			// Add undo
			ItemExistenceOperation* pOperation = new ItemExistenceOperation( 
				false, &(tree()), hNewItem );
			UndoRedo::instance().add( pOperation );
			UndoRedo::instance().barrier( LocaliseUTF8( 
				L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/INSERT_ITEM"), 
				false );

			++numInserted;
		}
	}

	tree().Expand( hParentItem, TVE_EXPAND );
	if (numInserted == 1)
	{
		// Expand the inserted item only if single item is dragged.
		tree().Expand( hNewItem, TVE_EXPAND );
	}

	if (changed)
	{
		pPageWnd_->postChange();
	}
}


/*virtual*/ void ListProviderSection::postChildAdded( HTREEITEM hParentItem, 
								HTREEITEM hChildItem, bool moveOnly /*= false*/ )
{
	BW_GUARD;
	// Update exist resource list
	if (pPageWnd_->secondaryWnd().curItem() == hParentItem)
	{
		pPageWnd_->existingResourceList().init( 
							this->prepareExistingResourceProvider( hParentItem ) );
	}	
	FloraSettingTreeSection::postChildAdded( hParentItem, hChildItem, moveOnly );
}


/**
 *	This method update texture used by this visual generator
 *	and notify the ecotype it belongs to.
 */
/*virtual*/ void ListProviderSection::postChildDeleted( HTREEITEM hParentItem,
								FloraSettingTreeSection* pDeletedItemSection)
{
	BW_GUARD;
	// Update exist resource list
	if (pPageWnd_->secondaryWnd().curItem() == hParentItem )
	{
		pPageWnd_->existingResourceList().init( 
							this->prepareExistingResourceProvider( hParentItem ) );
	}

	FloraSettingTreeSection::postChildDeleted( hParentItem, pDeletedItemSection );
}


/**
 *	This method init this Section from the sections in flora_setting_page.xml
 */
/*virtual*/ void ListProviderSection::init( DataSectionPtr pConfigSection,
													PageFloraSetting* pPageWnd )
{
	BW_GUARD;
	FloraSettingTreeSection::init( pConfigSection, pPageWnd );

	//mouse drag action
	BW::string action = pConfigSection->readString( "provider/mouse_drag_action" );
	if (action == "addToChildrenNoRepeat")
	{
		mouseDragAction_ = addToChildrenNoRepeat;
	}
	else if (action == "addToChildrenRepeat")
	{
		mouseDragAction_ = addToChildrenRepeat;
	}
	else if (action == "replaceChild")
	{
		mouseDragAction_ = replaceChild;
	}
	else
	{
		ERROR_MSG( "No valid mouse drag action found!!");
	}

}



/**
 *	This method check if dragged item(s) consumable.
 *	@param assetsInfo	List of items being dragged.
 */
/*virtual*/	bool ListFileProviderSection::isAssetConsumable( 
						HTREEITEM hItem, BW::vector<AssetInfo>& assetsInfo )
{
	BW_GUARD;
	BW::vector<AssetInfo>::iterator it = assetsInfo.begin();
	for (; it != assetsInfo.end(); it++)
	{
		if (StringUtils::matchExtensionT( (*it).text(), extensions_ ))
		{
			return true;
		}
	}
	return false;
}


/**
 *	This method generate the item's text according to asset
 */
/*virutal*/ bool ListFileProviderSection::generateItemText( 
					const AssetInfo& asset,	wchar_t* text, size_t maxText, 
					bool forTooltip /*=false*/)
{
	BW_GUARD;
	bw_str_copy( text, maxText, asset.longText() );
	return (BWResource::resolveToRelativePathT( text, maxText ));
}


/**
 *	This method return the associated Section to the asset
 *	@param assets	asset of the item
 *	@return			the associated Section to the asset
 */
FloraSettingTreeSection*ListFileProviderSection::childSection( AssetInfo& asset )
{
	BW_GUARD;
	MF_ASSERT(childSections_.size() > 0);

	// childSections_[0] has to be associated to the list, 
	// one ListFileProviderSection can't have two types of list.
	return tree().section( childSections_[0].name(), 
		childSections_[0].nameByDataSectionName() );
}


/**
 *	This method init this Section from the sections in flora_setting_page.xml
 */
/*virtual*/ void ListFileProviderSection::init( DataSectionPtr pConfigSection,
													PageFloraSetting* pPageWnd )
{
	BW_GUARD;
	ListProviderSection::init( pConfigSection, pPageWnd );

	DataSectionPtr pProviderSection = pConfigSection->openSection( "provider" );
	MF_ASSERT( pProviderSection );

	DataSectionPtr pDataSection = pProviderSection->openSection( "path" );
	if (!pDataSection)
	{
		ERROR_MSG( "No <path> section in %s\n", FLORA_SETTING_PAGE_XML );
		return;
	}

	paths_.clear();
	extensions_.clear();
	excludeFolders_.clear();
	includeFolders_.clear();

	//// paths:
	BW::wstring wTemp;
	bw_utf8towSW( pDataSection->asString(), wTemp );
	bw_tokenise( wTemp, L",;", paths_ );
	BW::vector<BW::wstring>::iterator p = paths_.begin();
	while (p != paths_.end())
	{
		IFileSystem::FileType fileType = BWResource::resolveToAbsolutePath( *p );
		if (fileType != IFileSystem::FT_DIRECTORY)
		{
			p = paths_.erase( p );
		}
		else
		{
			++p;
		}
	}

	//// extension:
	pDataSection = pProviderSection->openSection( "extension" );
	if (!pDataSection)
	{
		ERROR_MSG( "No <extension> section in %s\n", FLORA_SETTING_PAGE_XML );
		return;
	}
	bw_utf8towSW( pDataSection->asString(), wTemp );
	bw_tokenise( wTemp, L",;", extensions_ );

	//// excludeFolders:
	pDataSection = pProviderSection->openSection( "folderExclude" );
	if (!pDataSection)
	{
		WARNING_MSG( "No <folderExclude> section in %s, "
			"unnecessary folders might be scanned.\n", FLORA_SETTING_PAGE_XML );
	}
	else
	{
		bw_utf8towSW( pDataSection->asString(), wTemp );
		std::replace( wTemp.begin(), wTemp.end(), L'/', L'\\' );
		bw_tokenise( wTemp, L",;", excludeFolders_ );
	}

	assetListProvider_ = pPageWnd->fileScanProvider();
	existResourceListProvider_ = pPageWnd->existingResourceProvider();
}


/**
 *	This method prepare the provider, and return it.
 */
/*virtual*/ ListProviderPtr ListFileProviderSection::prepareAssetListProvider( 
									HTREEITEM hItem, FilterHolder& filterHolder )
{
	BW_GUARD;
	((ListFileProvider*)assetListProvider_.getObject())->init( 
		L"FILE", paths_, extensions_, includeFolders_, excludeFolders_, 
		ListFileProvider::LISTFILEPROV_FILTERNONDDS | 
		ListFileProvider::LISTFILEPROV_DONTFILTERDDS );
	return assetListProvider_;
}


/**
 *	This method prepare the provider for showing the exist resource, and return it.
 */
/*virtual*/ListProviderPtr ListFileProviderSection::prepareExistingResourceProvider(
																HTREEITEM hItem )
{
	BW_GUARD;

	BW::vector<HTREEITEM> displayItems;

	BW::vector< ChildSection >::iterator it_s = childSections_.begin();
	for (; it_s != childSections_.end(); ++it_s)
	{
		ChildSection child = *it_s;
		if (child.displayInPropertyWindow_)
		{
			FloraSettingTreeSection* pSection = 
				tree().section( child.name(), child.nameByDataSectionName() );
			MF_ASSERT( pSection );

			tree().findChildItemsBySection( hItem, pSection, displayItems, false );
			break;
		}
	}
	

	BW::vector<BW::wstring> paths;
	BW::vector<HTREEITEM>::iterator it_i = displayItems.begin();
	for (; it_i != displayItems.end(); ++it_i)
	{
		BW::wstring path =tree().GetItemText( *it_i );
		paths.push_back( path );
	}

	((FixedListFileProvider*)existResourceListProvider_.getObject())->init( paths );
	return existResourceListProvider_;
}


/**
 *	This method init this Section from the sections in flora_setting_page.xml
 */
/*virtual*/ void ListXmlProviderSection::init( DataSectionPtr pConfigSection, 
													PageFloraSetting* pPageWnd )
{
	BW_GUARD;
	ListProviderSection::init( pConfigSection, pPageWnd );

	DataSectionPtr pProviderSection = pConfigSection->openSection( "provider" );
	assetListProvider_ = pPageWnd_->listXmlSectionProvider();

	char temp[256];
	bw_snprintf( temp, sizeof(temp), "%s/tree_view_items/%s/provider",
										FLORA_SETTING_PAGE_XML, name().c_str() );
	bw_utf8towSW( temp, path_ );

	//possible child section(s)
	BW::vector<DataSectionPtr> childrenSections;
	pProviderSection->openSections( "item", childrenSections );
	BW::vector<DataSectionPtr>::iterator it = childrenSections.begin();
	for (; it != childrenSections.end(); ++it)
	{
		ChildSection childSection;
		childSection.name() = (*it)->asString();
		childSection.nameByDataSectionName() = (*it)->readBool( 
														"by_section_name", true );
		childSection.displayInPropertyWindow_ = (*it)->readBool( 
											"display_in_property_window", false );
		childSection.required_ = false;
		childSections_.push_back( childSection );
	}
}


/**
 *	This method prepare the provider, and return it.
 */
/*virtual*/ ListProviderPtr ListXmlProviderSection::prepareAssetListProvider(
									HTREEITEM hItem, FilterHolder& filterHolder )
{
	BW_GUARD;
	((ListXmlSectionProvider*)assetListProvider_.getObject())->init( path_ );
	return assetListProvider_;
}


/**
 *	This method check if dragged item(s) consumable.
 *	@param assetsInfo	List of items being dragged.
 */
/*virtual*/	bool ListXmlProviderSection::isAssetConsumable( 
							HTREEITEM hItem, BW::vector<AssetInfo>& assetsInfo )
{
	BW_GUARD;
	BW::vector<AssetInfo>::iterator it = assetsInfo.begin();
	for (; it != assetsInfo.end(); it++)
	{
		BW::string text;
		bw_wtoutf8( (*it).text(), text );
		if ( this->isValidChildSection( text ) )
		{
			return true;
		}
	}
	return false;
}


/**
 *	This method generate the item's text according to asset
 */
/*virutal*/ bool ListXmlProviderSection::generateItemText( const AssetInfo& asset,
					wchar_t* text, size_t maxText, bool forTooltip /*=false*/ )
{
	BW_GUARD;
	bw_str_copy( text, maxText, forTooltip ? asset.longText() : asset.text() );
	return true;
}


/**
 *	This method return the associated Section to the asset
 *	@param assets	asset of the item
 *	@return			the associated Section to the asset
 */
FloraSettingTreeSection* ListXmlProviderSection::childSection( AssetInfo& asset )
{
	BW_GUARD;
	BW::string sectionName;
	bw_wtoutf8( asset.text(), sectionName );
	bool nameBySectionName = true;
	if (this->isValidChildSection( sectionName, &nameBySectionName ))
	{
		return tree().section( sectionName, nameBySectionName );
	}
	return NULL;
}


/**
 *	This method create a tree item control
 *	@param	parentItem		the parent item in the TreeCtrl
  *	@param	pDataSection	if specified, we read the value from it, 
  *							otherwise use default value.
  *	@param	hInsertAfter	Handle of the item after which the new item 
  *							is to be inserted
*/
/*virtual*/ HTREEITEM FloatSection::createItem( HTREEITEM parentItem,
									DataSectionPtr pDataSection /*= NULL*/, 
									HTREEITEM hInsertAfter /*= TVI_LAST*/ )
{
	BW_GUARD;
	// Insert the new one.
	float value = pDataSection? pDataSection->asFloat(): default_;
	HTREEITEM newItem =tree().InsertItem( L"", parentItem, hInsertAfter );
	this->setNameAndValue( newItem, name(), value );
	this->createItemParam( newItem );

	return newItem;
}


/**
 *	This method use propertyName and float value to format an tree item's text
 *  @param	hItem			the item to set text.
 *	@param	propertyName	the property name the hItem is associated with.
 *	@param	value			the float value
*/
void FloatSection::setNameAndValue( HTREEITEM hItem, 
									const BW::string& propertyName, float value )
{
	BW_GUARD;
	wchar_t buf[ 256 ];
	bw_snwprintf( buf, 256, L"%S%s%.2f", propertyName.c_str(), 
											FLOAT_ITEM_FORMAT_SPLIT, value );
	tree().SetItemText( hItem, buf );
}


/**
 *	This method split the text of hItem and return the propertyName and value
*/
void FloatSection::getNameAndValue( HTREEITEM hItem, wchar_t* pPropertyNameRet,
	size_t retSize, float& value)
{
	BW_GUARD;
	CString itemText;
	itemText = tree().GetItemText( hItem );
	size_t pos;
	if (bw_str_find_last_of( (LPCWSTR)itemText, FLOAT_ITEM_FORMAT_SPLIT, pos ))
	{
		// Property name:
		bw_str_copy( pPropertyNameRet, retSize, (LPCWSTR)itemText, pos );

		pos += wcslen( FLOAT_ITEM_FORMAT_SPLIT );
		value = (float ) _wtof( &(itemText.GetBuffer()[pos]) );
	}
}


/**
 *	This method save hItem into pDataSection
 *	@param	pParentDataSection		The data section as our parent
 *	@param	hItem					the item to be saved.
 *	@param	pParentSection			the FloraSettingTreeSection 
 *									of our parent item
 *	@param	pHighLightVisuals		It's a hack to tell which visuals 
 *									need save <highlight> just for WE to 
 *									high light visual in 3d scene
*/
/*virtual*/  DataSectionPtr FloatSection::save( 
						DataSectionPtr pParentDataSection, 
						HTREEITEM hItem, FloraSettingTreeSection* pParentSection,
						BW::vector<HTREEITEM>* pHighLightVisuals /*= NULL*/)
{
	BW_GUARD;

	wchar_t propertyName[256];
	float fValue;
	this->getNameAndValue( hItem, propertyName, ARRAY_SIZE(propertyName), fValue );

	DataSectionPtr pMySection;
	MF_ASSERT(this->nameByDataSectionName());
	pMySection = pParentDataSection->newSection( name() );
	pMySection->setFloat( fValue );

	return pMySection;
}


/**
 *	This method init this Section from the sections in flora_setting_page.xml
 */
/*virtual*/ void FloatSection::init( DataSectionPtr pConfigSection, 
													PageFloraSetting* pPageWnd )
{
	BW_GUARD;
	FloraSettingTreeSection::init( pConfigSection, pPageWnd );

	DataSectionPtr pProviderSection = pConfigSection->openSection( "provider" );
	MF_ASSERT( pProviderSection && pProviderSection->asString() == "floatSection" );

	min_ = pProviderSection->readFloat( "min", 0.f );
	max_ = pProviderSection->readFloat( "max", 1.f );
	default_ = pProviderSection->readFloat( "default", 1.f );

}


/**
 *	This method check if dragged item(s) consumable.
 *	@param assetsInfo	List of items being dragged.
 */
/*virtual*/ bool AddNewItemSection::validEditedChildItemName( 
											HTREEITEM hParentItem, 
											const wchar_t* childItemName,
											wchar_t* error, size_t lenError )
{
	BW_GUARD;
	if (bw_str_len( childItemName ) > NAME_LENGTH)
	{
		bw_snwprintf( error, lenError, 
			Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/CHARACTER_TOO_LONG" )
			);
		return false;
	}
	if (tree().findChildItem( hParentItem, childItemName ))
	{
		bw_snwprintf( error, lenError, 
			Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/NAME_REPEAT", childItemName )
			);
		return false;
	}
	char nChildItemName[256];
	bw_wtoutf8( childItemName, bw_str_len( childItemName ), nChildItemName, 256 );
	if (!XMLSection::isValidXMLTag( nChildItemName))
	{
		bw_snwprintf( error, lenError, 
			Localise( L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/INVALID_CHARACTER" )
			);
		return false;
	}
	return true;
}


/**
 *	This method new a child item 
 */
HTREEITEM AddNewItemSection::newDefaultChildItem( HTREEITEM parentItem )
{
	MF_ASSERT( childSections().size() == 1 );
	FloraSettingTreeSection* pSection = 
		tree().section( childSections_[0].name(),
									childSections_[0].nameByDataSectionName() );
	MF_ASSERT( pSection );
	HTREEITEM newItem = pSection->createDefaultItem( parentItem );

	// find correct name
	wchar_t newItemName[64];
	uint32 i = 0;
	while(1)
	{
		bw_snwprintf( newItemName, sizeof(newItemName)/sizeof(wchar_t), L"%S%d",
			childSections_[0].name().c_str(), ++i);
		if (!tree().findChildItem( parentItem, newItemName ))
		{
			tree().SetItemText( newItem, newItemName );
			tree().Expand( parentItem, TVE_EXPAND );
			tree().Expand( newItem, TVE_EXPAND );
			tree().EditLabel( newItem );

			// Prepare for undo
			ItemExistenceOperation* pOperation = new ItemExistenceOperation( 
													false, &tree(), newItem );
			UndoRedo::instance().add( pOperation );
			UndoRedo::instance().barrier( LocaliseUTF8( 
				L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM"), 
				false );

			return newItem;
		}
	}
}


/**
 *	This method read the section name from pChildDataSection
 *	and check if it's our legal child section.
 *	@param	pChildDataSection  	The child data section of this section
 */
/*virtual */FloraSettingTreeSection* AddNewItemSection::childSection( 
											DataSectionPtr pChildDataSection )
{
	BW_GUARD;

	MF_ASSERT( childSections_.size() == 1 );
	return tree().section( childSections_[0].name(), false );
}


/**
 *	This method create a tree item control
 *	@param	parentItem		the parent item in the TreeCtrl
  *	@param	pDataSection	if specified, we read the value from it, 
  *							otherwise use default value.
  *	@param	hInsertAfter	Handle of the item after which the 
  *							new item is to be inserted
*/
/*virtual*/ HTREEITEM UnEditableSection::createItem( HTREEITEM parentItem,
							DataSectionPtr pDataSection /*= NULL*/, 
							HTREEITEM hInsertAfter /*= TVI_LAST*/ )
{
	BW_GUARD;
	// Insert the new one.
	MF_ASSERT( pDataSection );
	wchar_t buf[256];
	if (pDataSection->asString() != "")
	{
		bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%S : %S", 
			pDataSection->sectionName().c_str(), pDataSection->asString().c_str() );
	}
	else
	{
		bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%S", 
											pDataSection->sectionName().c_str() );
	}

	HTREEITEM newItem =tree().InsertItem( buf, parentItem, hInsertAfter );
	this->createItemParam( newItem );
	return newItem;
}


/**
 *	This method check if hItem is deletable.
 */
/*virtual*/ bool UnEditableSection::isDeletable( HTREEITEM hItem )
{
	BW_GUARD;
	// We are not editable
	return false;
}


/**
 *	This method read the section name from pChildDataSection
 *	and check if it's our legal child section.
 *	@param pChildDataSection  	The child data section of this section
 */
/*virtual */FloraSettingTreeSection* UnEditableSection::childSection( 
											DataSectionPtr pChildDataSection )
{
	BW_GUARD;
	// If pChildDataSection is a legal registered section, return that section,
	// otherwise it's uneditable, so return this.
	FloraSettingTreeSection* pSection =tree().section( 
										pChildDataSection->sectionName(), true );
	return (pSection) ? pSection : this;
}


/**
 *	This method create an item param for the item
 */
/*virtual */void VisualSection::createItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	VisualParam* pParam = new VisualParam();
	pParam->pSection_ = this;
	tree().SetItemData( hItem, ( DWORD_PTR ) pParam );
}


/**
 *	This method is called by UI and tell the UI need display the visual texture.
 *	@return		true if we have visual texture to display.
 */
/*virtual*/ bool VisualSection::prepareCurDisplayImage( HTREEITEM hItem, 
								wchar_t* description, size_t szDescription, 
								BW::string& imagePath )
{
	BW_GUARD;
	char visualPath[256];
	bw_wtoutf8( (LPCWSTR)tree().GetItemText( hItem ) , visualPath, 256 );

	char thumPath[256];
	bw_str_copy( thumPath, 256, visualPath );
	BWResource::removeExtensionT( thumPath );
	bw_str_append( thumPath, 256, ".thumbnail.jpg" );
	imagePath = thumPath;
	if (BWResource::fileExists( imagePath ))
	{
		VisualParam* pParam = (VisualParam*)tree().GetItemData( hItem );
		MF_ASSERT( pParam );
		if (pParam->numVertices_ == -1)
		{
			// Count how many vertices of the visual
			Moo::VisualPtr pVisual = Moo::VisualManager::instance()->get( visualPath );
			pParam->numVertices_ = (pVisual)? pVisual->nVerticesAsFlora() : 0;
		}

		size_t pos = 0;
		if (bw_str_find_last_of( &(visualPath[0]), "/", pos ))
		{
			pos++;
		}

		const wchar_t* pCDescription = Localise( 
			L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/VISUAL", 
			&visualPath[pos], pParam->numVertices_, 
			pPageWnd_->floraVerticesPerBlock()/pParam->numVertices_ );
		bw_snwprintf( description, szDescription, pCDescription );
		return true;
	}
	return false;
}


/**
 *	This method save hItem into pDataSection
 *	@param	pParentDataSection	The data section as our parent
 *	@param	hItem				the item to be saved.
 *	@param	pParentSection		the FloraSettingTreeSection of our parent item
 *	@param	pHighLightVisuals	It's a hack to tell which visuals need save
 *								<highlight> just for WE to high light visual 
 *								in 3d scene
 */
/*virtual*/  DataSectionPtr VisualSection::save( 
						DataSectionPtr pParentDataSection, 
						HTREEITEM hItem, FloraSettingTreeSection* pParentSection,
						BW::vector<HTREEITEM>* pHighLightVisuals /*= NULL*/)
{
	BW_GUARD;
	DataSectionPtr pMyDataSection = FloraSettingTreeSection::save( 
									pParentDataSection, hItem, pParentSection );
	VisualParam* pParam = (VisualParam*)tree().GetItemData( hItem );
	MF_ASSERT( pParam );
	if (pHighLightVisuals)
	{
		BW::vector<HTREEITEM>::iterator it = pHighLightVisuals->begin();
		for (; it != pHighLightVisuals->end(); ++it)
		{
			if ((*it) == hItem)
			{
				DataSectionPtr pHighlightDataSecton = 
										pMyDataSection->newSection( "highlight" );
				pHighlightDataSecton->setBool( true );
				break;
			}
		}
	}
	return pMyDataSection;
}

/**
 *	This method create an item param for the item
 */
/*virtual */void VisualGeneratorSection::createItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	VisualGeneratorParam* pParam = new VisualGeneratorParam();
	pParam->pSection_ = this;
	tree().SetItemData( hItem, ( DWORD_PTR ) pParam );
}


/*virtual*/ void VisualGeneratorSection::postChildAdded( HTREEITEM hParentItem, 
							HTREEITEM hChildItem, bool moveOnly /*= false*/ )
{
	BW_GUARD;
	// Check if the visual exist.
	FloraSettingTreeItemParam* pParam = 
		(FloraSettingTreeItemParam*)tree().GetItemData( hChildItem );
	bool isVisual = pParam->pSection_ == tree().section( "visual", true );
	if (!moveOnly && isVisual)
	{
		BW::string visualName;
		bw_wtoutf8( tree().GetItemText( hChildItem ), visualName );
		if (!BWResource::fileExists( visualName ))
		{
			tree().markItemWrong( hChildItem, FILE_MISSING );
		}
	}

	//FloraSettingTreeSection::postChildAdded will update ecotype
	this->updateVisualTexture( hParentItem, false );

	ListFileProviderSection::postChildAdded( hParentItem, hChildItem, moveOnly );

	// Update visual texture.
	if (isVisual && pPageWnd_->secondaryWnd().curItem() == hParentItem )
	{
		pPageWnd_->secondaryWnd().propertiesWnd().updateCurDisplayImage();
	}	

}


/**
 *	This method update texture used by this visual generator
 *	and notify the ecotype it belongs to.
 */
/*virtual*/ void VisualGeneratorSection::postChildDeleted( HTREEITEM hParentItem,
									FloraSettingTreeSection* pDeletedItemSection)
{
	BW_GUARD;
	//FloraSettingTreeSection::postChildDeleted will update ecotype
	this->updateVisualTexture( hParentItem, false );

	ListFileProviderSection::postChildDeleted( hParentItem, pDeletedItemSection );

	// Update visual texture.
	if (pPageWnd_->secondaryWnd().curItem() == hParentItem )
	{
		pPageWnd_->secondaryWnd().propertiesWnd().updateCurDisplayImage();
	}	

}


/*virtual*/ ListProviderPtr VisualGeneratorSection::prepareAssetListProvider( 
									HTREEITEM hItem, FilterHolder& filterHolder )
{
	BW_GUARD;
	BW::string ecotypeTextureName;
	if (this->getEcotypeVisualTexture( hItem, ecotypeTextureName ))
	{
		FilterSpecPtr pFilterSpec = 
					filterHolder.findfilter( FLORA_VISUAL_TEXTURE_FILTER_NAME );
		if (pFilterSpec)
		{
			((ValidTextureFilterSpec*)pFilterSpec.getObject())->setTextureName( 
															ecotypeTextureName );
			pFilterSpec->setActive( true );
		}
	}
	return ListFileProviderSection::prepareAssetListProvider( hItem, filterHolder );
}


/**
 *	This method is called by UI and tell the UI need display the visual texture.
 *	@return		true if we have visual texture to display.
 */
/*virtual*/ bool VisualGeneratorSection::prepareCurDisplayImage( 
									HTREEITEM hItem, wchar_t* description,
									size_t szDescription, BW::string& imagePath )
{
	BW_GUARD;
	VisualGeneratorParam* pMyParam = 
							(VisualGeneratorParam*)tree().GetItemData( hItem );
	MF_ASSERT( pMyParam );
	if (pMyParam->hDerivedVisualItem_ == NULL)
	{
		return false;
	}
	VisualParam* pVisualParam = (VisualParam*)tree().GetItemData(
												pMyParam->hDerivedVisualItem_ );
	MF_ASSERT(pVisualParam->textureName_ != "");

	wchar_t wTextureFileName[256];
	bw_utf8tow( pVisualParam->textureName_, wTextureFileName, 256 );
	MF_ASSERT( wcslen( wTextureFileName ) < 256 );

	CString wVisualName;
	wVisualName = tree().GetItemText( pMyParam->hDerivedVisualItem_ );
	size_t pos = 0;
	if ( bw_str_find_last_of( (LPCWSTR)wVisualName, L"/", pos ))
	{
		pos++;
	}
	const wchar_t* pCDescription = Localise( 
					L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/VISUAL_TEXTURE", 
					wTextureFileName, &(wVisualName.GetBuffer()[pos]) );
	bw_snwprintf( description, szDescription, pCDescription );

	if (BWResource::fileExists( pVisualParam->textureName_ ))
	{
		imagePath = pVisualParam->textureName_;
	}
	else
	{
		imagePath = s_notFoundTexture;
	}
	return true;
}


/**
 *	This method find the visual texture used by this visual generator
 *	@param	hItem			the tree view item of the visual generator.
 *	@param	textureName		the returned texture name.
 *	@param	return			true if we found a valid one.
 */
bool VisualGeneratorSection::getVisualTexture( HTREEITEM hItem, 
														BW::string& textureName )
{
	BW_GUARD;
	VisualGeneratorParam* pMyParam = 
							(VisualGeneratorParam*)tree().GetItemData( hItem );
	MF_ASSERT( pMyParam );
	if (pMyParam->hDerivedVisualItem_ == NULL)
	{
		return false;
	}
	VisualParam* pVisualParam = 
			(VisualParam*)tree().GetItemData( pMyParam->hDerivedVisualItem_ );
	MF_ASSERT( pVisualParam );
	textureName = pVisualParam->textureName_;
	return true;
}


/**
 *	This method find the visual texture used by the ecotype belonged to.
 *	@param	hItem			the tree view item of the visual generator.
 *	@param	textureName		the returned texture name.
 *	@return					true if we found a valid one.
 */
bool VisualGeneratorSection::getEcotypeVisualTexture( HTREEITEM hItem, 
													BW::string& textureName )
{
	BW_GUARD;
	EcotypeSection *pEcotypeSection = 
							(EcotypeSection*)tree().section( "ecotype", false );
	MF_ASSERT( pEcotypeSection );

	HTREEITEM hEcotypeItem = 
						tree().findFirstParentBySection( hItem, pEcotypeSection );
	MF_ASSERT( hEcotypeItem );
	return pEcotypeSection->getVisualTexture( hEcotypeItem, textureName );
}


/**
 *	This method find the texture used by this visual generator
 *	and notify the ecotype it belongs to.
 */
void VisualGeneratorSection::updateVisualTexture( HTREEITEM hItem, 
												 bool updateEcotype /*= true*/ )
{
	BW_GUARD;
	FloraSettingTreeSection *pVisuaSection =tree().section( "visual", true );
	MF_ASSERT( pVisuaSection );

	// direct children.
	BW::vector<HTREEITEM> visualItems;
	tree().findChildItemsBySection( hItem, pVisuaSection, visualItems, false );

	VisualGeneratorParam* pMyParam = 
							(VisualGeneratorParam*)tree().GetItemData( hItem );
	MF_ASSERT( pMyParam );
	pMyParam->hDerivedVisualItem_ = NULL;

	BW::vector<HTREEITEM>::iterator it = visualItems.begin();
	for (; it != visualItems.end(); ++it)
	{
		bool foundValidTexture = true;
		HTREEITEM hVisualItem = *it;
		VisualParam* pVisualItemParam = 
								(VisualParam*)tree().GetItemData( hVisualItem );
		MF_ASSERT( pVisualItemParam );
		if (pVisualItemParam->textureName_ == "")
		{
			//update visual item's texture name.
			BW::string itemText;
			bw_wtoutf8( tree().GetItemText( hVisualItem ), itemText );
			MF_ASSERT( itemText != "" );
			if (!VisualsEcotype::findTextureName( itemText, 
											pVisualItemParam->textureName_ ))
			{
				pVisualItemParam->textureName_ = s_notFoundTexture;
				foundValidTexture = false;
			}
			else
			{
				// If .dds exist, use .dds
				char cddsTexture[256];
				bw_str_copy( cddsTexture, 256, pVisualItemParam->textureName_ );
				BWResource::changeExtensionT( cddsTexture, ".dds" );
				BW::string ddsTexture = cddsTexture;
				if (BWResource::fileExists( ddsTexture ))
				{
					pVisualItemParam->textureName_ = ddsTexture;
				}
			}
		}

		// Same way as VisualsEcotype::load(..) does
		if (foundValidTexture)
		{
			pMyParam->hDerivedVisualItem_  = hVisualItem;
		}
	}


	if (updateEcotype)
	{
		// Notify the ecotype item to update it's texture.
		EcotypeSection *pEcotypeSection = 
							(EcotypeSection*)tree().section( "ecotype", false );
		MF_ASSERT( pEcotypeSection );

		HTREEITEM hEcotypeItem = 
						tree().findFirstParentBySection( hItem, pEcotypeSection );
		if (hEcotypeItem!= NULL )
		{
			pEcotypeSection->updateVisualTexture( hEcotypeItem );
		}
	}
}



/**
 *	This method validate the texture of the visuals
 *	under hItem, if it's not valid, high light it.
 */
void VisualGeneratorSection::validateTexture( HTREEITEM hItem, 
											const BW::string& validTextureName )
{
	BW_GUARD;
	MF_ASSERT( validTextureName != "" );
	BW::string myTextureName;
	bool same = ( this->getVisualTexture( hItem, myTextureName ) &&
											myTextureName == validTextureName );
	if (same)
	{
		tree().unmarkItemWrong( hItem, VISUAL_TEXTURE_WRONG );
	}
	else
	{
		tree().markItemWrong( hItem, VISUAL_TEXTURE_WRONG );
	}

	// Check the visuals.
	VisualSection *pVisualSection = 
								(VisualSection*)tree().section( "visual", true );
	MF_ASSERT( pVisualSection );
	BW::vector<HTREEITEM> visualItems;
	tree().findChildItemsBySection( hItem, pVisualSection, visualItems, false );
	BW::vector<HTREEITEM>::iterator it = visualItems.begin();
	for (; it != visualItems.end(); ++it)
	{
		VisualParam* pVisualParam = (VisualParam*)tree().GetItemData( *it );
		MF_ASSERT( pVisualParam );
		if (pVisualParam->textureName_ == validTextureName)
		{
			tree().unmarkItemWrong( *it, VISUAL_TEXTURE_WRONG );
		}
		else
		{
			tree().markItemWrong( *it, VISUAL_TEXTURE_WRONG );
		}
	}

	// Update the filter if the current window is associated to hItem
	if (pPageWnd_->secondaryWnd().curItem() == hItem )
	{
		FilterHolder& filterHolder = pPageWnd_->secondaryWnd().filterHolder();
		FilterSpecPtr pFilterSpec = 
					filterHolder.findfilter( FLORA_VISUAL_TEXTURE_FILTER_NAME );
		if (pFilterSpec &&
			((ValidTextureFilterSpec*)pFilterSpec.getObject())->setTextureName(
															validTextureName )
			&& pFilterSpec->getActive())
		{
			pPageWnd_->assetList().updateFilters();
		}
	}	

}


/**
 *	This method create an item param for the item
 */
/*virtual*/ void TextureSection::createItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	TextureParam* pParam = new TextureParam();
	pParam->pSection_ = this;
	tree().SetItemData( hItem, (DWORD_PTR)pParam );
}


/**
 *	This method is called by UI and tell the UI need display the visual texture.
 *	@return		true if we have visual texture to display.
 */
/*virtual*/ bool TextureSection::prepareCurDisplayImage( HTREEITEM hItem, 
								wchar_t* description, size_t szDescription, 
								BW::string& imagePath )
{
	BW_GUARD;
	char path[256];
	bw_wtoutf8( (LPCWSTR)tree().GetItemText( hItem ), path, 256 );
	TextureParam* pParam = (TextureParam*)tree().GetItemData( hItem );
	MF_ASSERT( pParam );
	if (pParam->extension_[0] != 0)
	{
		BWResource::changeExtensionT( path, pParam->extension_, false );
	}
	imagePath = path;
	bool ret = BWResource::fileExists( imagePath );
	if (ret)
	{
		size_t pos = 0;
		if (bw_str_find_last_of( path, "/", pos ))
		{
			pos++;
		}
		const wchar_t* pCDescription = Localise( 
			L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/TERRAIN_TEXTURE", &(path[pos]) );
		bw_snwprintf( description, szDescription, pCDescription );
	}
	return ret;
}


/**
 *	This method is called when the user double clicked on the item
*	or our current display image.
 *	@return		true if we have handled the event.
 */
/*virtual*/ bool TextureSection::onDoubleClicked( HTREEITEM hItem )
{
	BW_GUARD;

	PanelManager::instance().setToolMode( L"TerrainTexture" );

	// Set the terrain texture brush:
	BW::string imagePath;
	bw_wtoutf8(tree().GetItemText( hItem ), imagePath );
	PageTerrainTexture::instance()->currentTexture( imagePath, "" );

	return true;
}

/**
 *	This method find the visual texture used by this visual generator
 *	@param	hItem			the tree view item of the visual generator.
 *	@param	textureName		the returned texture name.
 *	@return					true if we found a valid one.
 */
bool EcotypeSection::getVisualTexture( HTREEITEM hItem, BW::string& textureName )
{
	BW_GUARD;
	EcotypeParam* pMyParam = (EcotypeParam*)tree().GetItemData( hItem );
	MF_ASSERT( pMyParam );
	if (pMyParam->hDerivedVisualGeneratorItem_ == NULL)
	{
		return false;
	}
	VisualGeneratorSection* pVisualGenSection = 
						(VisualGeneratorSection*)tree().getItemSection( 
						pMyParam->hDerivedVisualGeneratorItem_ );
	return pVisualGenSection->getVisualTexture( 
						pMyParam->hDerivedVisualGeneratorItem_, textureName );
}


/**
 *	This method find the texture used by this ecotype,
 *	which is the first visual generator's texture.
 */
void EcotypeSection::updateVisualTexture( HTREEITEM hItem )
{
	BW_GUARD;
	EcotypeParam* pMyParam = (EcotypeParam*)tree().GetItemData( hItem );
	MF_ASSERT( pMyParam );


	VisualGeneratorSection *pVisualGenSection = 
					(VisualGeneratorSection*)tree().section( "visual", false );
	MF_ASSERT( pVisualGenSection );
	BW::vector<HTREEITEM> visualGeneratorItems;
	tree().findChildItemsBySection( hItem, pVisualGenSection, visualGeneratorItems );

	pMyParam->hDerivedVisualGeneratorItem_ = NULL;
	BW::string textureName = "";
	BW::vector<HTREEITEM>::iterator it = visualGeneratorItems.begin();
	for (; it != visualGeneratorItems.end(); ++it)
	{
		// find my derived item
		if (pVisualGenSection->getVisualTexture( *it, textureName ))
		{
			pMyParam->hDerivedVisualGeneratorItem_ = *it;
			break;
		}
	}

	if (textureName != "")
	{
		BW::vector<HTREEITEM>::iterator it = visualGeneratorItems.begin();
		for (; it != visualGeneratorItems.end(); ++it)
		{
			// Validate all the visual generator's textures.
			pVisualGenSection->validateTexture( *it, textureName );
		}
	}

}


/**
 *	This method create an item param for the item
 */
/*virtual*/ void EcotypeSection::createItemParam( HTREEITEM hItem )
{
	BW_GUARD;
	EcotypeParam* pParam = new EcotypeParam();
	pParam->pSection_ = this;
	tree().SetItemData( hItem, (DWORD_PTR)pParam );
}


/**
 *	This method check if the texture is used by other ecotype( up and down)
 *	if yes, then mark the non-first ones wrong.
 */
/*virtual*/ void EcotypeSection::postChildAdded( HTREEITEM hParentItem, 
								HTREEITEM hChildItem, bool moveOnly /*= false*/ )
{
	BW_GUARD;
	// Check if the texture is used by other ecotype( up and down).
	if (tree().getItemSection( hChildItem ) == tree().section( "texture", true ))
	{
		CString wTextureName = tree().GetItemText( hChildItem );
		if (!moveOnly)
		{
			char textureName[256];
			bw_wtoutf8( (LPCWSTR)wTextureName, textureName, 256 );
			// Check if the terrain texture is missing.
			if (!BWResource::fileExists( textureName ))
			{
				bool fileExist = false;
				TextureParam* pParam = (TextureParam*)tree().GetItemData( hChildItem );
				MF_ASSERT( pParam );

				const char** textureExts = Moo::TextureManager::validTextureExtensions();
				for (size_t i = 0; textureExts[i] != NULL; ++i)
				{
					BWResource::changeExtensionT( textureName, textureExts[i], false );
					if (BWResource::fileExists( textureName ))
					{
						pParam->setExtension( textureExts[i] );
						fileExist = true;
						break;
					}
				}
				if (!fileExist)
				{
					tree().markItemWrong( hChildItem, FILE_MISSING );
				}
			}
		}


		// Check if those ecotypes after me have same texture,
		// if yes, mark me wrong.
		tree().unmarkItemWrong( hChildItem, TERRAIN_USED_IN_OTHER );
		HTREEITEM hRepeatItem = 
						tree().findTextureItem( wTextureName, hParentItem, NULL );
		if (hRepeatItem != NULL)
		{
			// found repeat one, mark me wrong.
			tree().markItemWrong( hChildItem, TERRAIN_USED_IN_OTHER );
		}

		// Check if those ecotypes in front of me have same texture, mark them wrong.
		hRepeatItem = tree().findTextureItem( wTextureName, hParentItem, NULL, true );
		if (hRepeatItem != NULL)
		{
			// found repeat one, mark it wrong.
			tree().markItemWrong( hRepeatItem, TERRAIN_USED_IN_OTHER );
		}
	}
	

	ListFileProviderSection::postChildAdded( hParentItem, hChildItem, moveOnly );

}


/**
 *	This method check if the texture is used by other ecotype( up and down),
 *	and unmark the first one who was marked as wrong item.
 */
/*virtual*/ void EcotypeSection::preChildDeleted( HTREEITEM hParentItem, 
														HTREEITEM hChildItem )
{
	BW_GUARD;
	FloraSettingTreeItemParam* pParam = 
				(FloraSettingTreeItemParam*)(tree().GetItemData( hChildItem ));
	if (pParam->pSection_ == tree().section( "texture", true ))
	{
		// Only do it if I am not marked wrong otherwise there should be 
		// another one in front of me.
		if ((pParam->error_ & TERRAIN_USED_IN_OTHER) == 0)
		{
			
			CString wTextureName = tree().GetItemText( hChildItem );
			// Check if those ecotypes in front of me have same texture, unmark the last one.
			HTREEITEM hRepeatItem = 
				tree().findTextureItem( wTextureName, hParentItem, NULL, true );
			if (hRepeatItem != NULL)
			{
				// found repeat one, mark it back to correct.
				tree().unmarkItemWrong( hRepeatItem, TERRAIN_USED_IN_OTHER );
			}
		}
	}

}


/**
 *	This method is called by UI and tell the UI need display the visual texture.
 *	@return		true if we have visual texture to display.
 */
/*virtual*/ bool EcotypeSection::prepareCurDisplayImage( HTREEITEM hItem, 
								wchar_t* description, size_t szDescription, 
								BW::string& imagePath )
{
	BW_GUARD;

	EcotypeParam* pParam = (EcotypeParam*)tree().GetItemData( hItem );
	MF_ASSERT( pParam );
	if (pParam->hDerivedVisualGeneratorItem_)
	{
		VisualGeneratorSection* pVisualGenSection = 
			(VisualGeneratorSection*)tree().getItemSection( 
										pParam->hDerivedVisualGeneratorItem_ );
		return pVisualGenSection->prepareCurDisplayImage( 
			pParam->hDerivedVisualGeneratorItem_, description, 
			szDescription, imagePath );
	}
	return false;
}


//////////////////////FloraSettingTree//////////////////////////////////////////

BEGIN_MESSAGE_MAP(FloraSettingTree, CTreeCtrl)
	ON_NOTIFY_REFLECT( TVN_SELCHANGED, OnSelChanged )
	ON_NOTIFY_REFLECT( TVN_KEYDOWN, OnKeyDown )
	ON_NOTIFY_REFLECT( TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT( TVN_ENDLABELEDIT, OnEndLabelEdit )

END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(FloraSettingTree, CTreeCtrl)
FloraSettingTree::FloraSettingTree( PageFloraSetting* pPageWnd ):
pPageWnd_( pPageWnd )
{
	BW_GUARD;
}

FloraSettingTree::~FloraSettingTree()
{
	BW_GUARD;
}



typedef FloraSettingTreeSection* (*FloraSettingTreeSectionCreator)();
typedef BW::vector< std::pair< BW::string, FloraSettingTreeSectionCreator> > FloraSettingSectionCreators;
FloraSettingSectionCreators g_floraSettingSectionCreators;

#define SECTION_FACTORY_DECLARE( b ) FloraSettingTreeSection* b##Creator() \
{															 \
	return new b##Section;									 \
}

#define REGISTER_SECTION_FACTORY( a, b ) g_floraSettingSectionCreators.push_back( std::make_pair( a, b##Creator ) );


SECTION_FACTORY_DECLARE( FloraSettingTree );
SECTION_FACTORY_DECLARE( ListFileProvider );
SECTION_FACTORY_DECLARE( ListXmlProvider );
SECTION_FACTORY_DECLARE( Float );
SECTION_FACTORY_DECLARE( AddNewItem );
SECTION_FACTORY_DECLARE( UnEditable )
SECTION_FACTORY_DECLARE( Ecotype )
SECTION_FACTORY_DECLARE( VisualGenerator );
SECTION_FACTORY_DECLARE( Visual );
SECTION_FACTORY_DECLARE( Texture );
bool registerSections()
{
	BW_GUARD;
	REGISTER_SECTION_FACTORY( "default", FloraSettingTree );
	REGISTER_SECTION_FACTORY( "listFileProviderSection", ListFileProvider );
	REGISTER_SECTION_FACTORY( "listXmlProviderSection", ListXmlProvider );
	REGISTER_SECTION_FACTORY( "floatSection", Float );
	REGISTER_SECTION_FACTORY( "addNewSection", AddNewItem );
	REGISTER_SECTION_FACTORY( "uneditableSection", UnEditable );
	REGISTER_SECTION_FACTORY( "ecotypeSection", Ecotype );
	REGISTER_SECTION_FACTORY( "visualGeneratorSection", VisualGenerator );
	REGISTER_SECTION_FACTORY( "visualSection", Visual );
	REGISTER_SECTION_FACTORY( "textureSection", Texture );
	return true;
};
bool successful = registerSections();

void FloraSettingTree::init( DataSectionPtr pDataSection )
{
	BW_GUARD;
	imgList_.Create( IMAGE_SIZE, IMAGE_SIZE, ILC_COLOR24|ILC_MASK, 2, 32 );
	imgList_.SetBkColor( ( GetBkColor() == -1 ) ? 
									GetSysColor( COLOR_WINDOW ) : GetBkColor() );
	imgList_.Add( AfxGetApp()->LoadIcon( IDI_FLORA_SETTING_TREE_ITEM_ERROR ) );
	imgList_.Add( AfxGetApp()->LoadIcon( IDI_FLORA_SETTING_TREE_ITEM_ERROR_SEL ) );
	SetImageList( &imgList_, TVSIL_STATE );

	// init the sections from flora_setting_page.xml
	{
		FloraSettingTreeSection* pSection; 
		for (int i = 0; i < pDataSection->countChildren(); ++i)
		{
			DataSectionPtr pChild = pDataSection->openChild( i );
			BW::string providerName = pChild->readString( "provider" );
			FloraSettingSectionCreators::iterator it = 
										g_floraSettingSectionCreators.begin();
			for (; it != g_floraSettingSectionCreators.end(); ++it)
			{
				if (providerName == (*it).first)
				{
					FloraSettingTreeSectionCreator creator = (*it).second;
					pSection = creator();
					break;
				}
			}
			MF_ASSERT( pSection );

			pSection->init( pChild, pPageWnd_ );
			// hack if name by section value, we add "_value".
			char key[256];
			strcpy( key, pChild->sectionName().c_str());
			if (pChild->readBool( "by_section_name", true ) == false)
			{
				strcpy( &key[ pChild->sectionName().size() ], "_value");
			}
			sectionMap_[ key ] = pSection;
		}
	}

}


/**
 *	This MFC method is overriden to allow for additional application-specific
 *	handling of selection change eevents on the tree view.
 *
 *	@param pNotifyStruct	MFC notify struct.
 *	@param result			MFC result.
 */
/*afx_msg*/ void FloraSettingTree::OnSelChanged( NMHDR * pNotifyStruct,
																LRESULT* result )
{
	BW_GUARD;

	FloraSettingTreeSection* pSection;
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) pNotifyStruct; 
	// unselect the previous one
	HTREEITEM item = pnmtv->itemOld.hItem;
	if (item != NULL)
	{
		pSection = this->getItemSection( item );
		if (pSection != NULL)
		{
			pPageWnd_->secondaryWnd().onDeactive( item );
			if (pPageWnd_->toggle3DView())
			{
				this->highLightVisuals( item , false );
			}

		}
	}

	//select the new one
	item = pnmtv->itemNew.hItem;
	if (item != NULL)
	{
		pPageWnd_->secondaryWnd().onActive( item );
		if (pPageWnd_->toggle3DView())
		{
			this->highLightVisuals( item , true );
		}
	}
}


/**
 *	This MFC method is overriden to prevent the user from editing the name of
 *	the default system chain.
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, 0 to allow editing, 1 to cancel.
 */
/*afx_msg*/void FloraSettingTree::OnBeginLabelEdit(NMHDR* nmhdr, LRESULT* presult)
{
	BW_GUARD;
	LRESULT result = 1;//reject
	NMTVDISPINFO *dispInfo = (NMTVDISPINFO*)nmhdr;
	FloraSettingTreeSection* pSection = this->getItemSection( dispInfo->item.hItem );
	HTREEITEM hParentItem = this->GetParentItem( dispInfo->item.hItem );
	if( hParentItem )
	{
		FloraSettingTreeSection* pParentSection = this->getItemSection( hParentItem );
		result = (!pParentSection->isChildItemNameEditable( dispInfo->item.hItem ));
	}

	if (presult != NULL)
	{
		*presult = result;
	}
}


/**
 *	This MFC method is overriden to finish the user's renaming of a chain.
 *
 *	@param pNMHDR	MFC notify header struct.
 *	@param pResult	MFC result, 0.
 */
/*afx_msg*/void FloraSettingTree::OnEndLabelEdit(NMHDR* nmhdr, LRESULT* presult)
{
	BW_GUARD;

	NMTVDISPINFO *dispInfo = (NMTVDISPINFO*)nmhdr;

	if (dispInfo->item.pszText)
	{
		FloraSettingTreeSection* pSection = 
									this->getItemSection( dispInfo->item.hItem );
		HTREEITEM hParentItem = this->GetParentItem( dispInfo->item.hItem );
		MF_ASSERT( hParentItem );
		FloraSettingTreeSection* pParentSection = this->getItemSection( hParentItem );
		if (pParentSection->isChildItemNameEditable( dispInfo->item.hItem ))
		{
			wchar_t error[256];
			if (pParentSection->validEditedChildItemName( hParentItem,
					dispInfo->item.pszText, error, 256 ))
			{
				//add undo
				ChangeNameOperation* pOperation = new ChangeNameOperation( 
					this, dispInfo->item.hItem, 
					(LPCWSTR)this->GetItemText( dispInfo->item.hItem ) );

				UndoRedo::instance().add( pOperation );
				UndoRedo::instance().barrier( LocaliseUTF8( 
					L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/CHANGE_ITEM_NAME"), 
					false );

				this->SetItemText( dispInfo->item.hItem, dispInfo->item.pszText );
			}
			else
			{
				MsgBox mb( L"", error, 
					Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/YES") );
				mb.doModal( NULL );
			}
		}
	}

	if (presult != NULL)
	{
		*presult = 0;
	}
}



/**
 * This method is called in response to pressing a key in the weather system
 * list, and if the key is F2, it invokes the rename edit box.
 */
/*afx_msg*/ void FloraSettingTree::OnKeyDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	
	if (pTVKeyDown->wVKey == VK_DELETE)
	{
		HTREEITEM hItem = this->GetSelectedItem();
		this->deleteItem( hItem, true );
	}

	*pResult = 0;
}


/**
 *	This method populate the tree from the parentItem
 *	@param	pFloraXMLDataSect	The data section of flora.xml
 */
void FloraSettingTree::populate( DataSectionPtr pFloraXMLDataSect )
{
	BW_GUARD;

	FloraSettingTreeSection* pSection = 
									this->section( "uneditable_section", false );
	HTREEITEM rootItem = pSection->populate( pFloraXMLDataSect, NULL );
	this->Expand( rootItem, TVE_EXPAND );

	HTREEITEM hEcotypesItem = this->findChildItem( rootItem, L"ecotypes" );

	if ( hEcotypesItem != NULL)
	{
		this->Expand( hEcotypesItem, TVE_EXPAND );
		this->SelectItem( hEcotypesItem );
	}
	else
	{
		this->SelectItem( rootItem );
	}
}


/**
 *	This method save the tree into parentItem,
 *	@param	pFloraXMLDataSect	The data section of flora.xml
 *	@param	pHighLightVisuals	It's a hack to tell which visuals need save 
 *								<highlight>	just for WE to high light visual 
 *								in 3d scene
*/
void FloraSettingTree::save( DataSectionPtr pFloraXMLDataSect, 
							BW::vector<HTREEITEM>* pHighLightVisuals /*= NULL*/ )
{
	BW_GUARD;

	// Since only the sections inside <ecotypes> are editable, so we
	// skip outside <ecotypes>
	HTREEITEM hEcotypesItem = 
						this->findChildItem( this->GetRootItem(), L"ecotypes" );
	if (hEcotypesItem != NULL)
	{
		FloraSettingTreeSection* pSection = this->getItemSection( hEcotypesItem );
		MF_ASSERT( pSection );
		pSection->save( pFloraXMLDataSect, hEcotypesItem, NULL, pHighLightVisuals );
	}
}


/**
 *	This method create a FloraSettingTreeSection associated with name.
 *	@param	isSectionName	true if it's section's name, 
 *							otherwise it's the value(==asString())
 */
FloraSettingTreeSection* FloraSettingTree::section( const BW::string& name, 
															bool isSectionName )
{
	BW_GUARD;

	char key[256];
	strcpy( key, name.c_str());
	if (!isSectionName)
	{
		strcpy( &key[name.size()], "_value");
	}

	return sectionMap_[ key ];
}


FloraSettingTreeSection* FloraSettingTree::getItemSection( HTREEITEM hItem )
{
	BW_GUARD;
	FloraSettingTreeItemParam* pParam = 
						(FloraSettingTreeItemParam*)this->GetItemData( hItem );
	if (pParam)
	{
		return pParam->pSection_;
	}
	return NULL;
}


/**
 *	This static function is used as a SortChildrenCB callback
 *	@param lParam1		First item to compare.
 *	@param lParam2		The other item to compare.
 *	@param lParamSort	not used.
 *	@return				less than zero if lParam1 < lParam2, greater than zero if
 *						lParam1 > lParam2, zero if they are equal.
 */
static int CALLBACK FloraSettingOrderFunc( LPARAM lParam1, LPARAM lParam2, 
															LPARAM lParamSort )
{
	BW_GUARD;
	return (int)lParam1 - (int)lParam2;
}


/**
 *	This method return how many child item does hParentItem have
 */
size_t FloraSettingTree::childCount( HTREEITEM hParentItem )
{
	BW_GUARD;
	size_t idx = 0;
	HTREEITEM hChildItem =this->GetChildItem( hParentItem );
	while (hChildItem != NULL)
	{
		++idx;
		hChildItem =this->GetNextItem( hChildItem, TVGN_NEXT );
	}
	return idx;
}

/**
 *	This method move up the hItem one index
 *	@param	hItem		the item to be moved.
 *	@return				true if succeed.
 */
bool FloraSettingTree::moveItem( HTREEITEM hItem, int step, bool addUndo /*= true*/ )
{
	BW_GUARD;
	MF_ASSERT( hItem != NULL );

	HTREEITEM hParentItem = this->GetParentItem( hItem );
	if (hParentItem != NULL)
	{
		pPageWnd_->preChange();
		size_t oldSiblingIndex;
		if (addUndo)
		{
			oldSiblingIndex = this->getSiblingIndex( hItem );
		}

		FloraSettingTreeSection *pParentSection = 
											this->getItemSection( hParentItem );
		pParentSection->preChildDeleted( hParentItem, hItem );

		// Add child items into a list
		BW::list< HTREEITEM > childItems;
		int idx = 0;
		int myIdx = -1;
		HTREEITEM hChildItem =this->GetChildItem( hParentItem );
		while (hChildItem != NULL)
		{
			if (hChildItem == hItem)
			{
				myIdx = idx;
			}
			else
			{
				childItems.push_back( hChildItem );
			}			
			idx++;
			hChildItem =this->GetNextItem( hChildItem, TVGN_NEXT );
		}
		MF_ASSERT( myIdx != -1 );
		int totalChild = idx;
		myIdx = (myIdx + step + totalChild) % totalChild;

		// Put hItem in correct place.
		idx = 0;
		BW::list< HTREEITEM >::iterator it = childItems.begin();
		for (; it != childItems.end(); ++it, ++idx )
		{
			if (idx == myIdx)
			{
				childItems.insert( it, hItem );
				break;
			}
		}

		// Save the old params
		MF_ASSERT( totalChild <= 512 );// if exceed we need grow the size.
		DWORD_PTR params[ 512 ];
		idx = 0;
		it = childItems.begin();
		for (; it != childItems.end(); ++it )
		{
			params[ idx ] = this->GetItemData( *it );
			this->SetItemData( *it , idx++ );
		}

		// Sort
		TVSORTCB sortCB;
		sortCB.hParent = hParentItem;
		sortCB.lpfnCompare = FloraSettingOrderFunc;
		sortCB.lParam = NULL;
		this->SortChildrenCB( &sortCB );

		// Restore params.
		idx = 0;
		it = childItems.begin();
		for (; it != childItems.end(); ++it )
		{
			this->SetItemData( *it , params[ idx++ ] );
		}

		pParentSection->postChildAdded( hParentItem, hItem, true );
		pPageWnd_->postChange();

		if (addUndo)
		{
			MoveItemUpDownOperation* pOperation = new MoveItemUpDownOperation( 
				this, hItem, oldSiblingIndex );
			UndoRedo::instance().add( pOperation );
			UndoRedo::instance().barrier( LocaliseUTF8( 
				L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/MOVE_ITEM"), 
				false );
		}

		return true;
	}
	return false;
}


/**
 *	Create an new ecotype.
 */
bool FloraSettingTree::createNewEcotype()
{
	BW_GUARD;
	pPageWnd_->preChange();

	HTREEITEM ecotypesItem = this->findChildItem( 
								this->GetRootItem(), L"ecotypes" );
	if (ecotypesItem == NULL)
	{
		return false;
	}
	AddNewItemSection* pEcotypesSection = 
		dynamic_cast<AddNewItemSection*>(this->getItemSection( ecotypesItem ));
	MF_ASSERT( pEcotypesSection );
	HTREEITEM hNewItem = pEcotypesSection->newDefaultChildItem( ecotypesItem );
	if (hNewItem)
	{
		pPageWnd_->postChange();
	}
	return hNewItem != NULL;
}


/**
 *	This method try to find a texture tree item.
 *	@param	hStartEcotypeItem		search starts from this item(exclusive).
 *	@param	hEndEcotypeItem			search ends at this item(exclusive).
 *	@param	textureName				the name of the tree view item as a texture item
 */
HTREEITEM FloraSettingTree::findTextureItem( const wchar_t* wTextureName, 
							HTREEITEM hStartEcotypeItem /*= NULL*/, 
							HTREEITEM hEndEcotypeItem /*= NULL*/, 
							bool reverseSearch /*= false*/ )
{
	BW_GUARD;
	HTREEITEM hEcotypesItem = this->findChildItem( this->GetRootItem(), L"ecotypes" );
	if (!hEcotypesItem)
	{
		return NULL;
	}

	if (hStartEcotypeItem == NULL)
	{
		hStartEcotypeItem = this->GetChildItem( hEcotypesItem );
		if (reverseSearch)
		{
			// start from the last one
			HTREEITEM hNext = this->GetNextItem( hStartEcotypeItem, TVGN_NEXT );
			while (hNext != NULL)
			{
				hStartEcotypeItem = hNext;
				hNext = this->GetNextItem( hStartEcotypeItem, TVGN_NEXT );
			}
		}
	}
	else
	{
		//exclude the start one.
		hStartEcotypeItem = this->GetNextItem( hStartEcotypeItem, 
			reverseSearch ? TVGN_PREVIOUS : TVGN_NEXT );
		// No brother items.
		if (hStartEcotypeItem == NULL)
		{
			return NULL;
		}
	}

	HTREEITEM hItem = hStartEcotypeItem;
	while (hItem != hEndEcotypeItem)
	{
		HTREEITEM textureItem = this->findChildItem( hItem, wTextureName );
		if (textureItem != NULL)
		{
			return textureItem;
		}
		hItem = this->GetNextItem( hItem, reverseSearch ? 
													TVGN_PREVIOUS : TVGN_NEXT );
	}
	return NULL;
}


/**
 *	This method check if hItem has a child item
 *	whose name is equal to itemName
 *	@param	hItem		the item to check if it has child 
 *						whose name is equal to itemName
 *	@param	itemName	the item name  to compare with
 *	@return				found hItem, or NULL if not found.
 */
HTREEITEM FloraSettingTree::findChildItem( HTREEITEM hItem, 
														const wchar_t* itemName)
{
	BW_GUARD;
	FloraSettingTreeSection* pTextureSection = this->section( "texture", true );
	MF_ASSERT( pTextureSection );

	size_t lenCompared, lenComparedNoExt;
	lenCompared = wcslen( itemName );
	bw_str_find_last_of( itemName, L".", lenComparedNoExt );


	HTREEITEM hNextItem;
	HTREEITEM hChildItem = this->GetChildItem( hItem );
	while (hChildItem != NULL)
	{
		bool different = false;
		wchar_t text[256];
		bw_snwprintf( text, 256, this->GetItemText( hChildItem ) );
		size_t len = wcslen( text );
		if (this->getItemSection( hChildItem ) == pTextureSection)
		{
			// we allow all texture format.
			bw_str_find_last_of( text, L".", len );
			if (lenComparedNoExt != len)
			{
				different = true;
			}
		}
		else if (lenCompared != len)
		{
			different = true;
		}
		if (!different && bw_str_equal( text, itemName, len ))
		{
			return hChildItem;
		}

		hNextItem = this->GetNextItem( hChildItem, TVGN_NEXT );
		hChildItem = hNextItem;
	}
	return NULL;
}



/**
*	This method tries to get the index path from root to hItem
*	@param indexPath	out, return the index path
*	@param len			in,out, length of indexPath
*	@return				found item
*/
HTREEITEM FloraSettingTree::findItem( size_t* indexPath, size_t len )
{
	BW_GUARD;
	HTREEITEM hItem;
	HTREEITEM hParentItem = this->GetRootItem();
	for (size_t i = 0; i < len; ++i)
	{
		size_t pos = indexPath[i];
		 hItem = this->GetChildItem( hParentItem );
		for (size_t idx = 0; idx < pos && hItem; ++idx)
		{
			hItem = this->GetNextItem( hItem, TVGN_NEXT );
		}
		if (!hItem)
		{
			return NULL;
		}
		hParentItem = hItem;
	}
	MF_ASSERT( hItem );
	return hItem;
}


size_t FloraSettingTree::getSiblingIndex( HTREEITEM hItem )
{
	BW_GUARD;
	HTREEITEM hParentItem = this->GetParentItem( hItem );
	size_t idx = 0;
	HTREEITEM hSiblingItem = this->GetChildItem( hParentItem );
	while (hSiblingItem != NULL && hSiblingItem != hItem)
	{
		++idx;
		hSiblingItem = this->GetNextItem( hSiblingItem, TVGN_NEXT );
	}
	return idx;
}


/**
*	This method tries to get the index path from root to hItem
*	@param hItem		the item that we want to get the path for
*	@param indexPath	out, return the index path
*	@param len			in,out, length of indexPath
*	@return				true if succeed.
*/
bool FloraSettingTree::getIndexPath( HTREEITEM hItem, size_t* indexPath, size_t& len )
{
	BW_GUARD;
	const size_t MAX = 256;
	size_t indexPathReverse[MAX];
	size_t cur = 0;
	HTREEITEM hParentItem = this->GetParentItem( hItem );
	while (hParentItem != NULL)
	{
		indexPathReverse[ cur++ ] = this->getSiblingIndex( hItem );

		hItem = hParentItem;
		hParentItem = this->GetParentItem( hParentItem );
	}

	if (len >= cur)
	{
		len = cur;
		//reverse the order
		for (size_t i = 0; i < len; ++i)
		{
			indexPath[i] = indexPathReverse[ len - 1 - i ];
		}
		return true;
	}
	return false;
}


bool FloraSettingTree::getHighlightVisualIndexPath( HTREEITEM hItem,
													BW::list<int>& indexPath )
{
	BW_GUARD;
	indexPath.clear();
	HTREEITEM hParentItem = this->GetParentItem( hItem );
	while( hParentItem != NULL)
	{
		FloraSettingTreeSection* pSection = this->getItemSection( hItem );
		MF_ASSERT( pSection );
		bool indexPathDependent = false;
		if (pSection->isEcotypeVisualDependent( true, &indexPathDependent ) &&
			indexPathDependent)
		{
			// Find index in brothers.
			int idx = 0;
			HTREEITEM hBrotherItem = this->GetChildItem( hParentItem );
			while( hBrotherItem != NULL && hBrotherItem != hItem )
			{
				pSection = this->getItemSection( hBrotherItem );
				MF_ASSERT( pSection );
				indexPathDependent = false;
				if (pSection->isEcotypeVisualDependent( true, &indexPathDependent ) 
					&& indexPathDependent)
				{
					idx++;
				}
				hBrotherItem = this->GetNextItem(hBrotherItem, TVGN_NEXT );
			}

			indexPath.push_front( idx );
		}

		hItem = hParentItem;
		hParentItem = this->GetParentItem( hParentItem );
	}
	return indexPath.size() > 0;
}



/**
 *	This method try to high light or cancel high light the visuals under the hItem
 */
void FloraSettingTree::highLightVisuals( HTREEITEM hItem, bool highLight )
{
	BW_GUARD;
	if (hItem != NULL)
	{
		FloraSettingTreeSection* pSecton = this->getItemSection( hItem );
		MF_ASSERT( pSecton );
		if (pSecton->isEcotypeVisualDependent( true ))
		{
			BW::list<int> indexPath;
			this->getHighlightVisualIndexPath( hItem, indexPath );
			if (indexPath.size() > 0)
			{
				ChunkManager::instance().cameraSpace()->enviro().flora()->highLight( indexPath, highLight );
				pPageWnd_->refreshFloraXMLDataSection( highLight? hItem: NULL, false );
			}
		}
	}
}


/**
 *	This method find the first parent item who is  
 *	associated to the  pSection.
 *	@param	hStartItem		the item start to search
 *	@param	pSection		the Section that the found item should be associated to.
 *	@return					the found item or NULL
 */
HTREEITEM FloraSettingTree::findFirstParentBySection( HTREEITEM hStartItem,
											FloraSettingTreeSection* pSection )
{
	BW_GUARD;

	MF_ASSERT(pSection);

	HTREEITEM hParentItem = this->GetParentItem( hStartItem );
	while (hParentItem != NULL)
	{
		if (this->getItemSection( hParentItem ) == pSection)
		{
			return hParentItem;
		}

		hParentItem = this->GetParentItem( hParentItem );
	}

	return NULL;
}


/**
 *	This method find all the child items in the tree who are  
 *	associated to the  pSection.
 *	@param	hStartItem		the item start to search
 *	@param	pSection		the Section that the found item should be associated to.
 *	@param	retItems		the list of items found
 */
void FloraSettingTree::findChildItemsBySection( HTREEITEM hStartItem,
								 FloraSettingTreeSection* pSection, 
								 BW::vector<HTREEITEM>& retItems,
								 bool lookForChildOfFoundItem /*= false*/)
{
	BW_GUARD;
	
	MF_ASSERT(pSection);
	HTREEITEM hChildItem = this->GetChildItem( hStartItem );

	while (hChildItem != NULL)
	{
		bool lookForChild = true;
		if (this->getItemSection( hChildItem ) == pSection)
		{
			retItems.push_back( hChildItem );
			if (!lookForChildOfFoundItem)
			{
				lookForChild = false;
			}
		}

		if (lookForChild)
		{
			//pass to child of hChildItem.
			this->findChildItemsBySection( hChildItem, pSection, retItems );
		}

		hChildItem = this->GetNextItem( hChildItem, TVGN_NEXT );
	}

}


/**
 *	This method expand hItem and if recursive is true
 *	it expand all the sub tree under hItem.
 */
void FloraSettingTree::expand( HTREEITEM hItem, bool recursive/* = false*/ )
{
	BW_GUARD;
	this->Expand( hItem, TVE_EXPAND );
	if (recursive)
	{
		HTREEITEM hChildItem = this->GetChildItem( hItem );
		while (hChildItem != NULL)
		{
			this->expand( hChildItem, recursive );
			hChildItem = this->GetNextItem( hChildItem, TVGN_NEXT );
		}
	}
}


/**
 *	This method delete an item with popping up a yes/no dialog
 *	@param	hItem			the item to be deleted.
 *	@param	popupAskDlg		if true, we pop up a dialog to ask the user yes or no.
 *	@return					if deleted or not
 */
bool FloraSettingTree::deleteItem( HTREEITEM hItem, bool popupAskDlg/*= false*/,
								  bool addUndo /*= true*/ )
{
	BW_GUARD;
	FloraSettingTreeSection* pMySection = this->getItemSection( hItem );
	MF_ASSERT( pMySection );
	if (!pMySection->isDeletable( hItem ))
	{
		MsgBox mb( Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM_TITLE"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM_CANNOT"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/YES") );
		mb.doModal( NULL );
		return false;
	}

	// Ensure with user
	if (popupAskDlg)
	{
		MsgBox mb( Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM_TITLE"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM_ENSURE"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/YES"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/NO") );
		if( mb.doModal( NULL ) == 1 )
		{
			return false;
		}
	}
	pPageWnd_->preChange();


	HTREEITEM hParentItem = this->GetParentItem( hItem );
	FloraSettingTreeSection* pParentSection = this->getItemSection( hParentItem );
	// Notify the parent item before deletion
	if (pParentSection)
	{
		pParentSection->preChildDeleted( hParentItem, hItem );
	}

	if (addUndo)
	{
		// Prepare for undo
		ItemExistenceOperation* pOperation = new ItemExistenceOperation( 
			true, this, hItem );
		UndoRedo::instance().add( pOperation );
		UndoRedo::instance().barrier( LocaliseUTF8( 
			L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/DELETE_ITEM"), 
			false );
	}
	pMySection->deleteItemParam( hItem );	
	this->DeleteItem( hItem );

	// Notify the parent item after deletion.
	if (pParentSection)
	{
		pParentSection->postChildDeleted( hParentItem, pMySection );
		this->Expand( hParentItem, TVE_EXPAND );
	}

	pPageWnd_->postChange();

	return true;
}


/**
 *	This method mark the item something wrong.
 *	@param	hItem		the item to marked.
 *	@param	error		the bit mask for error.
 */
void FloraSettingTree::markItemWrong( HTREEITEM hItem, 
							FloraSettingTreeSection::FLORA_SETTING_ERROR error )
{
	BW_GUARD;
	FloraSettingTreeItemParam* pParam = 
						(FloraSettingTreeItemParam*)this->GetItemData( hItem );
	pParam->error_ |= error;

	this->SetItemState( hItem, INDEXTOSTATEIMAGEMASK( TREE_ITEM_HIGHLIGHT ), 
														TVIS_STATEIMAGEMASK );
	HTREEITEM hParentItem = this->GetParentItem( hItem );
	if (hParentItem)
	{
		this->Expand( hParentItem, TVE_EXPAND );
	}
}


/**
 *	This method unmark the item who was marked wrong.
 *	@param	hItem		the item to marked.
 *	@param	error		the bit mask for error.
 */
void FloraSettingTree::unmarkItemWrong( HTREEITEM hItem,
							FloraSettingTreeSection::FLORA_SETTING_ERROR error )
{
	BW_GUARD;
	FloraSettingTreeItemParam* pParam = 
						(FloraSettingTreeItemParam*)this->GetItemData( hItem );
	bool wasWrong = (pParam->error_ != NO_ERROR);
	pParam->error_ &= ~error;
	if (pParam->error_ == NO_ERROR)
	{
		this->SetItemState( hItem, 0, TVIS_STATEIMAGEMASK );
		if (wasWrong)// if become correct, expand to the user for attention.
		{
			HTREEITEM hParentItem = this->GetParentItem( hItem );
			if (hParentItem)
			{
				this->Expand( hParentItem, TVE_EXPAND );
			}
		}
	}
}


ValidTextureFilterSpec::ValidTextureFilterSpec( const BW::wstring& name, 
													bool active/* = false*/ ):
	FilterSpec( name, active )
{
	BW_GUARD;
	validTextureName_[0] = 0;
}


/**
 *	This method set the validTextureName_, and return true if 
 *	validTextureName_ has been changed.
 *
 *	@param textureName	new validTexture_
 *	@return				True if the validTexture_ has been changed.
 */
bool ValidTextureFilterSpec::setTextureName( const BW::string& textureName)
{
	BW_GUARD;
	char textureNameWithoutExt[256];
	bw_snprintf( textureNameWithoutExt, 256, textureName.c_str() );
	BWResource::removeExtensionT( textureNameWithoutExt );
	if (bw_str_equal( validTextureName_, textureNameWithoutExt, 
		strlen( textureNameWithoutExt ) + 1 ))
	{
		return false;
	}

	if (Moo::TextureManager::instance()->isTextureFile( textureName ))
	{
		strcpy( validTextureName_, textureNameWithoutExt );
		return true;
	}

	return false;
}


/**
 *	This method tests the input string against the filter, and returns true if
 *	there's a match.
 *
 *	@param str	String to match.
 *	@return		True if the string matches the filter, false otherwise.
 */
bool ValidTextureFilterSpec::filter( const BW::wstring& str )
{
	BW_GUARD;
	if ( !active_ || !enabled_ )
	{
		return true;
	}

	// pass filter test if the visual has texture same as textureName_
	BW::string textureName;
	if (VisualsEcotype::findTextureName( bw_wtoutf8( str ), textureName ) &&
		Moo::TextureManager::instance()->isTextureFile( textureName ))
	{
		char stripped[256];
		bw_snprintf( stripped, 256, textureName.c_str() );
		BWResource::removeExtensionT( stripped );
		return bw_str_equal( stripped, validTextureName_, strlen( stripped ) );
	}
	return false;
}


TreeItemOperation::TreeItemOperation( FloraSettingTree* pTree, HTREEITEM hItem ):
UndoRedo::Operation( 0 ),
	pTree_( pTree )
{
	BW_GUARD;

	// Get index path.
	indexPathLen_ = MAX_INDEX_PATH;
	if (!pTree_->getIndexPath( hItem, indexPath_, indexPathLen_ ))
	{
		CRITICAL_MSG( "TreeItemOperation::TreeItemOperation, "
											"index path is too long\n" );
		//Need increase MAX_INDEX_PATH
	}
}


/**
 *	This method find the parent item according to indexPath_
 *	@return		the parent item or NULL if not found
 */
HTREEITEM TreeItemOperation::parentItem()
{
	BW_GUARD;
	return pTree_->findItem( indexPath_, indexPathLen_ - 1 );
}


/**
 *	This method find the operation item according to indexPath_~
 *	@return		the operation item or NULL if not found
 */
HTREEITEM TreeItemOperation::thisItem()
{
	BW_GUARD;
	return pTree_->findItem( indexPath_, indexPathLen_ );
}


void ItemExistenceOperation::undo()
{
	BW_GUARD;
	if (deletion_)
	{
		//Insert back
		HTREEITEM hParentItem = this->parentItem();
		MF_ASSERT( hParentItem );
		//FloraSettingTreeSection* pSection = 
		//				pTree_->section( sectionName_, nameByDataSectionName_);
		MF_ASSERT( pData_ && pSection_ );
		HTREEITEM hInsertItem = pSection_->populate( pData_, hParentItem );
		MF_ASSERT( hInsertItem );
		int curIndex = static_cast<int>(pTree_->childCount( hParentItem ) - 1);
		if (curIndex > 0)
		{
			pTree_->moveItem( hInsertItem, 
				static_cast<int>(indexPath_[indexPathLen_ - 1] - curIndex), false );
		}
		pTree_->SelectItem( hInsertItem );
	}
	else //Insert
	{
		//Delete the item.
		HTREEITEM hItem = this->thisItem();
		MF_ASSERT( hItem );
		pTree_->deleteItem( hItem, false, false );
	}
	ItemExistenceOperation* pOperation = new ItemExistenceOperation( *this );
	pOperation->deletion_ = !deletion_;
	UndoRedo::instance().add( pOperation );
}


ItemExistenceOperation::ItemExistenceOperation( bool deletion, 
								FloraSettingTree* pTree, HTREEITEM hItem ):
	TreeItemOperation( pTree, hItem ),
	deletion_( deletion )
{

	BW_GUARD;
	HTREEITEM hParentItem = pTree_->GetParentItem( hItem );
	if (!hParentItem)
	{
		MF_ASSERT( hParentItem );// shouldn't happen
		return;
	}
	FloraSettingTreeSection* pParentSection = pTree_->getItemSection( hParentItem );
	MF_ASSERT( pParentSection );
	FloraSettingTreeSection* pSection = pTree_->getItemSection( hItem );
	MF_ASSERT( pSection );

	// Save sub tree as data section.
	pData_ = pSection->save( NULL, hItem, pParentSection );

	pSection_ = pSection;
}


MoveItemUpDownOperation::MoveItemUpDownOperation( FloraSettingTree* pTree, 
										HTREEITEM hItem, size_t oldLeaveIndex ):
	TreeItemOperation( pTree, hItem ),
	oldLeaveIndex_( oldLeaveIndex )
{
	BW_GUARD;
}


void MoveItemUpDownOperation::undo()
{
	BW_GUARD;
	HTREEITEM hItem = this->thisItem();
	MF_ASSERT( hItem );
	HTREEITEM hParentItem = pTree_->GetParentItem( hItem );
	MF_ASSERT( hParentItem );
	size_t curSiblingIndex = pTree_->getSiblingIndex( hItem );
	int step = static_cast<int>(oldLeaveIndex_ - curSiblingIndex);
	pTree_->moveItem( hItem, step, false );
	MoveItemUpDownOperation* pOperation = new MoveItemUpDownOperation( *this );
	pOperation->indexPath_[ pOperation->indexPathLen_ - 1 ] += step;
	pOperation->oldLeaveIndex_ = curSiblingIndex;
	UndoRedo::instance().add( pOperation );
}


FloatValueOperation::FloatValueOperation( FloraSettingTree* pTree, 
		HTREEITEM hItem, float oldValue, FloraSettingFloatBarWnd* pFloatBarWnd ):
	TreeItemOperation( pTree, hItem ),
	oldValue_( oldValue ),
	pFloatBarWnd_( pFloatBarWnd )
{
	BW_GUARD;
}


void FloatValueOperation::undo()
{
	BW_GUARD;
	HTREEITEM hItem = this->thisItem();
	MF_ASSERT( hItem );
	FloatSection* pSection = 
					dynamic_cast<FloatSection*>(pTree_->getItemSection( hItem ));
	MF_ASSERT( pSection );
	wchar_t name[256];
	float curValue;
	pSection->getNameAndValue( hItem, name, ARRAY_SIZE(name), curValue );
	pSection->setNameAndValue( hItem, pSection->name(), oldValue_);
	if (pFloatBarWnd_->item() == hItem)
	{
		pFloatBarWnd_->onActive( hItem );
	}
	// Add undo
	FloatValueOperation* pOperation = new FloatValueOperation( *this );
	pOperation->oldValue_ = curValue;
	UndoRedo::instance().add( pOperation );
}



ChangeNameOperation::ChangeNameOperation( FloraSettingTree* pTree, 
										 HTREEITEM hItem, LPCWSTR pOldName ):
TreeItemOperation( pTree, hItem )
{
	BW_GUARD;
	bw_str_copy( oldName_, AddNewItemSection::NAME_LENGTH, pOldName );
}


void ChangeNameOperation::undo()
{
	BW_GUARD;
	HTREEITEM hItem = this->thisItem();
	MF_ASSERT( hItem );

	// Add undo
	ChangeNameOperation* pOperation = new ChangeNameOperation( *this );
	bw_str_copy( pOperation->oldName_, AddNewItemSection::NAME_LENGTH, 
		(LPCWSTR)pTree_->GetItemText( hItem ) );
	UndoRedo::instance().add( pOperation );

	pTree_->SetItemText( hItem, oldName_ );
}

BW_END_NAMESPACE

