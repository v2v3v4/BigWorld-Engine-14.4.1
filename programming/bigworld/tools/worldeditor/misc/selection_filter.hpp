#ifndef SELECTION_FILTER_HPP
#define SELECTION_FILTER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

class SelectionFilter
{
public:
	enum SelectMode { SELECT_ANY, SELECT_SHELLS, SELECT_NOSHELLS };
private:
	static BW::vector<BW::string> typeFilters_;
	static BW::vector<BW::string> noSelectTypeFilters_;
	static BW::string filterByName_;

	static SelectMode selectMode_;
public:
	/**
	 * Return true if the given item is selectable.
	 *
	 * ignoreCurrentSelection: Don't let the currently selected items affect 
	 * the selectability of the given item.
	 *
	 * ignoreCameraChunk: Don't allow the shell model for the chunk the camera
	 * is in to be selected.
	 *
	 * ignoreVisibility: Don't use the visibility check.
	 *
	 * ignoreFrozen: Don't use the frozen check.
	 *	 
	 */
	static bool canSelect( ChunkItem* item,
		bool ignoreCurrentSelection = false, bool ignoreCameraChunk = true,
		bool ignoreVisibility = false, bool ignoreFrozen = false );

	/**
	 * Set the typeFilters_ field to filters, which is a
	 * "|" seperated list of filters
	 */
	static void typeFilters( BW::string filters );
	static BW::string typeFilters();

	/**
	 * As typeFilters_, but these will never be selected
	 */
	static void noSelectTypeFilters( BW::string filters );
	static BW::string noSelectTypeFilters();

	/**
	 * filter by name support
	 */
	static void setFilterByName( BW::string& name );

	static void setSelectMode( SelectMode selectMode )	{ selectMode_ = selectMode; }
	static SelectMode getSelectMode()
	{
		return selectMode_;
	}
};

BW_END_NAMESPACE

#endif // SELECTION_FILTER_HPP
