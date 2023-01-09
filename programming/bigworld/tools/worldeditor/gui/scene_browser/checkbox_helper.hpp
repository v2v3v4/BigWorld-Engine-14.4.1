#ifndef CHECKBOX_HELPER_HPP
#define CHECKBOX_HELPER_HPP

BW_BEGIN_NAMESPACE

class CheckboxHelper
{
public:
	void init( int checkedResId, int uncheckedResId,
		COLORREF fgSel, COLORREF bgSel, COLORREF fgEven, COLORREF bgEven,
		COLORREF fgOdd, COLORREF bgOdd );

	void draw( CDC & dc, const CRect & rect, const CPoint & pt,
									bool selected, bool odd, bool checked );

	void rect( CRect & retRect ) const;

private:
	CBitmap checkOnOdd_;
	CBitmap checkOnEven_;
	CBitmap checkOnSel_;
	CBitmap checkOffOdd_;
	CBitmap checkOffEven_;
	CBitmap checkOffSel_;
};

BW_END_NAMESPACE

#endif // CHECKBOX_HELPER_HPP
