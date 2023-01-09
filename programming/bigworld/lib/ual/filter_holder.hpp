/**
 *	FilterHolder: a class that manages a series of filters and searchtext
 */


#ifndef FILTER_HOLDER_HPP
#define FILTER_HOLDER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "filter_spec.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class keeps track of search text and multiple filters used for
 *	narrowing a list of strings.
 */
class FilterHolder
{
public:
	FilterHolder();

	bool hasActiveFilters();
	bool isFiltering();
	void addFilter( FilterSpecPtr filter );
	FilterSpecPtr getFilter( int index );

	void setSearchText( const BW::wstring& searchText );
	void enableSearchText( bool enable );

	bool filter( const BW::wstring& shortText, const BW::wstring& text );
	void enableAll( bool enable );
	void enable( const BW::wstring& name, bool enable );

	void activateAll( bool active );
	FilterSpecPtr findfilter( const BW::wstring& name );
private:
	BW::wstring searchText_;
	bool searchTextEnabled_;
	BW::vector<FilterSpecPtr> filters_;
	typedef BW::vector<FilterSpecPtr>::iterator FilterSpecItr;
};


BW_END_NAMESPACE

#endif // FILTER_HOLDER_HPP
