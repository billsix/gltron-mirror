# Loaded as CPACK_PROJECT_CONFIG_FILE. Runs once per `cpack` invocation,
# after CPACK_GENERATOR is set, so we can branch on it.

if(CPACK_GENERATOR STREQUAL "External")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  set(CPACK_MONOLITHIC_INSTALL 1)
endif()
