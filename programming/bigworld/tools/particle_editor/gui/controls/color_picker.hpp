#ifndef COLOR_PICKER_HPP
#define COLOR_PICKER_HPP

class ColorPicker : public CWnd
{
public:
    typedef void(*CallbackFunc)(COLORREF, void *ClientData);

    ColorPicker();
    /*virtual*/ ~ColorPicker();

    BOOL 
    Create
    (
        DWORD   dwStyle, 
        CRect   rcPos, 
        CWnd    *pParent, 
        bool    alphaSelection  = true
    );
    
    void 
    registerCallbackOnMove
    (
        CallbackFunc    ptr, 
        void            *userData
    );

    void registerCallbackOnLButtonDown
    (
        CallbackFunc    ptr, 
        void            *userData
    );
    
    void        setRGB(COLORREF ref);
    COLORREF    getRGB();

    void        setHLS(float hue, float luminance, float saturation);
    void        getHLS(float & hue, float & luminance, float & saturation);

    void        setRGBA(const Vector4 & color);
    Vector4     getRGBA() const;

private:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    void generatePicker();
    void generateHueBar();
    void generateAlphaBar();

    void setPicker();

    void drawCrossAt(CPoint &point, CPaintDC &dc, CSize &sz);
    void calculateSizes();

private:
    enum MouseMoveMode
    {
        MMM_NONE,
        MMM_HUE,
        MMM_SL_PICKER,
        MMM_ALPHA
    };

    CDC             *pickerDC_;
    CDC             *hueBarDC_;
    CDC             *alphaBarDC_;
    bool            createPickerDC_;
    bool            createHueBarDC_;
    bool            createAlphaBarDC_;
    bool            slGraphDirty_;      // saturation and luminance graph dirty flag
    bool            hueBarDirty_;       // hue bar dirty flag
    bool            slPickerDirty_;     // picker position dirty
    bool            alphaBarDirty_;
    MouseMoveMode   mouseMoveMode_;     // moving on picker or on hue
    CPoint          slPickerPosition_;  // position of the picker
    float           currentLuminance_;
    float           currentSaturation_;
    float           currentHue_;
    COLORREF        currentColor_;
    float           currentAlpha_;
    SIZE            slGraphSize_;
    SIZE            hueBarSize_;
    SIZE            alphaBarSize_;
    SIZE            totalSize_;
    CallbackFunc    pOnMove;            // callback
    CallbackFunc    pOnLDown;           // callback
    void            *onMoveCD;          // client data
    void            *onLDownCD;         // client data
    bool            alphaSelection_;
};

#endif // COLOR_PICKER_HPP
