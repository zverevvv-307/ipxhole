project(ipxhole VERSION 21.0)

include(my_boost)
include(my_git)


file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable ( ${PROJECT_NAME} ${SOURCES} ${HEADERS} )
target_link_libraries (${PROJECT_NAME}
			gtcommon
			vx
			gtlan
			vxsqlite
			ipx
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GIT_REVISION="${${PROJECT_NAME}_GIT_REVISION}" )

#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)


