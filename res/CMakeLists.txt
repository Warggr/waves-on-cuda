function(compile_spirv in_file out_file)
    add_custom_command(
        OUTPUT ${out_file}
        COMMAND glslc ${in_file} -o ${out_file}
        DEPENDS ${in_file}
        VERBATIM # enables escaping; generally a good practice
    )
endfunction()

# see https://jeremimucha.com/2021/05/cmake-managing-resources/
compile_spirv(${CMAKE_CURRENT_SOURCE_DIR}/shader.frag fragment_shader.spv)
compile_spirv(${CMAKE_CURRENT_SOURCE_DIR}/shader.vert vertex_shader.spv)

add_custom_target(resources ALL DEPENDS vertex_shader.spv fragment_shader.spv)
