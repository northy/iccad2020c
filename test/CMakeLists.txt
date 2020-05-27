add_executable(TestVcdParse test_parse_vcd.cxx)
target_link_libraries(TestVcdParse
  sources
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

configure_file(mux_kernel.cl mux_kernel.cl COPYONLY)

add_executable(TestOpenCL test_opencl.cxx)
target_link_libraries(TestOpenCL
  sources
  OpenCL
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)