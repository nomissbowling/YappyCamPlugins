# Clock.yap
add_library(Clock SHARED Clock_yap.cpp Clock_yap.def)
set_target_properties(Clock PROPERTIES OUTPUT_NAME "Clock.yap")
set_target_properties(Clock PROPERTIES PREFIX "")
set_target_properties(Clock PROPERTIES SUFFIX "")
target_link_libraries(Clock ${OpenCV_LIBS})
