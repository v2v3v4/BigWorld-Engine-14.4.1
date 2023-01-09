#include "pch.hpp"
#include "moo/render_context_debug.hpp"
#include "moo/render_context.hpp"


#ifdef _DEBUG




DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE


namespace
{
    //
    // Entries in these tables correspond to D3D state values.  For example see
    // d3DFillModeTable below.
    //
    struct D3DStateTableEntry
    {
        DWORD               value;
        char                *valueStr;
    };


    //
    // This macro creates a D3DStateTableEntry by taking a value and giving it
    // a string with the same name.
    //
#define D3DTABLE_ENTRY(NAME) \
    { (DWORD)NAME, #NAME }


    //
    // This macro creates a D3DStateTableEntry that terminates the table.
    //
#define D3DTABLE_DONE() \
    { 0, NULL }


    //
    // Filling modes.
    //
    D3DStateTableEntry d3DFillModeTable[] =
    {
        D3DTABLE_ENTRY(D3DFILL_POINT),
        D3DTABLE_ENTRY(D3DFILL_WIREFRAME),
        D3DTABLE_ENTRY(D3DFILL_SOLID),
        D3DTABLE_DONE()
    };


    //
    // Shade modes.
    //
    D3DStateTableEntry d3DShadeModeTable[] =
    {
        D3DTABLE_ENTRY(D3DSHADE_FLAT),
        D3DTABLE_ENTRY(D3DSHADE_GOURAUD),
        D3DTABLE_ENTRY(D3DSHADE_PHONG),
        D3DTABLE_DONE()
    };


    //
    // Blending modes.
    //
    D3DStateTableEntry d3DBlendTable[] =
    {
        D3DTABLE_ENTRY(D3DBLEND_ZERO),
        D3DTABLE_ENTRY(D3DBLEND_ONE),
        D3DTABLE_ENTRY(D3DBLEND_SRCCOLOR),
        D3DTABLE_ENTRY(D3DBLEND_INVSRCCOLOR),
        D3DTABLE_ENTRY(D3DBLEND_SRCALPHA),
        D3DTABLE_ENTRY(D3DBLEND_INVSRCALPHA),
        D3DTABLE_ENTRY(D3DBLEND_DESTALPHA),
        D3DTABLE_ENTRY(D3DBLEND_INVDESTALPHA),
        D3DTABLE_ENTRY(D3DBLEND_DESTCOLOR),
        D3DTABLE_ENTRY(D3DBLEND_INVDESTCOLOR),
        D3DTABLE_ENTRY(D3DBLEND_SRCALPHASAT),
        D3DTABLE_ENTRY(D3DBLEND_BOTHSRCALPHA),
        D3DTABLE_ENTRY(D3DBLEND_BOTHINVSRCALPHA),
        D3DTABLE_ENTRY(D3DBLEND_BLENDFACTOR),
        D3DTABLE_ENTRY(D3DBLEND_INVBLENDFACTOR),
        D3DTABLE_DONE()
    };


    //
    // Cull state.
    //
    D3DStateTableEntry d3dCullTable[] =
    {
        D3DTABLE_ENTRY(D3DCULL_NONE),
        D3DTABLE_ENTRY(D3DCULL_CW),
        D3DTABLE_ENTRY(D3DCULL_CCW),
        D3DTABLE_DONE()
    };


    //
    // Comparison functions.
    //
    D3DStateTableEntry d3dCmpFuncTable[] =
    {
        D3DTABLE_ENTRY(D3DCMP_NEVER),
        D3DTABLE_ENTRY(D3DCMP_LESS),
        D3DTABLE_ENTRY(D3DCMP_EQUAL),
        D3DTABLE_ENTRY(D3DCMP_LESSEQUAL),
        D3DTABLE_ENTRY(D3DCMP_GREATER),
        D3DTABLE_ENTRY(D3DCMP_NOTEQUAL),
        D3DTABLE_ENTRY(D3DCMP_GREATEREQUAL),
        D3DTABLE_ENTRY(D3DCMP_ALWAYS),
        D3DTABLE_DONE()
    };


    //
    // Fog formula state.
    //
    D3DStateTableEntry d3dFogTable[] =
    {
        D3DTABLE_ENTRY(D3DFOG_NONE),
        D3DTABLE_ENTRY(D3DFOG_EXP),
        D3DTABLE_ENTRY(D3DFOG_EXP2),
        D3DTABLE_ENTRY(D3DFOG_LINEAR),
        D3DTABLE_DONE()
    };


    //
    // Stencil operations.
    //
    D3DStateTableEntry d3dStencilOpTable[] =
    {
        D3DTABLE_ENTRY(D3DSTENCILOP_KEEP),
        D3DTABLE_ENTRY(D3DSTENCILOP_ZERO),
        D3DTABLE_ENTRY(D3DSTENCILOP_REPLACE),
        D3DTABLE_ENTRY(D3DSTENCILOP_INCRSAT),
        D3DTABLE_ENTRY(D3DSTENCILOP_DECRSAT),
        D3DTABLE_ENTRY(D3DSTENCILOP_INVERT),
        D3DTABLE_ENTRY(D3DSTENCILOP_INCR),
        D3DTABLE_ENTRY(D3DSTENCILOP_DECR),
        D3DTABLE_DONE()
    };


    //
    // Texture wrapping modes.
    //
    D3DStateTableEntry d3dWrapTable[] =
    {
        D3DTABLE_ENTRY(D3DWRAPCOORD_0),
        D3DTABLE_ENTRY(D3DWRAPCOORD_1),
        D3DTABLE_ENTRY(D3DWRAPCOORD_2),
        D3DTABLE_ENTRY(D3DWRAPCOORD_3),
        D3DTABLE_ENTRY(D3DWRAP_U),
        D3DTABLE_ENTRY(D3DWRAP_V),
        D3DTABLE_ENTRY(D3DWRAP_W),
        D3DTABLE_DONE()
    };


    //
    // Material colour sources.
    //
    D3DStateTableEntry d3dMaterialClrSrcTable[] =
    {
        D3DTABLE_ENTRY(D3DMCS_MATERIAL),
        D3DTABLE_ENTRY(D3DMCS_COLOR1),
        D3DTABLE_ENTRY(D3DMCS_COLOR2),
        D3DTABLE_DONE()
    };


    //
    // Vertex blending values.
    // 
    D3DStateTableEntry d3dVertexBlendFlagsTable[] =
    {
        D3DTABLE_ENTRY(D3DVBF_DISABLE),
        D3DTABLE_ENTRY(D3DVBF_1WEIGHTS),
        D3DTABLE_ENTRY(D3DVBF_2WEIGHTS),
        D3DTABLE_ENTRY(D3DVBF_3WEIGHTS),
        D3DTABLE_ENTRY(D3DVBF_TWEENING),
        D3DTABLE_ENTRY(D3DVBF_0WEIGHTS),
        D3DTABLE_DONE()
    };


    //
    // Patch edge values.
    //
    D3DStateTableEntry d3dPatchedEdgeTable[] =
    {
        D3DTABLE_ENTRY(D3DPATCHEDGE_DISCRETE),
        D3DTABLE_ENTRY(D3DPATCHEDGE_CONTINUOUS),
        D3DTABLE_DONE()
    };


    //
    // Debug monitor states.
    //
    D3DStateTableEntry d3dDebugMonitorTable[] =
    {
        D3DTABLE_ENTRY(D3DDMT_ENABLE),
        D3DTABLE_ENTRY(D3DDMT_DISABLE),
        D3DTABLE_DONE()
    };


    //
    // Patch degree table.
    //
    D3DStateTableEntry d3dDegreeTable[] =
    {
        D3DTABLE_ENTRY(D3DDEGREE_LINEAR),
        D3DTABLE_ENTRY(D3DDEGREE_QUADRATIC),
        D3DTABLE_ENTRY(D3DDEGREE_CUBIC),
        D3DTABLE_ENTRY(D3DDEGREE_QUINTIC),
        D3DTABLE_DONE()
    };


    //
    // What type is a D3D state?
    //
    enum D3DStateType
    {
        STATE_NUMBER,       // The state is a 32-bit number
        STATE_HEXNUMBER,    // The state is a hexadecimal 32-bit number (e.g. colour)
        STATE_FLOAT,        // The state is a floating point number.
        STATE_BOOLEAN,      // The state is a boolean.
        STATE_TABLE         // The state is an entry in one of the above tables.
    };


    //
    // This keeps track of what a device state is (boolean, number etc), a 
    // string representation of the state, the state's value and a pointer
    // to one of the above tables if the state is an enum.
    //
    struct D3DStateDetails
    {
        D3DRENDERSTATETYPE  stateValue;
        char                *stateValueStr;
        D3DStateType        type;
        D3DStateTableEntry  *table;
    };


    //
    // This macro creates a state that is a number.
    //
#define D3DSTATE_NUMBER(NAME) \
    { NAME, #NAME, STATE_NUMBER, NULL }


    //
    // This macro creates a state that is a hexadecimal number.
    //
#define D3DSTATE_HEXNUMBER(NAME) \
    { NAME, #NAME, STATE_HEXNUMBER, NULL }


    //
    // This macro creates a state that is a floating point number.
    //
#define D3DSTATE_FLOAT(NAME) \
    { NAME, #NAME, STATE_FLOAT, NULL }


    //
    // This macro creates a state that is an enum.
    //
#define D3DSTATE_TABLE(NAME, TABLE) \
    { NAME, #NAME, STATE_TABLE, TABLE }


    //
    // This macro creates a state that is a boolean.
    // 
#define D3DSTATE_BOOL(NAME) \
    { NAME, #NAME, STATE_BOOLEAN, NULL }


    //
    // This macro creates the state that corresponds to the end of the state
    // table.
    //
#define D3DSTATE_DONE() \
    { D3DRS_FORCE_DWORD, NULL, STATE_NUMBER, NULL }


    //
    // This is a giant list of a device's possible states.
    //
    D3DStateDetails d3dStates[] =
    {
        D3DSTATE_BOOL     (D3DRS_ZENABLE                                         ),
        D3DSTATE_TABLE    (D3DRS_FILLMODE              , d3DFillModeTable        ),
        D3DSTATE_TABLE    (D3DRS_SHADEMODE             , d3DShadeModeTable       ),
        D3DSTATE_BOOL     (D3DRS_ZWRITEENABLE                                    ),
        D3DSTATE_BOOL     (D3DRS_ALPHATESTENABLE                                 ),
        D3DSTATE_BOOL     (D3DRS_LASTPIXEL                                       ),
        D3DSTATE_TABLE    (D3DRS_SRCBLEND              , d3DBlendTable           ),
        D3DSTATE_TABLE    (D3DRS_DESTBLEND             , d3DBlendTable           ),
        D3DSTATE_TABLE    (D3DRS_CULLMODE              , d3dCullTable            ),
        D3DSTATE_TABLE    (D3DRS_ZFUNC                 , d3dCmpFuncTable         ),        
        D3DSTATE_HEXNUMBER(D3DRS_ALPHAREF                                        ),
        D3DSTATE_TABLE    (D3DRS_ALPHAFUNC             , d3dCmpFuncTable         ),
        D3DSTATE_BOOL     (D3DRS_DITHERENABLE                                    ),
        D3DSTATE_BOOL     (D3DRS_FOGENABLE                                       ),
        D3DSTATE_BOOL     (D3DRS_SPECULARENABLE                                  ),        
        D3DSTATE_HEXNUMBER(D3DRS_FOGCOLOR                                        ),
        D3DSTATE_TABLE    (D3DRS_FOGTABLEMODE          , d3dFogTable             ),
        D3DSTATE_FLOAT    (D3DRS_FOGSTART                                        ),
        D3DSTATE_FLOAT    (D3DRS_FOGEND                                          ),
        D3DSTATE_FLOAT    (D3DRS_FOGDENSITY                                      ),
        D3DSTATE_BOOL     (D3DRS_RANGEFOGENABLE                                  ),
        D3DSTATE_BOOL     (D3DRS_STENCILENABLE                                   ),
        D3DSTATE_TABLE    (D3DRS_STENCILFAIL           , d3dStencilOpTable       ),
        D3DSTATE_TABLE    (D3DRS_STENCILZFAIL          , d3dStencilOpTable       ),
        D3DSTATE_TABLE    (D3DRS_STENCILPASS           , d3dStencilOpTable       ),
        D3DSTATE_TABLE    (D3DRS_STENCILFUNC           , d3dCmpFuncTable         ),
        D3DSTATE_NUMBER   (D3DRS_STENCILREF                                      ),        
        D3DSTATE_HEXNUMBER(D3DRS_STENCILMASK                                     ),        
        D3DSTATE_HEXNUMBER(D3DRS_STENCILWRITEMASK                                ),        
        D3DSTATE_HEXNUMBER(D3DRS_TEXTUREFACTOR                                   ),
        D3DSTATE_TABLE    (D3DRS_WRAP0                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP1                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP2                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP3                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP4                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP5                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP6                 , d3dWrapTable            ),
        D3DSTATE_TABLE    (D3DRS_WRAP7                 , d3dWrapTable            ),
        D3DSTATE_BOOL     (D3DRS_CLIPPING                                        ),
        D3DSTATE_BOOL     (D3DRS_LIGHTING                                        ),        
        D3DSTATE_HEXNUMBER(D3DRS_AMBIENT                                         ),
        D3DSTATE_TABLE    (D3DRS_FOGVERTEXMODE         , d3dFogTable             ),
        D3DSTATE_BOOL     (D3DRS_COLORVERTEX                                     ),
        D3DSTATE_BOOL     (D3DRS_LOCALVIEWER                                     ),
        D3DSTATE_BOOL     (D3DRS_NORMALIZENORMALS                                ),
        D3DSTATE_TABLE    (D3DRS_DIFFUSEMATERIALSOURCE , d3dMaterialClrSrcTable  ),
        D3DSTATE_TABLE    (D3DRS_SPECULARMATERIALSOURCE, d3dMaterialClrSrcTable  ),
        D3DSTATE_TABLE    (D3DRS_AMBIENTMATERIALSOURCE , d3dMaterialClrSrcTable  ),
        D3DSTATE_TABLE    (D3DRS_EMISSIVEMATERIALSOURCE, d3dMaterialClrSrcTable  ),
        D3DSTATE_TABLE    (D3DRS_VERTEXBLEND           , d3dVertexBlendFlagsTable),
        D3DSTATE_BOOL     (D3DRS_CLIPPLANEENABLE                                 ),
        D3DSTATE_FLOAT    (D3DRS_POINTSIZE                                       ),
        D3DSTATE_FLOAT    (D3DRS_POINTSIZE_MIN                                   ),
        D3DSTATE_BOOL     (D3DRS_POINTSPRITEENABLE                               ),
        D3DSTATE_BOOL     (D3DRS_POINTSCALEENABLE                                ),
        D3DSTATE_FLOAT    (D3DRS_POINTSCALE_A                                    ),
        D3DSTATE_FLOAT    (D3DRS_POINTSCALE_B                                    ),
        D3DSTATE_FLOAT    (D3DRS_POINTSCALE_C                                    ),
        D3DSTATE_BOOL     (D3DRS_MULTISAMPLEANTIALIAS                            ),        
        D3DSTATE_HEXNUMBER(D3DRS_MULTISAMPLEMASK                                 ),
        D3DSTATE_TABLE    (D3DRS_PATCHEDGESTYLE       , d3dPatchedEdgeTable      ),
        D3DSTATE_TABLE    (D3DRS_DEBUGMONITORTOKEN    , d3dDebugMonitorTable     ),
        D3DSTATE_FLOAT    (D3DRS_POINTSIZE_MAX                                   ),
        D3DSTATE_BOOL     (D3DRS_INDEXEDVERTEXBLENDENABLE                        ),        
        D3DSTATE_HEXNUMBER(D3DRS_COLORWRITEENABLE                                ),
        D3DSTATE_FLOAT    (D3DRS_TWEENFACTOR                                     ),
        D3DSTATE_TABLE    (D3DRS_BLENDOP              , d3DBlendTable            ),
        D3DSTATE_TABLE    (D3DRS_POSITIONDEGREE       , d3dDegreeTable           ),
        D3DSTATE_TABLE    (D3DRS_NORMALDEGREE         , d3dDegreeTable           ),
        D3DSTATE_BOOL     (D3DRS_SCISSORTESTENABLE                               ),
        D3DSTATE_NUMBER   (D3DRS_SLOPESCALEDEPTHBIAS                             ),
        D3DSTATE_BOOL     (D3DRS_ANTIALIASEDLINEENABLE                           ),
        D3DSTATE_FLOAT    (D3DRS_MINTESSELLATIONLEVEL                            ),
        D3DSTATE_FLOAT    (D3DRS_MAXTESSELLATIONLEVEL                            ),
        D3DSTATE_FLOAT    (D3DRS_ADAPTIVETESS_X                                  ),
        D3DSTATE_FLOAT    (D3DRS_ADAPTIVETESS_Y                                  ),
        D3DSTATE_FLOAT    (D3DRS_ADAPTIVETESS_Z                                  ),
        D3DSTATE_FLOAT    (D3DRS_ADAPTIVETESS_W                                  ),
        D3DSTATE_BOOL     (D3DRS_ENABLEADAPTIVETESSELLATION                      ),
        D3DSTATE_BOOL     (D3DRS_TWOSIDEDSTENCILMODE                             ),
        D3DSTATE_TABLE    (D3DRS_CCW_STENCILFAIL      , d3dStencilOpTable        ),
        D3DSTATE_TABLE    (D3DRS_CCW_STENCILZFAIL     , d3dStencilOpTable        ),
        D3DSTATE_TABLE    (D3DRS_CCW_STENCILPASS      , d3dStencilOpTable        ),
        D3DSTATE_TABLE    (D3DRS_CCW_STENCILFUNC      , d3dCmpFuncTable          ),        
        D3DSTATE_HEXNUMBER(D3DRS_COLORWRITEENABLE1                               ),        
        D3DSTATE_HEXNUMBER(D3DRS_COLORWRITEENABLE2                               ),        
        D3DSTATE_HEXNUMBER(D3DRS_COLORWRITEENABLE3                               ),        
        D3DSTATE_HEXNUMBER(D3DRS_BLENDFACTOR                                     ),
        D3DSTATE_NUMBER   (D3DRS_SRGBWRITEENABLE                                 ),
        D3DSTATE_NUMBER   (D3DRS_DEPTHBIAS                                       ),
        D3DSTATE_TABLE    (D3DRS_WRAP8               , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP9               , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP10              , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP11              , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP12              , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP13              , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP14              , d3dWrapTable              ),
        D3DSTATE_TABLE    (D3DRS_WRAP15              , d3dWrapTable              ),
        D3DSTATE_BOOL     (D3DRS_SEPARATEALPHABLENDENABLE                        ),
        D3DSTATE_TABLE    (D3DRS_SRCBLENDALPHA       , d3DBlendTable             ),
        D3DSTATE_TABLE    (D3DRS_DESTBLENDALPHA      , d3DBlendTable             ),
        D3DSTATE_TABLE    (D3DRS_BLENDOPALPHA        , d3DBlendTable             ),
        D3DSTATE_DONE()
    };


    //
    // This prints the lower four bits of "nibble" to out.
    //
	void outNibble(std::ostream &out, DWORD nibble)
    {
        nibble &= 0x0f;
        if (nibble <= 9)
            out << (char)(nibble + '0');
        else
            out << (char)(nibble + 'A' - 10);
    }


    //
    // Output a 32-bit hexadecimal number to out.
    //
    void outHex(std::ostream &out, DWORD number)
    {
        out << "0x";
        outNibble(out, (number >> 28) & 0x0f);
        outNibble(out, (number >> 24) & 0x0f);
        outNibble(out, (number >> 20) & 0x0f);
        outNibble(out, (number >> 16) & 0x0f);
        outNibble(out, (number >> 12) & 0x0f);
        outNibble(out, (number >>  8) & 0x0f);
        outNibble(out, (number >>  4) & 0x0f);
        outNibble(out, (number >>  0) & 0x0f);
    }
}


//
// This function prints the state of a device to out.
//
// @param out           The output stream.
// @param context       The render context whose state should be printed.
// @return              A reference to out.
//
std::ostream &Moo::printRenderContextState(std::ostream &out, RenderContext &rc)
{
    DX::Device *device = rc.device();
    if (device == NULL)
    {
		out << "no device" << std::endl;
        return out;
    }

    for (size_t i = 0; d3dStates[i].stateValueStr != NULL; ++i)
    {
		BW::string stateName = d3dStates[i].stateValueStr;
        out << stateName;
        for (size_t j = stateName.length(); j < 40; ++j)
            out << ' ';
        out << "= ";
        DWORD stateValue = 0;
        HRESULT hr =
            device->GetRenderState(d3dStates[i].stateValue, &stateValue);
        switch (d3dStates[i].type)
        {
        case STATE_NUMBER:
            out << stateValue;
            break;
        case STATE_HEXNUMBER:
            outHex(out, stateValue);
            break;
        case STATE_FLOAT:
            {
            float value = *((float *)(&stateValue));
            out << value;
            }
            break;
        case STATE_BOOLEAN:
            if (stateValue == 0)
                out << "FALSE";
            else
                out << "TRUE";
            break;
        case STATE_TABLE:
            {
                bool foundOne = false;
                D3DStateTableEntry *table = d3dStates[i].table;
                for (size_t j = 0; table[j].valueStr != NULL; ++j)
                {
                    if (stateValue == table[j].value)
                    {
                        out << table[j].valueStr;
                        foundOne = true;
                        break;
                    }
                }
                if (!foundOne)
                    outHex(out, stateValue);
            }
            break;
        }
        out << std::endl;
    }
    return out;
}

BW_END_NAMESPACE

#endif // _DEBUG
