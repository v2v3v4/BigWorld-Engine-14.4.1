#ifndef PAGE_OPTIONS_WEATHER_HPP
#define PAGE_OPTIONS_WEATHER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "common/editor_views.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/edit_numeric.hpp"
#include "gui/controls/weather_settings_table.hpp"
#include "guitabs/guitabs_content.hpp"
#include "resmgr/string_provider.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/** 
 *	This class sub-classes the MFC CTreeCtrl and allows us to handle
 *	and override the behaviour of some events.
 */
class WeatherSystemsList : public CTreeCtrl
{
public:
	afx_msg void OnTvnEndlabeleditWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnKeydownWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLbnSelchangeWeatherSystemsList(NMHDR *pNMHDR=NULL, LRESULT *pResult=NULL);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP();
	CPoint mousePos_;
};


/** 
 *	This class implements the Weather Systems Page in WorldEditor.  It inherits
 *	from the WeatherSettingsTable class for its properties list.
 */
class PageOptionsWeather : public WeatherSettingsTable, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_WEATHER/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_WEATHER/LONG_NAME"), 290, 500, NULL )

	DECLARE_AUTO_TOOLTIP_EX( PageOptionsWeather )

	typedef PageOptionsWeather This;

public:
	PageOptionsWeather();	
	void	onSelectWeatherSystem();
	void	saveUndoState(const BW::string& description);
	bool	refillingWeatherSystems_;

// Dialog Data
	enum { IDD = IDD_PAGE_OPTIONS_WEATHER };

private:
	void	refillWeatherSystemsList();
	DataSectionPtr systemDataSection() const;
	void	addPropertyItems(DataSectionPtr pSystem);
	void	populateSkyBoxes(DataSectionPtr pSystem);
	bool	changingWeatherSettings_;
	bool	pageReady_;	
	HTREEITEM lastWeatherSystemSelection_;
	WeatherSystemsList weatherSystemsList_;
	BW::vector<HTREEITEM> weatherSystemItems_;
	CListBox skyBoxesList_;
	controls::DialogToolbar skyBoxesTB_;

	static void refreshWeatherSystemList();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, refreshWeatherSystemList, END )

	static void refreshSkyBoxList();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, refreshSkyBoxList, END )

	static void refillWeatherSystemProperties();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, refillWeatherSystemProperties, END )

	static void selectWeatherSystemByIdx( uint32 idx );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, selectWeatherSystemByIdx, ARG( uint32, END ) )

	static void selectSkyBoxByIdx( uint32 idx );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, selectSkyBoxByIdx, ARG( uint32, END ) )

	static BW::string spaceName();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, spaceName, END )

	static void PageOptionsWeather::renameCurrentWeatherSystem();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, renameCurrentWeatherSystem, END )

	static void PageOptionsWeather::saveWeatherUndoState( const BW::string& desc );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, saveWeatherUndoState, ARG( BW::string, END ) )


#define IMPLEMENT_BTN_HANDLER( fn, action )	\
	afx_msg void OnBnClicked##fn##()		\
	{										\
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute( action );\
	}

	IMPLEMENT_BTN_HANDLER( WeatherAdd, "actAddWeatherSystem" )
	IMPLEMENT_BTN_HANDLER( WeatherRemove, "actRemoveWeatherSystem" )
	IMPLEMENT_BTN_HANDLER( WeatherDefault, "actDefaultWeatherSystem" )
	IMPLEMENT_BTN_HANDLER( WeatherExclude, "actExcludeWeatherSystem" )
	IMPLEMENT_BTN_HANDLER( SkyBoxUp, "actSkyBoxUp" )
	IMPLEMENT_BTN_HANDLER( SkyBoxDown, "actSkyBoxDown" )
	IMPLEMENT_BTN_HANDLER( SkyBoxDel, "actSkyBoxDel" )
	IMPLEMENT_BTN_HANDLER( SkyBoxClear, "actClearSkyBoxes" )

	afx_msg void OnBnClickedWeatherRename();
	afx_msg void OnBnClickedSkyBoxAdd();	
	afx_msg void OnLbnSelchangeSkyBoxList();
	afx_msg void OnSkyBoxUpEnable  (CCmdUI *cmdui);
    afx_msg void OnSkyBoxDownEnable(CCmdUI *cmdui);
    afx_msg void OnSkyBoxDelEnable (CCmdUI *cmdui);
	virtual BOOL PreTranslateMessage( MSG* pMsg );	
	PropertyItem * rclickItem_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP();

public:
	virtual BOOL InitPage();

	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);	
	bool skyBoxDrop( UalItemInfo* ii );
};


IMPLEMENT_BASIC_CONTENT_FACTORY( PageOptionsWeather )

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_WEATHER_HPP
