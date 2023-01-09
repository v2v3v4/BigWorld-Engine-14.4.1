ADD_DEFINITIONS( -D_AFXDLL )
SET( CMAKE_MFC_FLAG 2 ) # 2 = MFC as a DLL

# TODO: Remove this once virtual hiding warnings fixed
BW_REMOVE_COMPILE_FLAGS( /w34263 )
