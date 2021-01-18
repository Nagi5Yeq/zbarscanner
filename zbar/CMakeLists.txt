if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Iconv REQUIRED)

add_library(zbar STATIC
    decoder.c
    error.c
    image.c
    img_scanner.c
    refcnt.c
    scanner.c
    sqcode.c
    symbol.c
    decoder/qr_finder.c
    decoder/sq_finder.c
    qrcode/bch15_5.c
    qrcode/binarize.c
    qrcode/isaac.c
    qrcode/qrdec.c
    qrcode/rs.c
    qrcode/util.c
    qrcode/qrdectxt.c
)

target_include_directories(zbar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/qrcode
    ${Iconv_INCLUDE_DIR}
)

target_compile_definitions(zbar PUBLIC
    -DZBAR_VERSION_MAJOR=0
    -DZBAR_VERSION_MINOR=23
    -DZBAR_VERSION_PATCH=1
    -D_CRT_SECURE_NO_WARNINGS
)

target_compile_options(zbar PUBLIC
    /wd4554
    /wd4146
    /wd4018
)

target_link_libraries(zbar PUBLIC
    Iconv::Iconv
)
