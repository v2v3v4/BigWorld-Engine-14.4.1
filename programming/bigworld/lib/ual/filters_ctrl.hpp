/**
 *	FiltersCtrl: Manages a set of push-like checkbox buttons.
 */

#ifndef FILTERS_CTRL_HPP
#define FILTERS_CTRL_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This interface class is used as a base class of classes wanting to receive
 *	events from the FiltersCtrl.
 */
class FiltersCtrlEventHandler
{
public:
	virtual void filterClicked( const wchar_t* name, bool pushed, void* data ) = 0;
};



/**
 *	This class implements an MFC control that displays a series of grouped
 *	filters as checkboxes/buttons.
 */
class FiltersCtrl : public CWnd
{
public:
	FiltersCtrl();
	virtual ~FiltersCtrl();

	void setPushlike( bool pushlike ) { pushlike_ = pushlike; };

	void clear();
	bool empty();
	void add( const wchar_t* name, bool pushed, void* data );
	void addSeparator();
	int getHeight();
	void recalcWidth( int width );
	void enableAll( bool enable );
	void enable( const BW::wstring& name, bool enable );

	void setEventHandler( FiltersCtrlEventHandler* eventHandler );

private:
	/**
	 *	This internal helper class helps in keeping info for one filter button.
	 */
	class Filter : public ReferenceCount
	{
	public:
		BW::wstring name;
		CButton button;
		CStatic separator;
		void* data;
	};
	typedef SmartPointer<Filter> FilterPtr;

	// Member variables
	static const int FILTERCTRL_ID_BASE = 3000;
	FiltersCtrlEventHandler* eventHandler_;
	BW::vector<FilterPtr> filters_;
	typedef BW::vector<FilterPtr>::iterator FilterItr;
	int lines_;
	int separatorWidth_;
	int butSeparation_;
	bool pushlike_;

	// Private methods
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnFilterClicked( UINT nID );
	DECLARE_MESSAGE_MAP()
};


BW_END_NAMESPACE


#endif // FILTERS_CTRL_HPP
