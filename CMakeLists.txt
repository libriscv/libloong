cmake_minimum_required(VERSION 3.5)
get_directory_property(hasParent PARENT_DIRECTORY)

if (NOT hasParent)
	project (libloong VERSION 0.1 LANGUAGES CXX)

	set(CPACK_PACKAGE_NAME libloong-dev)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "libloong contributors")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LoongArch userspace emulator library")

	set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
	set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/packages")

	set(CPACK_PACKAGE_CONTACT "libloong@example.com")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "libloong contributors <${CPACK_PACKAGE_CONTACT}>")

	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	set(CPACK_DEB_COMPONENT_INSTALL YES)

	include(CPack)
endif()

add_subdirectory(lib)

# Add tests if this is the main project
if (NOT hasParent)
	option(BUILD_TESTING "Build the testing tree" ON)
	if(BUILD_TESTING)
		add_subdirectory(tests)
	endif()
endif()
