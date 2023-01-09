#ifndef SCENE_BROWSER_HPP
#define SCENE_BROWSER_HPP


#include "cstdmf/bw_functor.hpp"
#include "cstdmf/singleton.hpp"
#include "world/item_info_db.hpp"

BW_BEGIN_NAMESPACE

static const char * SCENE_BROWSER_CONFIG =
									"resources/scene_browser/sb_config.xml";
static const char * SCENE_BROWSER_PRESETS =
									"resources/scene_browser/sb_presets.xml";

class SceneBrowserDlg;
class ChunkItem;
typedef SmartPointer< ChunkItem > ChunkItemPtr;

namespace GUITABS
{
	class Content;
}


/**
 *	This class serves as a layer between the app and the dialog, allowing for
 *	some initialisation tasks to be done independently of the dialog's creation
 *	and destruction.
 */
class SceneBrowser : public Singleton< SceneBrowser >
{
public:
	typedef BW::list< ItemInfoDB::ItemPtr > ItemList;
	typedef BWBaseFunctor1< const ItemList & > SelChangeFunctor;
	typedef BWBaseFunctor0R< const BW::vector< ChunkItemPtr > & >
															CurrentSelFunctor;

	SceneBrowser();


	/**
	 *	This method sets the app callback to be called when the user changes
	 *	the selection in the Scene Browser UI.
	 *	The callback receives the new selection and a boolean that tells
	 *	whether or not the change was caused by user interaction.
	 *
	 *	@param functor	App callback for when the user changes the selection.
	 */
	void callbackSelChange( SelChangeFunctor * functor )
	{
		cbSelChange_ = functor;
	}
	
	/**
	 *	This method sets the app callback to be used by the Scene Browser to
	 *	know the current selection in the application.
	 *	The callback receives a reference to a vector for returning the current
	 *	selection to the Scene Browser.
	 *
	 *	@param functor	Application callback to query the current selection.
	 */
	void callbackCurrentSel( CurrentSelFunctor * functor )
	{
		cbCurrentSel_ = functor;
	}
	
	void scrollTo( ChunkItemPtr pItem ) const;

	bool isFocused() const;

	bool currentItems( BW::vector< ChunkItemPtr > & chunkItems ) const;

private:
	friend SceneBrowserDlg;

	ItemInfoDB itemInfoDbHolder_;

	SmartPointer< SelChangeFunctor > cbSelChange_;
	SmartPointer< CurrentSelFunctor > cbCurrentSel_;


	// Method called from the dialog to get the Selection Change app callback.
	SelChangeFunctor * callbackSelChange()
	{
		return cbSelChange_.get();
	}

	// Method called from the dialog to get the current selection from the app.
	CurrentSelFunctor * callbackCurrentSel()
	{
		return cbCurrentSel_.get();
	}

	bool isFocusedInternal( GUITABS::Content * cnt ) const;

};

BW_END_NAMESPACE

#endif // SCENE_BROWSER_HPP
