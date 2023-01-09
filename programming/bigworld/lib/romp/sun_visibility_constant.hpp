#ifndef LENS_DIRT_SUPPORT_HPP
#define LENS_DIRT_SUPPORT_HPP

#include "lens_effect.hpp"
#include "moo/forward_declarations.hpp"

BW_BEGIN_NAMESPACE

class SunVisibilityConstantSupport: public Moo::DeviceCallback
{
    LensEffect m_sunVisibilityTestEffect;
    bool m_added;
public:
    SunVisibilityConstantSupport();

    void load();

    const LensEffect& lensEffect() const { return m_sunVisibilityTestEffect; }

    virtual void createManagedObjects();
    virtual void deleteManagedObjects();
};

BW_END_NAMESPACE

#endif