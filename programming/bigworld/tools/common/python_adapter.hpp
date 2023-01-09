#pragma once

#include "appmgr/scripted_module.hpp"

#include "cstdmf/bw_string.hpp"

class CListBox;
class CSliderCtrl;

BW_BEGIN_NAMESPACE

class PythonAdapter
{
public:
	PythonAdapter();
	~PythonAdapter();

	bool hasScriptObject() const;

	void reloadUIAdapter();

	bool call( const BW::string & fnName );
	bool callString( const BW::string & fnName, const BW::string & param );
	bool callString2( const BW::string & fnName, const BW::string & param1,
		const BW::string & param2 );

	bool ActionScriptExecute( const BW::string & functionName );
	bool ActionScriptUpdate( const BW::string & actionName, int & enabled,
		int & checked );

	void onSliderAdjust( const BW::string & name, int pos, int min, int max );
	void sliderUpdate( CSliderCtrl * slider, const BW::string & sliderName );

	void onListItemSelect( const BW::string & name, int index );
	void onListSelectUpdate( CListBox * listBox,
		const BW::string & listBoxName );

	void onListItemToggleState( const BW::string & name, unsigned int index,
		bool state );

	void contextMenuGetItems( const BW::wstring & type,
		const BW::wstring & path, BW::map<int,BW::wstring> & items );
	void contextMenuHandleResult( const BW::wstring & type,
		const BW::wstring & path, int result );

protected:
	PyObject* pScriptObject_;
    bool proActive_;
};
BW_END_NAMESPACE

