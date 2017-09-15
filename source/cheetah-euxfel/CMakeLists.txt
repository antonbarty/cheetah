
find_package(HDF5 REQUIRED C HL)

mark_as_advanced(CLEAR HDF5_HL_LIBRARIES)

LIST(APPEND sources "cheetah-euxfel.cpp")
LIST(APPEND sources "agipd_reader.cpp")
LIST(APPEND sources "agipd_module_reader.cpp")
LIST(APPEND sources "PNGFile.cpp")
LIST(APPEND sources "TextManager.cpp")

include_directories(${CHEETAH_INCLUDES} ${HDF5_INCLUDE_DIR})

add_executable(cheetah-euxfel ${sources})

add_dependencies(cheetah-euxfel cheetah)

target_link_libraries(cheetah-euxfel ${CHEETAH_LIBRARY} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} )

install(TARGETS cheetah-euxfel
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
