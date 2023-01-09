#ifndef PAGE_MESSAGE_IMPL_HPP
#define PAGE_MESSAGE_IMPL_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "common/page_messages.hpp"

BW_BEGIN_NAMESPACE

class BBMsgImpl: public MsgsImpl
{
public:

	BBMsgImpl( PageMessages* msgs );
	~BBMsgImpl();

	void OnNMClickMsgList(NMHDR *pNMHDR, LRESULT *pResult);
	void OnNMCustomdrawMsgList(NMHDR *pNMHDR, LRESULT *pResult);

private:
	PageMessages* msgs_;
	HFONT boldFont;
};

BW_END_NAMESPACE

#endif // PAGE_MESSAGE_IMPL_HPP
