add_executable(TestVcdParse test_parse_vcd.cxx)
target_link_libraries(TestVcdParse
  sources
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)
