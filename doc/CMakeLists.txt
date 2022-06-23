#this file follows the tutorial here:
# https://tty1.net/blog/2014/cmake-doxygen_en.html
# but does not use a doxyfile.in


find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the API documentation (requires Doxygen and doxygen-latex)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR "Doxygen is needed to build the documentation")
	endif()

	set(doxyfile  ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile)
	set(doxyfile_in  ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)

	configure_file(${doxyfile_in} ${doxyfile} @ONLY)

	set(DOXYGEN_GENERATED ${CMAKE_CURRENT_SOURCE_DIR}/html ${CMAKE_CURRENT_SOURCE_DIR}/man ${CMAKE_CURRENT_SOURCE_DIR}/latex)

	add_custom_command(OUTPUT ${DOXYGEN_GENERATED}
		COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
		COMMAND cd latex && make
		DEPENDS ${doxyfile} ../include/rma2.h
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)

	add_custom_target(doc ALL DEPENDS ${DOXYGEN_GENERATED})

	set(LIBRMA_DOC_DIR ${CMAKE_INSTALL_PREFIX}/doc/librma2)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html ${CMAKE_CURRENT_BINARY_DIR}/man
		DESTINATION ${LIBRMA_DOC_DIR})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/latex/refman.pdf DESTINATION ${LIBRMA_DOC_DIR})
endif()
	
