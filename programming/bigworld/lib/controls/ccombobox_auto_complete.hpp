#ifndef CCOMBOBOX_AUTO_COMPLETE_HPP
#define CCOMBOBOX_AUTO_COMPLETE_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
	class CONTROLS_DLL CComboBoxAutoComplete : public CComboBox
	{
		DECLARE_DYNAMIC(CComboBoxAutoComplete)

	public:
		CComboBoxAutoComplete();

		/*virtual*/ ~CComboBoxAutoComplete();

		void restrictToListBoxItems(bool option);

	private:
		virtual BOOL PreTranslateMessage(MSG* pMsg);

		afx_msg void OnCbnEditupdate();
		
		DECLARE_MESSAGE_MAP()

	private:
		CString         previousText_;
		int             previousCurSel_;
		bool            restrictToListBoxItems_;
	};
}

BW_END_NAMESPACE

#endif // CCOMBOBOX_AUTO_COMPLETE_HPP



