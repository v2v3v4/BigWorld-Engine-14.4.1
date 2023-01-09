#include "pch.hpp"

BW_BEGIN_NAMESPACE
extern int ConnectionModel_Token;
extern int EntityDef_Token;
static int tokens = ConnectionModel_Token |
	EntityDef_Token;
BW_END_NAMESPACE

BOOL    WINAPI  DllMain (HANDLE hInst,
                         ULONG ul_reason_for_call,
                         LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
			BW::Allocator::setSystemStage( BW::Allocator::SS_MAIN );
            break;

        case DLL_PROCESS_DETACH:
			BW::Allocator::setSystemStage( BW::Allocator::SS_POST_MAIN );
            break;
    }
    return TRUE;
}



