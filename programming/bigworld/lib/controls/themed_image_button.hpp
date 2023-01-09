/**
 *	Class that draws an overlay image over the button, allowing image buttons
 *	with XP/Vista themes.
 */
#pragma once

BW_BEGIN_NAMESPACE

namespace controls
{

	class ThemedImageButton : public CButton
	{
		DECLARE_DYNAMIC(ThemedImageButton)

	public:
		ThemedImageButton();
		~ThemedImageButton();

		HICON SetIcon( HICON hIcon );
		HICON SetCheckedIcon( HICON hIcon );

	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnNotifyCustomDraw ( NMHDR * pNotifyStruct, LRESULT* result );

	private:
		HICON hIcon_;
		HICON hCheckedIcon_;
	};

}

BW_END_NAMESPACE

