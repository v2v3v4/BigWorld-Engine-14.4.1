#ifndef MATERIAL_KINDS_CONSTANTS
#define MATERIAL_KINDS_CONSTANTS
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

// Range of material kind values for destructibles
const uint MATKIND_DESTRUCTIBLE_MIN = 71;
const uint MATKIND_DESTRUCTIBLE_MAX = 100;

const uint MATKIND_UNBROKEN_MIN = 73;
const uint MATKIND_UNBROKEN_MAX = 86;
const uint MATKIND_BROKEN_MIN = 87;
const uint MATKIND_BROKEN_MAX = 100;

// Speedtree matkinds also theaded as tangible.
const uint MATKIND_TANGIBLE_MIN = MATKIND_DESTRUCTIBLE_MIN;
const uint MATKIND_TANGIBLE_MAX = MATKIND_UNBROKEN_MAX;
const uint NUM_TANGIBLE_MATKINDS = MATKIND_TANGIBLE_MAX - MATKIND_TANGIBLE_MIN;


// Material kind values for speedtree objects
const uint MATKIND_SPT_SOLID = 71;
const uint MATKIND_SPT_LEAVES = 72;

const uint MATKIND_ROCK = 107;

const uint MATKIND_MAX = 255;

inline bool isDestructible(uint matKind)
{
    return (matKind >= MATKIND_DESTRUCTIBLE_MIN) & (matKind <= MATKIND_DESTRUCTIBLE_MAX);
}

inline bool isTangibleMatkind(uint matKind)
{
    return (matKind >= MATKIND_TANGIBLE_MIN) & (matKind <= MATKIND_TANGIBLE_MAX);
}

enum GroundType
{
    GROUNDTYPE_NONE = 0,
    GROUNDTYPE_FIRM = 1,
    GROUNDTYPE_MEDIUM = 2,
    GROUNDTYPE_SOFT = 3,
    GROUNDTYPE_SLOPE = 4,
    GROUNDTYPE_DEATH_ZONE = 5
};

BW_END_NAMESPACE

#endif //MATERIAL_KINDS_CONSTANTS
