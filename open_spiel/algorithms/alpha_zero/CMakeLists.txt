# All commented out because we don't support depending on the C++
# TensorFlow library yet. Fixes/contributions welcome.

# add_library (alpha_zero OBJECT
#   alpha_zero.h
#   alpha_zero.cc
#   device_manager.h
#   vpevaluator.h
#   vpevaluator.cc
#   vpnet.h
#   vpnet.cc
# )
# target_include_directories (alpha_zero PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#
# add_executable(vpnet_test vpnet_test.cc ${OPEN_SPIEL_OBJECTS}
#                $<TARGET_OBJECTS:tests>)
# add_test(vpnet_test vpnet_test)
