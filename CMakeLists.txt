cmake_minimum_required (VERSION 3.8)

project ("zbarscanner")

add_subdirectory ("zbar")
add_subdirectory ("qrcodegen")

add_executable(zbarscanner WIN32
	main.c
	result.c
	zbarscanner.rc
	zbarscanner.exe.manifest
	result.h
)

target_link_libraries(zbarscanner PUBLIC
	qrcodegen
	zbar
	winmm
	comctl32
)
