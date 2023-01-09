/**
 *	FILTER_SPEC: filters text according to its include/exclude rules
 */


#ifndef FILTER_SPEC_HPP
#define FILTER_SPEC_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class stores information about a single list filter.
 */
class FilterSpec : public ReferenceCount
{
public:
	FilterSpec( const BW::wstring& name, bool active = false,
		const BW::wstring& include = L"", const BW::wstring& exclude = L"",
		const BW::wstring& group = L"" );
	virtual ~FilterSpec();

	// Getters and setters
	virtual BW::wstring getName() { return name_; };
	virtual void setActive( bool active ) { active_ = active; };
	virtual bool getActive() { return active_ && enabled_; };
	virtual BW::wstring getGroup() { return group_; };

	virtual bool filter( const BW::wstring& str );

	void enable( bool enable );

protected:
	BW::wstring name_;
	bool active_;
	bool enabled_;
	BW::vector<BW::wstring> includes_;
	BW::vector<BW::wstring> excludes_;
	BW::wstring group_;
};
typedef SmartPointer<FilterSpec> FilterSpecPtr;

BW_END_NAMESPACE

#endif // FILTER_SPEC_HPP
