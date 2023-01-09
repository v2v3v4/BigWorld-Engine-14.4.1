/**
 *	FilterHolder: a class that manages a series of filters and searchtext
 */


#include "pch.hpp"
#include "Shlwapi.h"
#include "filter_holder.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
FilterHolder::FilterHolder() :
	searchTextEnabled_( true )
{
	BW_GUARD;
}


/**
 *	This method returns whether or not there are any active filters.
 *
 *	@return True if at least one of the filters is active.
 */
bool FilterHolder::hasActiveFilters()
{
	BW_GUARD;

	for( FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f )
		if ( (*f)->getActive() && !(*f)->getName().empty() )
			return true;
	return false;
}


/**
 *	This method returns true if there is a search text, or if at least one of
 *	the filters is active.
 *
 *	@return True if using searcg text or at least one of the filters is active.
 */
bool FilterHolder::isFiltering()
{
	BW_GUARD;

	if ( filters_.empty() && searchText_.empty() )
		return false;

	if ( !searchText_.empty() )
		return true;

	if ( hasActiveFilters() )
		return true;

	return false;
}


/**
 *	This method adds a filter to the list of filters.
 *
 *	@param filter  Filter to be added.
 */
void FilterHolder::addFilter( FilterSpecPtr filter )
{
	BW_GUARD;

	if ( !filter )
		return;

	filters_.push_back( filter );
}


/**
 *	This method returns the filter at position "index" from it's internal list.
 *
 *	@param index	Position of the desired filter in the list.
 *	@return			Filter object at position "index".
 */
FilterSpecPtr FilterHolder::getFilter( int index )
{
	BW_GUARD;

	if ( index < 0 || index >= (int)filters_.size() )
		return 0;

	return filters_[ index ];
}


/**
 *	This method sets or clears the search text.
 *
 *	@param searchText	Search expresion text or the empty string to disable.
 */
void FilterHolder::setSearchText( const BW::wstring& searchText )
{
	BW_GUARD;

	searchText_ = searchText;
	StringUtils::toLowerCaseT( searchText_ );
}


/**
 *	This method enables or disables the search text.
 *
 *	@param enable	True to enable using the search text, false to disable it.
 */
void FilterHolder::enableSearchText( bool enable )
{
	BW_GUARD;

	searchTextEnabled_ = enable;
}


/**
 *	This method returns true if the "shortText" matches the search string and
 *	if the "text" passes the active the filters (at least one per group).
 *
 *	@param shortText	Item's short description.
 *	@param text			Item's long description, usually a file path.
 *	@return				True if the item passed the search text and filters.
 */
bool FilterHolder::filter( const BW::wstring& shortText, const BW::wstring& text )
{
	BW_GUARD;

	bool searchOk = true;

	// Match the search text, if any and enabled.
	if ( searchTextEnabled_ && !searchText_.empty() && !shortText.empty() )
	{
		bool useWildcards = true;
		if ( searchText_.find( '*' ) == BW::string::npos && searchText_.find( '?' ) == BW::string::npos )
			useWildcards = false;

		if ( useWildcards )
		{
			if ( !PathMatchSpec( shortText.c_str(), searchText_.c_str() ) )
				searchOk = false;
		}
		else
		{
			BW::wstring stxt = shortText;
			StringUtils::toLowerCaseT( stxt );
			if ( !wcswcs( stxt.c_str(), searchText_.c_str() ) )
				searchOk = false;
		}
	}

	if ( !searchOk )
		return false;

	// It matched the search, now make sure it passes the active filters.
	if ( !filters_.empty() && !text.empty() )
	{
		BW::map<BW::wstring,bool> groups; // group name, group state
		for( FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f )
		{
			if ( !(*f)->getActive() || (*f)->getName().empty() )
				continue;
			BW::map<BW::wstring,bool>::iterator g = groups.find( (*f)->getGroup() );
			if ( g != groups.end() )
			{
				// There's already an entry for this group, check if the filter test passes
				// If the group state is already true, don't change anything, even if the filter test fails
				if ( !(*g).second && (*f)->filter( text ) )
					(*g).second = true;
			}
			else
				// insert the new group with the filter test result as its state
				groups.insert( std::make_pair( (*f)->getGroup(), (*f)->filter( text ) ) );
		}

		// find out if the item passed at least one test per group
		bool push = true; // default value is true, in the case that no filter is active.
		for( BW::map<BW::wstring,bool>::iterator g = groups.begin(); 
			g != groups.end();
			++g )
		{
			if ( !(*g).second )
			{
				push = false;
				break;
			}
		}

		return push;
	}
	return true;
}


/**
 *	This method enables or disables all filters.
 *
 *	@param enable	True to activate all filters, false to disable.
 */
void FilterHolder::enableAll( bool enable )
{
	BW_GUARD;

	for( FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f )
		(*f)->enable( enable );
}


/**
 *	This method enables or disables a filter by name.
 *
 *	@param name		Name of the filter
 *	@param enable	True to activate all filters, false to disable.
 */
void FilterHolder::enable( const BW::wstring& name, bool enable )
{
	BW_GUARD;

	for( FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f )
		if ( (*f)->getName() == name )
		{
			(*f)->enable( enable );
			break;
		}
}


/**
 *	This method enables or disables all filters.
 *
 *	@param enable	True to activate all filters, false to disable.
 */
void FilterHolder::activateAll( bool enable )
{
	BW_GUARD;

	for (FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f)
	{
		(*f)->setActive( enable );
	}
}


/**
 *	This method find the filter by name.
 *
 *	@param name		Name of the filter
 */
FilterSpecPtr FilterHolder::findfilter( const BW::wstring& name )
{
	BW_GUARD;

	for (FilterSpecItr f = filters_.begin(); f != filters_.end(); ++f)
	{
		if ((*f)->getName() == name)
		{
			return (*f);
		}
	}
	return false;
}

BW_END_NAMESPACE

