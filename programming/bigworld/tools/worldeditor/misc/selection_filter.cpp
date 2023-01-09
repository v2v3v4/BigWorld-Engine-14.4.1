#include "pch.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_space.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"

BW_BEGIN_NAMESPACE

BW::vector<BW::string> SelectionFilter::typeFilters_;
BW::vector<BW::string> SelectionFilter::noSelectTypeFilters_;
SelectionFilter::SelectMode SelectionFilter::selectMode_ = SELECT_ANY;

BW::string SelectionFilter::filterByName_;

namespace
{
	class VisibilityCollision : public CollisionCallback
	{
	public:
		VisibilityCollision() : gotone_( false ) { }

		int operator()( const CollisionObstacle & co,
			const WorldTriangle & hitTriangle, float dist )
		{
			// if it's not transparent, we can stop now
			if (!hitTriangle.isTransparent()) { gotone_ = true; return 0; }

			// otherwise we have to keep on going
			return COLLIDE_ALL;
		}

		bool gotone()			{ return gotone_; }
	private:
		bool gotone_;
	};

	bool isVisibleFrom( Vector3 vertex, const BoundingBox& bb )
	{
		BW_GUARD;

		// check the vertex is visible from the light
		VisibilityCollision vc;
		float x[] = { bb.minBounds().x, bb.maxBounds().x };
		float y[] = { bb.minBounds().y, bb.maxBounds().y };
		float z[] = { bb.minBounds().z, bb.maxBounds().z };

		for( int xx = 0; xx < 2; ++xx )
			for( int yy = 0; yy < 2; ++yy )
				for( int zz = 0; zz < 2; ++zz )
				{
					Vector3 v3( x[ xx ], y[ yy ], z[ zz ] );
					ChunkManager::instance().cameraSpace()->collide( vertex,
						v3, vc );
					if( !vc.gotone() )
						return true;
				}
		return false;
	}
}

bool SelectionFilter::canSelect( ChunkItem* item,
								bool ignoreCurrentSelection /*=false*/,
								bool ignoreCameraChunk /*=true*/,
								bool ignoreVisibility /*=false*/,
								bool ignoreFrozen /*=false*/ )
{
	BW_GUARD;

	if (!item->edSelectable())
	{
		return false;
	}

	// protect vs. deleted items
	if (!item->chunk())
		return false;

	// first off, filter out named items if name filter is non-empty
	if (filterByName_.size() > 0)
	{
		BW::string path = item->edFilePath();
		if (path.size() > 0)
			return (strstr(strlwr( &path[0 ]), &filterByName_[0]) != NULL);
		else
			return false;
	}

	bool isShellModel = item->isShellModel(); 

	if (selectMode_ == SELECT_SHELLS && !isShellModel)
		return false;

	if (selectMode_ == SELECT_NOSHELLS && isShellModel)
		return false;

	// check the frozen filter.
	bool frozenFilter = (std::find( typeFilters_.begin(), typeFilters_.end(), "frozen" ) != typeFilters_.end());
	if (item->edFrozen() && frozenFilter)
	{
		return true;
	}

	// only check the 'editability' if asked.
	if (!ignoreFrozen)
	{
		if ( (item->edFrozen() && !frozenFilter) || 
			(!item->edFrozen() && !item->edIsEditable()) ) 
		{
			return false;
		}
	}
	
	bool hiddenFilter = std::find( typeFilters_.begin(), typeFilters_.end(), "hidden" ) != typeFilters_.end();
	if (item->edHidden() && hiddenFilter)
	{
		return true;
	}
	
	if (!ignoreVisibility)
	{
		if (item->edHidden())
		{
			if (!hiddenFilter)
				return false;
		}
		else
		{
			// Don't select invisible items
			if (!item->edShouldDraw())
				return false;
		}
	}

	if (isShellModel)
	{
		// Don't select a shell if the camera is in the shell
		if (ignoreCameraChunk && ChunkManager::instance().cameraChunk() == item->chunk())
			return false;

		// Don't select a shell if an item in it is already selected
		if (!ignoreCurrentSelection && WorldManager::instance().isChunkSelectable( item->chunk() ))
			return false;
	}
	else
	{
		// Don't select an item in the shell if the shell is selected
		if (!ignoreCurrentSelection && WorldManager::instance().isChunkSelected( item->chunk() ))
			return false;
	}


	if (typeFilters_.empty() && noSelectTypeFilters_.empty())
        return true;

	DataSectionPtr ds = item->pOwnSect();
	if (!ds)
	{
		if (typeFilters_.empty())
			return true;
		else
		{
			// ChunkLink hasn't an own section
			if( item->edClassName() == "ChunkLink" )
				return std::find( typeFilters_.begin(), typeFilters_.end(), "station" ) != typeFilters_.end();
			return false;
		}
	}

	BW::string type = ds->sectionName();
	if (type == "vlo")
		type = ds->readString("type", "");

	if (std::find( noSelectTypeFilters_.begin(), noSelectTypeFilters_.end(), type ) !=
					noSelectTypeFilters_.end())
		return false;

	bool filterCheck = typeFilters_.empty() ||
		std::find( typeFilters_.begin(), typeFilters_.end(), type ) != typeFilters_.end();
	if( !filterCheck )
		return false;
	
	return true;
}

namespace
{
	void fillVector( BW::string str, BW::vector<BW::string>& vec, const char* seperator = "|" )
	{
		BW_GUARD;

		for (char* token = strtok( const_cast<char*>( str.c_str() ), seperator );
			token != NULL;
			token = strtok( NULL, seperator ) )
		{
			vec.push_back( token );
		}
	}

	BW::string fillString( const BW::vector<BW::string>& vec, char seperator = '|' )
	{
		BW_GUARD;

		BW::string f = "";
		bool first = true;

		BW::vector<BW::string>::const_iterator i = vec.begin();
		for (; i != vec.end(); ++i)
			if (first)
			{
				f += *i;
				first = false;
			}
			else
			{
				f += seperator + *i;
			}

		return f;	
	}
}

void SelectionFilter::typeFilters( BW::string filters )
{
	BW_GUARD;

	typeFilters_.clear();
	fillVector( filters, typeFilters_ );
}

BW::string SelectionFilter::typeFilters()
{
	BW_GUARD;

	return fillString( typeFilters_ );
}

void SelectionFilter::noSelectTypeFilters( BW::string filters )
{
	BW_GUARD;

	noSelectTypeFilters_.clear();
	fillVector( filters, noSelectTypeFilters_ );
}

BW::string SelectionFilter::noSelectTypeFilters()
{
	BW_GUARD;

	return fillString( noSelectTypeFilters_ );
}

void SelectionFilter::setFilterByName( BW::string& name )
{
	BW_GUARD;

	filterByName_ = strlwr( &name[0] );
}


// -----------------------------------------------------------------------------
// Section: setSelectionFilter
// -----------------------------------------------------------------------------

/*~ function WorldEditor.setSelectionFilter
 *	@components{ worldeditor }
 *
 *	This function sets the specified selection filters.
 *
 *	@param filters The selection filters to set.
 *
 *	Code Example:
 *	@{
 *	WorldEditor.setSelectionFilter('models|particles|omniLight')
 *	@}
 */
static PyObject* py_setSelectionFilter( PyObject* args )
{
	BW_GUARD;

	// parse arguments
	char* str;
	if (!PyArg_ParseTuple( args, "s", &str ))	{
		PyErr_SetString( PyExc_TypeError, "setSelectionFilter() "
			"expects a string argument" );
		return NULL;
	}

	SelectionFilter::typeFilters( str );
	GUI::Manager::instance().update();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setSelectionFilter, WorldEditor )

/*~ function WorldEditor.setNoSelectionFilter
 *	@components{ worldeditor }
 *
 *	This function excludes the specified selection filters from the current selection filters.
 *
 *	@param filters The filters to exclude from selection.
 */
static PyObject* py_setNoSelectionFilter( PyObject* args )
{
	BW_GUARD;

	// parse arguments
	char* str;
	if (!PyArg_ParseTuple( args, "s", &str ))	{
		PyErr_SetString( PyExc_TypeError, "setSelectionFilter() "
			"expects a string argument" );
		return NULL;
	}

	SelectionFilter::noSelectTypeFilters( str );
	GUI::Manager::instance().update();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setNoSelectionFilter, WorldEditor )


// -----------------------------------------------------------------------------
// Section: getSelectionFilter
// -----------------------------------------------------------------------------

/*~ function WorldEditor.getSelectionFilter
 *	@components{ worldeditor }
 *
 *	This function retrieves the selection filters.
 *
 *	@return Returns the selection filters.
 */
static PyObject* py_getSelectionFilter( PyObject* args )
{
	BW_GUARD;

	BW::string filters = SelectionFilter::typeFilters();

	return Py_BuildValue( "s", filters.c_str() );
}
PY_MODULE_FUNCTION( getSelectionFilter, WorldEditor )


// -----------------------------------------------------------------------------
// Section: setSelectShellsOnly
// -----------------------------------------------------------------------------

/*~ function WorldEditor.setSelectShellsOnly
 *	@components{ worldeditor }
 *
 *	This function sets the shell's selection rules. If 0 is passed 
 *	then the shell's chunk items may be selected depending on the selection filter, but
 *	not the shell itself. If 1 is passed then the shell will be able to be selected.
 *	If 2 is passed then all shell's chunk items may be selected, but not the shell
 *	itself.
 *
 *	@param rule The shell selection rule.
 */
static PyObject* py_setSelectShellsOnly( PyObject* args )
{
	BW_GUARD;

	// parse arguments
	int i;
	if (!PyArg_ParseTuple( args, "i", &i ))	{
		PyErr_SetString( PyExc_TypeError, "setSelectShellsOnly() "
			"expects an int argument" );
		return NULL;
	}

	SelectionFilter::setSelectMode( (SelectionFilter::SelectMode) i );
	GUI::Manager::instance().update();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setSelectShellsOnly, WorldEditor )
BW_END_NAMESPACE

