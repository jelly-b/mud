cmake_minimum_required(VERSION 3.21)

add_executable(things_tiny_id_test
	things_tiny_id_test.c
	${CMAKE_SOURCE_DIR}/tests/Unity/unity.c
)
target_include_directories(things_tiny_id_test PRIVATE
	"${CMAKE_SOURCE_DIR}/tests/Unity"
	"${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(things_tiny_id_test PRIVATE tacp)

add_test(things_tiny_id_test things_tiny_id_test)

add_executable(protocols_test
	protocols_test.c
	${CMAKE_SOURCE_DIR}/tests/Unity/unity.c
)
target_include_directories(protocols_test PRIVATE
	"${CMAKE_SOURCE_DIR}/tests/Unity"
	"${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(protocols_test PRIVATE tacp)

add_test(tacp_test tacp_test)

add_executable(tacp_test
	tacp_test.c
	${CMAKE_SOURCE_DIR}/tests/Unity/unity.c
)
target_include_directories(tacp_test PRIVATE
	"${CMAKE_SOURCE_DIR}/tests/Unity"
	"${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(tacp_test PRIVATE tacp)

add_test(tacp_test tacp_test)