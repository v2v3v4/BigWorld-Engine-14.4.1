#ifndef EMBEDDABLE_CDIALOG_HPP
#define EMBEDDABLE_CDIALOG_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     * CDialog controls defined in the resource editor that 
	 * need to be embedded in another CDialog should extend from this class
	 * @see SubclassDlgItem()
     */
	class EmbeddableCDialog : public CDialog
    {
    public:
        EmbeddableCDialog( UINT nIDTemplate, CWnd* pParentWnd = NULL );
		~EmbeddableCDialog();

		void SubclassDlgItem( UINT nID, CWnd * pParent );

	protected:
		virtual void OnOK();
		virtual void OnCancel();

	private:
		UINT templateId_;
	};
}

BW_END_NAMESPACE

#endif // EMBEDDABLE_CDIALOG_HPP
