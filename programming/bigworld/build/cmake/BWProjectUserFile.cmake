# We must run the following at "include" time, not at function call time,
# to find the path to this module rather than the path to a calling list file
GET_FILENAME_COMPONENT( _userFileTemplatePath
	${CMAKE_CURRENT_LIST_FILE}
	PATH )
SET( _userFileTemplatePath "${_userFileTemplatePath}/BWUserFileTemplates" )	

FUNCTION( BW_SET_WORKING_DIRECTORY TARGET_NAME PROJECT_PATH _WORKING_DIRECTORY )
	IF(  ${CMAKE_GENERATOR} MATCHES "Visual Studio" )
		SET(VCPROJ_TYPE vcxproj)
		IF(MSVC14)
			SET(BW_USERFILE_VC_VERSION 14.00)
		ELSEIF(MSVC12)
			SET(BW_USERFILE_VC_VERSION 12.00)
		ELSEIF(MSVC11)
			SET(BW_USERFILE_VC_VERSION 11.00)
		ELSEIF(MSVC10)
			SET(BW_USERFILE_VC_VERSION 10.00)
		ELSEIF(MSVC90)
			SET(BW_USERFILE_VC_VERSION 9.00)
			SET(VCPROJ_TYPE vcproj)
		ELSE()
			MESSAGE( FATAL_ERROR "This MSVC version is not supported by BigWorld!" )
		ENDIF()
		
		SET( TARGET_PATH "${PROJECT_PATH}/${TARGET_NAME}.${VCPROJ_TYPE}.user" )
		IF( EXISTS ${TARGET_PATH} )
			# Don't overwrite existing file
			RETURN()
		ENDIF()

		IF( BW_ARCH_32 ) 
			SET( BW_USERFILE_PLATFORM Win32 )
		ELSE()
			SET( BW_USERFILE_PLATFORM x64 )
		ENDIF()

		SET( BW_USERFILE_WORKING_DIRECTORY ${_WORKING_DIRECTORY} )
		
		FILE( READ "${_userFileTemplatePath}/BWPerConfig.${VCPROJ_TYPE}.user.in" _perconfig)
		
		SET( BW_USERFILE_CONFIGSECTIONS )
		FOREACH( BW_USERFILE_CONFIGNAME ${CMAKE_CONFIGURATION_TYPES} )
			STRING( CONFIGURE "${_perconfig}" _temp @ONLY ESCAPE_QUOTES )
			STRING( CONFIGURE
				"${BW_USERFILE_CONFIGSECTIONS}${_temp}"
				BW_USERFILE_CONFIGSECTIONS
				ESCAPE_QUOTES )
		ENDFOREACH()

		MESSAGE( STATUS "Creating .user file ${TARGET_PATH}" )		
		CONFIGURE_FILE( "${_userFileTemplatePath}/BW${VCPROJ_TYPE}.user.in"
			${TARGET_PATH}
			@ONLY )
	ENDIF()
ENDFUNCTION()
