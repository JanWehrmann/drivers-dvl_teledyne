include_directories(../src)

rock_testsuite(test_suite suite.cpp
  test_Driver.cpp
  DEPS dvl_teledyne)
