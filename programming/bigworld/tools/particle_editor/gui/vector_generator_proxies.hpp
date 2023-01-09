#ifndef VECTOR_GENERATOR_PROXIES_HPP
#define VECTOR_GENERATOR_PROXIES_HPP

#include "main_frame.hpp"
#include "gizmo/position_gizmo.hpp"
#include "gizmo/general_properties.hpp"
#include "particle/actions/vector_generator.hpp"

BW_BEGIN_NAMESPACE

struct VectorGeneratorGizmoProperties
{
    GizmoPtr            gizmo_;
    MatrixProxyPtr      referenceMatrix_;
    MatrixProxyPtr      referenceMatrix2_;

    VectorGeneratorGizmoProperties();
};

struct CreateGizmoInfo
{
    VectorGenerator     *vectorGenerator_;
    Moo::Colour         wireColor_;
    bool                keepCurrentEditors_;
    bool                drawVectors_;
    MatrixProxyPtr      visualOffsetMatrixProxy_;
    Vector3             *initialPosition_;
    bool                setDefaultRadius_;
    float               radiusGizmoRadius_;
    float               scale_;

    CreateGizmoInfo();
};

VectorGeneratorGizmoProperties 
AddVectorGeneratorGizmo
(
    CreateGizmoInfo     const &info, 
    bool                drawIt      = true
);

// Gizmos adjust an item's properties through the Matrix Proxy class.
// These templates provide an interface to this for VectorGenerator objects.
template <class CL> 
class VectorGeneratorMatrixProxy : public MatrixProxy
{
public:
    typedef Vector3 (CL::*GetFn)() const;
    typedef void (CL::*SetFn)( const Vector3 & v );

    VectorGeneratorMatrixProxy
    (
        CL          *generator, 
        GetFn       getFn, 
        SetFn       setFn,
        float       scale   = 1.0f
    );

    /*virtual*/ ~VectorGeneratorMatrixProxy();

    void SetPositionSource(CL *generator);

    /*virtual*/ void getMatrix(Matrix &m, bool world = true);

    /*virtual*/ void getMatrixContext(Matrix &m);

    /*virtual*/ void getMatrixContextInverse(Matrix &m);

    /*virtual*/ bool setMatrix(const Matrix &m);

    /*virtual*/ void setMatrixAlone(const Matrix &m);

    /*virtual*/ void recordState();

    /*virtual*/ bool commitState( bool revertToRecord = false,
		bool addUndoBarrier = true );

    /*virtual*/ bool hasChanged();

private:
    CL          *generator_;             // this is owned elsewhere
    Matrix      matrix_;
    Matrix      recordStateMatrix_;
    GetFn       getFn_;
    SetFn       setFn_;
    float       scale_;
};


// Wrap some property that has get and set accessors.
// Template arguments are class to be overriden, and proxy to use.
template <class CL, class DT> 
class AccessorDataProxy : public DT
{
public:
    typedef typename DT::Data DTData;
    typedef typename DT::Data (CL::*GetFn)() const;
    typedef void (CL::*SetFn)( typename DT::Data v );

    AccessorDataProxy(CL * pItem, GetFn getFn, SetFn setFn);

    virtual DTData get() const;

    virtual void set(DTData v, bool transient, bool addBarrier = true);

private:
    CL                  *pItem_;
    GetFn               getFn_;
    SetFn               setFn_;
};

//
// Implementation
//

template <class CL> 
VectorGeneratorMatrixProxy<CL>::VectorGeneratorMatrixProxy
(
    CL          *generator, 
    GetFn       getFn, 
    SetFn       setFn,
    float       scale      /*= 1.0f*/
) 
:
getFn_(getFn),
setFn_(setFn),
scale_(scale)
{
    SetPositionSource(generator);
}

template <class CL>
/*virtual*/ VectorGeneratorMatrixProxy<CL>::~VectorGeneratorMatrixProxy() 
{
}

template <class CL>
void VectorGeneratorMatrixProxy<CL>::SetPositionSource(CL *generator)
{
    generator_ = generator;
    matrix_.setTranslate(((*generator).*getFn_)());
}

template <class CL>
/*virtual*/ void VectorGeneratorMatrixProxy<CL>::getMatrix
(
    Matrix      &m, 
    bool        world /*= true*/
)
{
    m = matrix_;
}

template <class CL>
/*virtual*/ void VectorGeneratorMatrixProxy<CL>::getMatrixContext
(
    Matrix      &m
)
{
    m = Matrix::identity;
}

template <class CL>
/*virtual*/ void VectorGeneratorMatrixProxy<CL>::getMatrixContextInverse
(
    Matrix      &m
)
{
    m = Matrix::identity;
}

template <class CL>
/*virtual*/ bool VectorGeneratorMatrixProxy<CL>::setMatrix
(
    Matrix      const &m
)
{
    setMatrixAlone(m);
    ((*generator_).*setFn_)(scale_*matrix_[3]);

    // update the gui
    MainFrame::instance()->ForceActionPropertiesUpdate();

    return true;
}

template <class CL>
/*virtual*/ void VectorGeneratorMatrixProxy<CL>::setMatrixAlone
(
    Matrix      const &m
)
{
    matrix_ = m;
}

template <class CL>
/*virtual*/ void VectorGeneratorMatrixProxy<CL>::recordState()
{
    recordStateMatrix_ = matrix_;
}

template <class CL>
/*virtual*/ bool VectorGeneratorMatrixProxy<CL>::commitState
( 
    bool        revertToRecord /*= false*/, 
    bool        addUndoBarrier /*= true*/ 
)
{
    if (revertToRecord)
        if (matrix_ != recordStateMatrix_)
            setMatrix(recordStateMatrix_);

    return true;
}

template <class CL>
/*virtual*/ bool VectorGeneratorMatrixProxy<CL>::hasChanged()
{
    return !!(recordStateMatrix_ != matrix_);
}

template <class CL, class DT>
AccessorDataProxy<CL, DT>::AccessorDataProxy
(
    CL      *pItem, 
    GetFn   getFn, 
    SetFn   setFn
) 
:
pItem_(pItem),
getFn_(getFn),
setFn_(setFn)
{
}

template <class CL, class DT>
/*virtual*/ typename AccessorDataProxy<CL, DT>::DTData
	AccessorDataProxy<CL, DT>::get() const
{
    return ((*pItem_).*getFn_)();
}

template <class CL, class DT>
/*virtual*/ void AccessorDataProxy<CL, DT>::set( typename DT::Data v,
	bool transient, bool addBarrier /*= true*/ )
{
    if (!transient)
	{
        MainFrame::instance()->OnBatchedUndoOperationEnd();
	}
    ((*pItem_).*setFn_)(v);
}

BW_END_NAMESPACE

#endif // VECTOR_GENERATOR_PROXIES_HPP
