SET( BW_LINUX_CONN_TYPE "NONE" 
	CACHE STRING "The method to connect to the remote server")
SET( BW_LINUX_PRIV_KEY "" CACHE STRING "The location of the private key file")
SET( BW_LINUX_DIR "" CACHE STRING "Mounted Linux Directory")
SET( BW_LINUX_HOST "" CACHE STRING "Server build hostname")
SET( BW_LINUX_USER $ENV{USERNAME} CACHE STRING "Server build Username")
SET( BW_LINUX_CFLAGS "" CACHE STRING "Server build compile flags")
OPTION( BW_REPLACE_LINUX_DIR "Replace linux dir in output" ON )
OPTION( BW_VERIFY_LINUX "Compile server libraries when compiling client librarires" OFF )
SET( BW_IS_REMOTE_BUILD OFF )
SET( BW_IS_REMOTE_ONLY OFF )

IF ( NOT "${BW_LINUX_CONN_TYPE}" STREQUAL "NONE" )
	IF ( BW_VERIFY_LINUX )
		BW_CUSTOM_COMMAND( "enable_linux_build" 
			del /f $(SolutionDir).DO_NOT_BUILD_SERVER )
		BW_CUSTOM_COMMAND( "disable_linux_build"
			echo \"\" > $(SolutionDir).DO_NOT_BUILD_SERVER )
	ELSE()
		SET( BW_IS_REMOTE_ONLY ON )
	ENDIF()
	SET( BW_IS_REMOTE_BUILD ON )
ENDIF()

IF( "${BW_LINUX_CONN_TYPE}" STREQUAL "PUTTY_SSH" )
	FIND_PROGRAM( BW_LINUX_PLINK_PATH "plink.exe" 
		HINTS "${BW_SOURCE_DIR}/third_party/putty" )

	SET( BW_LINUX_SSH_PATH "${BW_LINUX_PLINK_PATH}" )
	IF( "${BW_LINUX_PRIV_KEY}" STREQUAL "" )
		SET( BW_LINUX_SSH_ARGS "-ssh -batch -agent" )
	ELSE()
		SET( BW_LINUX_SSH_ARGS "-ssh -batch -i \"${BW_LINUX_PRIV_KEY}\"" )
	ENDIF()
	SET( BW_LINUX_SSH_ARGS "${BW_LINUX_SSH_ARGS} ${BW_LINUX_USER}@${BW_LINUX_HOST}" )

ENDIF()

IF( BW_REPLACE_LINUX_DIR )
	string( REGEX REPLACE "[]/()$*.^|[]" "\\\\\\0" BW_LINUX_DIR_SAFE ${BW_LINUX_DIR} "" )
	string( REGEX REPLACE "[]/()$*.^|[]" "\\\\\\0" BW_SOURCE_DIR_SAFE ${BW_SOURCE_DIR} "" )

	SET( BW_LINUX_COMPILE_EXTRA "| sed 's/${BW_LINUX_DIR_SAFE}/${BW_SOURCE_DIR_SAFE}/'" )
	IF( "${BW_LINUX_CONN_TYPE}" STREQUAL "PUTTY_SSH" )
		SET( BW_LINUX_COMPILE_EXTRA "${BW_LINUX_COMPILE_EXTRA} | sed 's/\\//\\\\/g'" )
	ENDIF()
	SET( BW_LINUX_COMPILE_EXTRA "${BW_LINUX_COMPILE_EXTRA} | sed 's/\\(\\w\\+\\):\\([0-9]\\+\\):/\\1(\\2):/'" )
	SET( BW_LINUX_COMPILE_EXTRA "${BW_LINUX_COMPILE_EXTRA} | iconv -c -f UTF-8 -t ASCII" )
ENDIF()

IF( BW_IS_REMOTE_BUILD )
	STRING( REPLACE "/" "\\" BW_LINUX_SSH_PATH ${BW_LINUX_SSH_PATH} )
	
	# Create _ssh.bat
	FILE( WRITE "${CMAKE_BINARY_DIR}/_ssh.bat" "@${BW_LINUX_SSH_PATH} ${BW_LINUX_SSH_ARGS} %1" )
	
	# Construct _remote_build.bat and _remote_clean.bat strings
	SET( _LINUX_MAKE_CMD_PRE "${BW_LINUX_SSH_PATH} ${BW_LINUX_SSH_ARGS} \"(make -C ${BW_LINUX_DIR}/%1 ${BW_LINUX_CFLAGS}" )
	SET( _LINUX_MAKE_CMD_POST "MF_CONFIG=\\\"%2\\\" 2>&1) | while read line; do echo \\\"$line\\\" ${BW_LINUX_COMPILE_EXTRA}; done\" || exit %ERRORLEVEL%")
	SET( _LINUX_MAKE_COMMAND "${_LINUX_MAKE_CMD_PRE} ${_LINUX_MAKE_CMD_POST}" )
	SET( _LINUX_MAKE_COMMAND_CLEAN "${_LINUX_MAKE_CMD_PRE} clean ${_LINUX_MAKE_CMD_POST}" )
	
	# Construct rsync command if required
	IF( ${BW_LINUX_CONN_TYPE} MATCHES "RSYNC" )
		SET( _LINUX_RSYNC_COMMAND "${BW_LINUX_RSYNC_ARGS} \\\"${_PROJ_PATH}/*\\\" \\\"${BW_LINUX_USER}@${BW_LINUX_HOST}:${BW_LINUX_DIR}/%1/\\\"" )
	ENDIF()
	
	STRING( REPLACE "/" "\\" CMAKE_BINARY_DIR_WINDOWS ${CMAKE_BINARY_DIR} )
	# Create _remote_build.bat
	FILE( WRITE "${CMAKE_BINARY_DIR}/_remote_build.bat" 
		"@echo OFF
		if not exist \"${CMAKE_BINARY_DIR}/.DO_NOT_BUILD_SERVER\" (
			@echo Starting remote build of %1
			${_LINUX_RSYNC_COMMAND}
			${_LINUX_MAKE_COMMAND}
		) else (
			@echo Remote build disabled, skipping
		)" )
	
	# Create _remote_clean.bat
	FILE( WRITE "${CMAKE_BINARY_DIR}/_remote_clean.bat" 
		"@echo OFF
		if not exist \"${CMAKE_BINARY_DIR_WINDOWS}/.DO_NOT_BUILD_SERVER\" (
			${_LINUX_MAKE_COMMAND_CLEAN}
		)" )
	
	# Create _remote_rebuild.bat
	FILE( WRITE "${CMAKE_BINARY_DIR}/_remote_rebuild.bat" 
		"@echo OFF
		if not exist \"${CMAKE_BINARY_DIR_WINDOWS}/.DO_NOT_BUILD_SERVER\" (
			${CMAKE_BINARY_DIR}/_remote_clean.bat %1 %2
			${CMAKE_BINARY_DIR}/_remote_build.bat %1 %2
		)" )
	
	IF (BW_IS_REMOTE_ONLY)
		# Create the initial _update_projs.bat, which is used for changing normal 
		# Visual studio projects into NMake visual studio projects
		FILE( WRITE "${CMAKE_BINARY_DIR}/_update_projs.bat" "@echo OFF\necho Updating project files to handle clean and rebuild" )
	
		# Generate gcc_defines.h to help intellisense
		MESSAGE( STATUS "Generating gcc defines from ${BW_LINUX_HOST}" )
		EXECUTE_PROCESS( 
			COMMAND "${CMAKE_BINARY_DIR}/_ssh.bat" "echo | g++ -x c++ -dM -E -"
			OUTPUT_VARIABLE GCC_DEFINES
		)
		FILE( WRITE "${CMAKE_BINARY_DIR}/gcc_defines.h" "#undef _WIN32\n#undef _WINDOWS\n${GCC_DEFINES}" )

		IF( NOT ${BW_LINUX_CONN_TYPE} MATCHES "RSYNC" )
			# Copy system header files to help intellisense
			MESSAGE( STATUS "Syncing system includes from ${BW_LINUX_HOST} (Can take a few seconds the first time)" )
			
			# Get the include directories
			EXECUTE_PROCESS(
				COMMAND "${CMAKE_BINARY_DIR}/_ssh.bat" "echo | g++ -Wp,-v -x c++ -E - 2>&1 | grep '^ ' | sed 's/^ //' | while read x; do readlink -f $x; done"
				OUTPUT_VARIABLE BW_REMOTE_INCLUDE_DIRS_LINUX
			)
			STRING( REPLACE "\n" " " BW_REMOTE_INCLUDE_DIRS_LINUX ${BW_REMOTE_INCLUDE_DIRS_LINUX} )
			
			# Sync the include directories into build/remote/<hostname>
			EXECUTE_PROCESS( 
				COMMAND "${CMAKE_BINARY_DIR}/_ssh.bat" "mkdir -p ${BW_LINUX_DIR}/build/remote/${BW_LINUX_HOST}/"
				COMMAND "${CMAKE_BINARY_DIR}/_ssh.bat" "rsync -RLruq ${BW_REMOTE_INCLUDE_DIRS_LINUX} ${BW_LINUX_DIR}/build/remote/${BW_LINUX_HOST}/"
			)
			
			# Construct include paths for cmake
			STRING( REPLACE " " ";" BW_REMOTE_INCLUDE_DIRS_LINUX ${BW_REMOTE_INCLUDE_DIRS_LINUX} )
			SET( BW_REMOTE_INCLUDE_DIRS )
			FOREACH( INCLUDE_DIR ${BW_REMOTE_INCLUDE_DIRS_LINUX} )
				LIST( APPEND BW_REMOTE_INCLUDE_DIRS "${BW_SOURCE_DIR}/build/remote/${BW_LINUX_HOST}${INCLUDE_DIR}" )
			ENDFOREACH( INCLUDE_DIR )
		ENDIF()
		
		# Get the remote platform
		EXECUTE_PROCESS( 
			COMMAND "${CMAKE_BINARY_DIR}/_ssh.bat" "python ${BW_LINUX_DIR}/build/make/platform_info.py"
			OUTPUT_VARIABLE BW_REMOTE_PLATFORM
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	ENDIF()
ENDIF()
