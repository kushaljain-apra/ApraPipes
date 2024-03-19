vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Apra-Labs/llama.cpp
    REF bfbf41e1f0ce1dc95666ebe303093bb6329ce0b2
    SHA512 fbfae64d2b97ec1286d57271f4edf970b0c65e8b334337a04c3ef25b4b319c73e5e0d6be3ae9221eff11dfa62424e123acfbed7f2b14d782f80f0076bebdd739
    HEAD_REF kj/vcpkg-port
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
 FEATURES
 "cuda" LLAMA_CUBLAS
)

set(LLAMA_CUBLAS OFF)
if("cuda" IN_LIST FEATURES)
  set(LLAMA_CUBLAS ON)
endif()


vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(
    CONFIG_PATH lib/cmake/llama
    PACKAGE_NAME llama
    )
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()