#
# Common Functions
#
set(generate_hash_file_py "${CMAKE_SOURCE_DIR}/utils/resources/generate_hash_file.py")
set(resources_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")

function(add_resource source_path install_path)
  set(one_value_args IMAGE_NAME)
  cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})
  if(NOT ARG_IMAGE_NAME)
    set(ARG_IMAGE_NAME resources-image)
  endif()
  lfs_image_add_file(${ARG_IMAGE_NAME} "${source_path}" "${install_path}")
endfunction()

function(add_gzip_resource source_path install_path)
  set(gzip_file_path "${CMAKE_CURRENT_BINARY_DIR}/gzipped${install_path}")
  gzip_file("${source_path}" "${gzip_file_path}")
  add_resource("${gzip_file_path}" "${install_path}" ${ARGN})
endfunction()

function(add_header_with_revision revision_name image_name)
  # generate hash (to a file)
  set(binary_hash_file "${CMAKE_CURRENT_BINARY_DIR}/resources_${revision_name}_hash.bin")
  lfs_image_generate_hash_bin_file(${image_name} "${binary_hash_file}")

  # generate header with the hash
  set(header_hash_file "${resources_include_dir}/resources/revision_${revision_name}.hpp")
  add_custom_command(
    OUTPUT "${header_hash_file}"
    COMMAND
      "${Python3_EXECUTABLE}" "${generate_hash_file_py}" "--header-variable-name" "${revision_name}"
      "--header-namespace-name" "buddy::resources::revision" "${binary_hash_file}"
      "${header_hash_file}"
    DEPENDS "${binary_hash_file}" "${generate_hash_file_py}"
    VERBATIM
    )

  # set dependency on the main target
  add_custom_target(resources-header-${revision_name} DEPENDS "${header_hash_file}")
  add_dependencies(firmware resources-header-${revision_name})
endfunction()

#
# General
#

target_sources(firmware PUBLIC bootstrap.cpp revision.cpp hash.cpp)
target_include_directories(firmware PUBLIC "${resources_include_dir}")

#
# Standard Resources Image
#

add_lfs_image(resources-image BLOCK_SIZE 256 BLOCK_COUNT 3000)

# /esp directory
add_resource("esp/uart_wifi.bin" "/esp/uart_wifi.bin")
add_resource("esp/bootloader.bin" "/esp/bootloader.bin")
add_resource("esp/partition-table.bin" "/esp/partition-table.bin")

# /web directory
add_gzip_resource(
  "web/e5ae0f11eb9a619ec903488e77be89b8.svg" "/web/e5ae0f11eb9a619ec903488e77be89b8.svg"
  )
add_gzip_resource("web/favicon.ico" "/web/favicon.ico")
add_gzip_resource("web/index.html" "/web/index.html")
add_gzip_resource("web/main.77a2a5989f980e626c6f.js" "/web/main.77a2a5989f980e626c6f.js")
add_gzip_resource("web/main.a0710e06cbc7ab353115.css" "/web/main.a0710e06cbc7ab353115.css")

# generate header with hash
add_header_with_revision(standard resources-image)

#
# Bootloader Resources Image
#
if(BOOTLOADER_UPDATE)

  add_lfs_image(resources-bootloader-image BLOCK_SIZE 256 BLOCK_COUNT 1000)

  get_dependency_directory("bootloader-${BOOTLOADER_VARIANT}" bootloader_dir)
  get_dependency_version("bootloader-${BOOTLOADER_VARIANT}" bootloader_version_str)
  set(bootloader_bin "${bootloader_dir}/bootloader.bin")

  # add the bootloader.bin to the image
  add_resource("${bootloader_bin}" "/bootloader.bin" IMAGE_NAME resources-bootloader-image)

  # generate header with hash
  add_header_with_revision(bootloader resources-bootloader-image)

  # parse bootloader version
  string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" result "${bootloader_version_str}")
  if(NOT result)
    message(FATAL_ERROR "Failed to parse bootloader version ${bootloader_version_str}")
  endif()
  set(bootloader_version ${CMAKE_MATCH_0})

  # generate header file with bootloader version
  configure_file(
    required_bootloader_version.hpp.in "${resources_include_dir}/bootloader/required_version.hpp"
    )

endif()
