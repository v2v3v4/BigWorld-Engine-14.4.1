ADD_DEFINITIONS(
	-D_LIB
)

IF ( BW_IS_TOOLS OR 
	 BW_IS_OFFLINEPROCESSOR )
	SET( BW_USE_CSTDMF_DLL ON )
	SET( BW_USE_CSTDMF_IN_DLL ON )

	ADD_DEFINITIONS( 
		-DCSTDMF_IMPORT
	)
ENDIF()

