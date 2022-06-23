set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -O2")

add_library(rma2 SHARED librma2.c extoll2_list.c)
install(TARGETS rma2 DESTINATION lib)

add_library(rma2rc SHARED librma2rc.c)
install(TARGETS rma2rc DESTINATION lib)
