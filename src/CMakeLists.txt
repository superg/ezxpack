# link to boost statically
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.42.0 COMPONENTS system filesystem program_options REQUIRED)
if(Boost_FOUND)
	add_executable(ezxpack ezxpack.cc)
	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(ezxpack ${Boost_LIBRARIES})
	install(TARGETS ezxpack DESTINATION bin)
endif()
