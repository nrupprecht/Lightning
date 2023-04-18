# Local cmake file.

file(GLOB APPLICATIONS_CPP *.cpp)

foreach(_SOURCE ${APPLICATIONS_CPP})
    get_filename_component(_NAME ${_SOURCE} NAME_WE)
    add_executable(${_NAME} ${_SOURCE})
    target_link_libraries(${_NAME} ${PROJECT_NAME})
    assign_source_group(${_SOURCE})
endforeach()

