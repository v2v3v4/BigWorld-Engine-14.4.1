#ifndef VECTOR_GENERATOR_CUSTODIAN_HPP
#define VECTOR_GENERATOR_CUSTODIAN_HPP

#include "fwd.hpp"
#include "controls/edit_numeric.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gizmo/gizmo_manager.hpp"

BW_BEGIN_NAMESPACE

BW::string VecGenGUIStrFromID(BW::string const &str);
BW::string VecGenIDFromGuiStr(BW::string const &str);

template <class CL>
class VectorGeneratorCustodian
{
public:
    typedef VectorGenerator *(CL::*GetFn)();
    typedef void (CL::*SetFn)(VectorGenerator *);

    VectorGeneratorCustodian();

    virtual ~VectorGeneratorCustodian();

    void setComboBox(CComboBox * box);

    void setVectorGeneratorGetFn(GetFn func);

    void setVectorGeneratorSetFn(SetFn func);

    void setVectorGeneratorOwner(CL * owner);

    void 
    setGizmoProperties
    (
        Moo::Colour         wireColor           = 0xff0000ff, 
        bool                keepCurrentEditors  = false, 
        bool                drawVectors         = false
    );

    void 
    setPositionControls
    ( 
        controls::EditNumeric        *x, 
        controls::EditNumeric        *y, 
        controls::EditNumeric        *z,
        controls::EditNumeric        *x2, 
        controls::EditNumeric        *y2, 
        controls::EditNumeric        *z2
    );

    void 
    setParentVectorGeneratorCustodian
    (
        VectorGeneratorCustodian<CL>    *custodian
    );

    void 
    setChildVectorGeneratorCustodian
    (
        VectorGeneratorCustodian<CL>    *custodian
    );

    void updateControl();

    void updateControlNumerics();

    void updateOwner(bool drawIt = true);

    VectorGeneratorGizmoProperties GetGizmoProperties() const;

    void doDraw(bool draw);

    void scale(float scale_);

private:
    void populate();

    void setGizmo(bool drawIt = true, float scale = 1.0f);

private:
    bool                            populated_;
    bool                            inited_;
    CL                              *owner_;
    GetFn                           getFn_;
    SetFn                           setFn_;
    CComboBox                       *comboBox_;
    Moo::Colour                     gizmoWireColor_;
    bool                            gizmoKeepCurrentEditors_;
    bool                            gizmoDrawVectors_;
    VectorGeneratorGizmoProperties  gizmoProperties_;
    VectorGeneratorCustodian<CL>    *parentVectorGeneratorCustodian_;
    VectorGeneratorCustodian<CL>    *childVectorGeneratorCustodian_;
    controls::EditNumeric                    *x_;
    controls::EditNumeric                    *y_;
    controls::EditNumeric                    *z_;
    controls::EditNumeric                    *x2_;
    controls::EditNumeric                    *y2_;
    controls::EditNumeric                    *z2_;
    float                           scale_;
};

//
// Implementation
//

template <class CL>
VectorGeneratorCustodian<CL>::VectorGeneratorCustodian()
:
populated_(false),
inited_(false),
owner_(NULL),
getFn_(NULL),
setFn_(NULL),
comboBox_(NULL),
gizmoWireColor_(0.0f, 0.0f, 0.0f, 0.0f),
gizmoKeepCurrentEditors_(false),
gizmoDrawVectors_(false),
gizmoProperties_(),
parentVectorGeneratorCustodian_(NULL),
childVectorGeneratorCustodian_(NULL),
x_(NULL),
y_(NULL),
z_(NULL),
x2_(NULL),
y2_(NULL),
z2_(NULL),
scale_(1.0f)
{
}

template <class CL>
/*virtual*/ VectorGeneratorCustodian<CL>::~VectorGeneratorCustodian()
{
    if (gizmoProperties_.gizmo_)
        GizmoManager::instance().removeGizmo(gizmoProperties_.gizmo_);
}

template <class CL>
void VectorGeneratorCustodian<CL>::setComboBox(CComboBox * box) 
{ 
    comboBox_ = box; 
}

template <class CL>
void VectorGeneratorCustodian<CL>::setVectorGeneratorGetFn(GetFn func)
{
    // Set owner after setting these Get and Set funtion ptrs.
    ASSERT(!owner_);
    getFn_ = func;
}

template <class CL>
void VectorGeneratorCustodian<CL>::setVectorGeneratorSetFn(SetFn func)
{
    // Set owner after setting these Get and Set funtion ptrs.
    ASSERT(!owner_);    
    setFn_ = func;
}

template <class CL>
void VectorGeneratorCustodian<CL>::setVectorGeneratorOwner(CL * owner)
{
    owner_ = owner;
}

template <class CL>
void 
VectorGeneratorCustodian<CL>::setGizmoProperties
(
    Moo::Colour         wireColor           /*= 0xff0000ff*/, 
    bool                keepCurrentEditors  /*= false*/, 
    bool                drawVectors         /*= false*/
)
{
    gizmoWireColor_ = wireColor;
    gizmoKeepCurrentEditors_ = keepCurrentEditors;
    gizmoDrawVectors_ = drawVectors;
}

template <class CL>
void 
VectorGeneratorCustodian<CL>::setPositionControls
( 
    controls::EditNumeric        *x, 
    controls::EditNumeric        *y, 
    controls::EditNumeric        *z,
    controls::EditNumeric        *x2, 
    controls::EditNumeric        *y2, 
    controls::EditNumeric        *z2 
)
{
    x_  = x;
    y_  = y;
    z_  = z;
    x2_ = x2;
    y2_ = y2;
    z2_ = z2;
}

template <class CL>
void 
VectorGeneratorCustodian<CL>::setParentVectorGeneratorCustodian
(
    VectorGeneratorCustodian<CL>    *custodian
)
{
    parentVectorGeneratorCustodian_ = custodian;
}

template <class CL>
void 
VectorGeneratorCustodian<CL>::setChildVectorGeneratorCustodian
(
    VectorGeneratorCustodian<CL>    *custodian
)
{
    childVectorGeneratorCustodian_ = custodian;
}

template <class CL>
void VectorGeneratorCustodian<CL>::updateControl()
{
    // update the type of vector generator
    if (!populated_)
        populate();

    MF_ASSERT(owner_);
    MF_ASSERT(getFn_);

    BW::string generatorNameID  = ((*owner_).*getFn_)()->nameID();
    BW::wstring generatorNameGUI = bw_utf8tow( VecGenGUIStrFromID(generatorNameID) );

    CString selectedString = L"";
    MF_ASSERT( comboBox_->GetSafeHwnd() );
    int curSel = comboBox_->GetCurSel();
    if (curSel >= 0)
        comboBox_->GetLBText(curSel, selectedString);

    if (selectedString != generatorNameGUI.c_str())
    {
        // select string
        comboBox_->SelectString( -1, generatorNameGUI.c_str() );

        // setup gizmo
        setGizmo(true, scale_);

        // setup control states
        if ( x2_ )
        {
            if (generatorNameID == PointVectorGenerator::nameID_ ||
                generatorNameID == SphereVectorGenerator::nameID_)
            {
                
                x2_->EnableWindow( false );
                y2_->EnableWindow( false );
                z2_->EnableWindow( false );
            }
            else
            {
                x2_->EnableWindow( true );
                y2_->EnableWindow( true );
                z2_->EnableWindow( true );
            }
        }
    }

    updateControlNumerics();
}

template <class CL>
void VectorGeneratorCustodian<CL>::updateControlNumerics()
{
    // update the position of the vector generator (tell the controls)
    Matrix mat;
    gizmoProperties_.referenceMatrix_->getMatrix(mat);
    Vector3 pos = mat.applyToOrigin();
    if ( x_ )
    {
        x_->SetValue( pos.x );
        y_->SetValue( pos.y );
        z_->SetValue( pos.z );
    }
    if (gizmoProperties_.referenceMatrix2_)
    {
        gizmoProperties_.referenceMatrix2_->getMatrix(mat);
        pos = mat.applyToOrigin();
        if ( x2_ )
        {
            x2_->SetValue( pos.x );
            y2_->SetValue( pos.y );
            z2_->SetValue( pos.z );
        }
    }
}

template <class CL>
void VectorGeneratorCustodian<CL>::updateOwner( bool drawIt  /*= true*/)
{
    // update the type of vector generator
    if (!inited_)
    {
        MF_ASSERT(0);   // empty string will be returned.. should not be called
    }

    MF_ASSERT(owner_);
    MF_ASSERT(setFn_);

    CString selectedStringGUI;
    CString selectedStringID;
    MF_ASSERT( comboBox_->GetSafeHwnd() );
    int curSel = comboBox_->GetCurSel();
    if (curSel >= 0)
    {
        comboBox_->GetLBText(curSel, selectedStringGUI);
        selectedStringID = VecGenIDFromGuiStr(bw_wtoutf8( selectedStringGUI.GetString() )).c_str();
    }
    BW::string ownerId  = ((*owner_).*getFn_)()->nameID().c_str();

    if (selectedStringID != ownerId.c_str())
    {
        // create a new generator (delete is handled in the setPosition...
        VectorGenerator * newGenerator = 
            VectorGenerator::createGeneratorOfType(bw_wtoutf8( selectedStringID.GetString() ));
        ((*owner_).*setFn_)(newGenerator);

        // change gizmo
        setGizmo( drawIt, scale_ );

        // setup control states
        if ( x2_ )
        {
			BW::string nselectedStringID = bw_wtoutf8( selectedStringID.GetString() );
            if 
            (
                nselectedStringID == PointVectorGenerator::nameID_ 
                ||
                nselectedStringID == SphereVectorGenerator::nameID_
            )
            {
                x2_->EnableWindow( false );
                y2_->EnableWindow( false );
                z2_->EnableWindow( false );
            }
            else
            {
                x2_->EnableWindow( true );
                y2_->EnableWindow( true );
                z2_->EnableWindow( true );
            }
        }

        updateControlNumerics();
    }
    else
    {
        // update the position of the vector generator (tell the gizmo)
        if ( x2_ )
        {
            Matrix mat;
            if (gizmoProperties_.referenceMatrix2_)
            {
                gizmoProperties_.referenceMatrix2_->getMatrix( mat );
                Vector3 pos = mat.applyToOrigin();
                Vector3 newPos( x2_->GetValue(), y2_->GetValue(), z2_->GetValue() );
                if (newPos != pos)
                {
                    mat.setTranslate( newPos );
                    MainFrame::instance()->ForceActionPropertiesUpdateSkip();
                    gizmoProperties_.referenceMatrix2_->setMatrix( mat );
                }
            }
        
            // should always be a first matrix 
            gizmoProperties_.referenceMatrix_->getMatrix( mat );
            Vector3 pos = mat.applyToOrigin();
            Vector3 newPos( x_->GetValue(), y_->GetValue(), z_->GetValue() );
            if (newPos != pos)
            {
                mat.setTranslate( newPos );
                MainFrame::instance()->ForceActionPropertiesUpdateSkip();
                gizmoProperties_.referenceMatrix_->setMatrix( mat );
            }
        }
    }
}

template <class CL>
VectorGeneratorGizmoProperties 
VectorGeneratorCustodian<CL>::GetGizmoProperties() const 
{ 
    return gizmoProperties_; 
}

template <class CL>
void VectorGeneratorCustodian<CL>::doDraw(bool draw)
{
    if (draw)
        GizmoManager::instance().addGizmo(gizmoProperties_.gizmo_);
    else
        GizmoManager::instance().removeGizmo(gizmoProperties_.gizmo_);
}

template <class CL>
void VectorGeneratorCustodian<CL>::scale(float s)
{
    scale_ = s;
}

template <class CL>
void VectorGeneratorCustodian<CL>::populate()
{
    comboBox_->ResetContent();

    comboBox_->AddString
    (
        bw_utf8tow( VecGenGUIStrFromID(PointVectorGenerator::nameID_) ).c_str()
    );
    comboBox_->AddString
    (
        bw_utf8tow( VecGenGUIStrFromID(LineVectorGenerator::nameID_) ).c_str()
    );
    comboBox_->AddString
    (
        bw_utf8tow( VecGenGUIStrFromID(CylinderVectorGenerator::nameID_) ).c_str()
    );
    comboBox_->AddString
    (
        bw_utf8tow( VecGenGUIStrFromID(SphereVectorGenerator::nameID_) ).c_str()
    );
    comboBox_->AddString
    (
        bw_utf8tow( VecGenGUIStrFromID(BoxVectorGenerator::nameID_) ).c_str()
    );

    populated_ = true;
}

template <class CL>
void 
VectorGeneratorCustodian<CL>::setGizmo
(
    bool        drawIt      /*= true*/, 
    float       scale       /*= 1.0f*/
)
{
    // on initial call, do not set the radius of the gizmo..

    Vector3 * initialPosition = NULL;

    MatrixProxyPtr originMatrixProxy = NULL;
    if (parentVectorGeneratorCustodian_)
    {
        originMatrixProxy = 
            parentVectorGeneratorCustodian_->GetGizmoProperties().referenceMatrix_;
    }

    if (gizmoProperties_.gizmo_)
    {
        // i.e. the gizmo is being changed to another gizmo, keep similar positions
        Matrix m;
        gizmoProperties_.referenceMatrix_->getMatrix(m);
        initialPosition = new Vector3;
        *initialPosition = m.applyToOrigin();            
        GizmoManager::instance().removeGizmo(gizmoProperties_.gizmo_);
    }

    CreateGizmoInfo info;
    info.vectorGenerator_           = ((*owner_).*getFn_)();
    info.wireColor_                 = gizmoWireColor_;
    info.keepCurrentEditors_        = gizmoKeepCurrentEditors_;
    info.drawVectors_               = gizmoDrawVectors_;
    info.visualOffsetMatrixProxy_   = originMatrixProxy;
    info.initialPosition_           = initialPosition;
    info.setDefaultRadius_          = true;
    info.radiusGizmoRadius_         = 4.f;
    info.scale_                     = scale;

    // seperate the parent and child radius gizmos (only affects cylinder)
    if (childVectorGeneratorCustodian_)
        info.radiusGizmoRadius_ = 5.f;

    // want to set the default radius in all cases but where there is a radius set
    // in the loaded data (which can only happen in the first update)
    // if creating new, always create PointVectorGenerators, no radius required
    if (!inited_)
        info.setDefaultRadius_ = false;

    // make the gizmo!
    gizmoProperties_ = AddVectorGeneratorGizmo(info, drawIt);

    if (childVectorGeneratorCustodian_)
    {
        // give it a new position offset (the one it had is no longer in use)
        GizmoPtr gizmoDude = childVectorGeneratorCustodian_->GetGizmoProperties().gizmo_;
        if (gizmoDude)
        {
            gizmoDude->setVisualOffsetMatrixProxy(GetGizmoProperties().referenceMatrix_);
        }
    }

    if (initialPosition)
        delete initialPosition;

    inited_ = true;
}

BW_END_NAMESPACE

#endif // VECTOR_GENERATOR_CUSTODIAN_HPP
