add_executable(joiner.out
  main.cc
  text_converter.cc
  text_converter.h
  joiner.cc
  joiner.h
)

target_link_libraries(joiner.out PRIVATE Boost::program_options rlclientlib)
