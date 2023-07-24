# Local cmake file.

file(GLOB APPLICATIONS_CPP *.cpp)

foreach(_SOURCE ${APPLICATIONS_CPP})
    get_filename_component(_NAME ${_SOURCE} NAME_WE)

    add_executable(${_NAME} ${_SOURCE})
    set_target_properties(${_NAME} PROPERTIES LINKER_LANGUAGE CXX)

    #target_link_libraries(${_NAME} ${PROJECT_NAME})
    target_include_directories(${_NAME} PUBLIC include)
    target_link_libraries(
            ${_NAME} PRIVATE
            Lightning::Lightning
    )
    target_compile_features(${_NAME} PRIVATE cxx_std_17)

endforeach()

