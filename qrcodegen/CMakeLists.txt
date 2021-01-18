if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

add_library(qrcodegen STATIC
    qrcodegen.c
    stbi.c
)

target_include_directories(qrcodegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
