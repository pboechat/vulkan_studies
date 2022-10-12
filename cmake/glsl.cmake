function(add_glsl)
    get_target_property(OUT_DIR ${ARGV0} RUNTIME_OUTPUT_DIRECTORY)
    set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/spirv")
    math(EXPR ARG_LEN "${ARGC} - 1")
    foreach(ARG_IDX RANGE 1 ${ARG_LEN} 1)
        list(GET ARGN ${ARG_IDX} GLSL)
        get_filename_component(GLSL_FILENAME ${GLSL} NAME)
        set(SPIRV "${SPIRV_DIR}/${GLSL_FILENAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIRV_DIR}"
            COMMAND glslangValidator -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
        )
        list(APPEND GLSL_SPIRV_FILES ${SPIRV})
    endforeach(ARG_IDX)
    add_custom_target(
        ${ARGV0}_GLSL_SPIRV_FILES
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/spirv"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SPIRV_DIR}" "${OUT_DIR}/spirv"
        DEPENDS ${GLSL_SPIRV_FILES}
    )
    add_dependencies(${ARGV0} ${ARGV0}_GLSL_SPIRV_FILES)
endfunction()