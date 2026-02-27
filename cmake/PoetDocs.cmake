option(POET_GENERATE_DOCS "Generate documentation (Doxygen + Sphinx)" OFF)

if(POET_GENERATE_DOCS)
    find_package(Doxygen REQUIRED)
    find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build REQUIRED)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    # Optional: Check for breathe/exhale availability
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import breathe, exhale"
        RESULT_VARIABLE DOCS_DEPS_CHECK_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT DOCS_DEPS_CHECK_RESULT EQUAL 0)
        message(WARNING "Python packages 'breathe' and 'exhale' not found. Docs generation may fail. Please run 'pip install -r docs/requirements.txt'.")
    endif()

    configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_BINARY_DIR}/docs/Doxyfile @ONLY)

    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs
        COMMENT "Generating API documentation with Doxygen"
    )

    add_custom_target(sphinx
        DEPENDS doxygen
        COMMAND ${CMAKE_COMMAND} -E env DOXYGEN_XML_OUTPUT=${CMAKE_BINARY_DIR}/docs/xml
                ${SPHINX_BUILD_EXECUTABLE} -b html
                ${CMAKE_SOURCE_DIR}/docs ${CMAKE_BINARY_DIR}/docs/_build/html
        COMMENT "Generating HTML documentation with Sphinx"
    )

    add_custom_target(docs DEPENDS sphinx)
    message(STATUS "POET: Documentation targets enabled (doxygen, sphinx, docs)")
endif()
