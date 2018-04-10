MACRO(ATDM_SET_CACHE VAR_NAME DEFAULT_VAL CACHE_STR DATA_TYPE)
  IF (ATDM_CONFIG_VERBOSE_DEBUG)
    MESSAGE("-- SET(${VAR_NAME} \"${DEFAULT_VAL}\" ${CACHE_STR} ${DATA_TYPE})")
  ENDIF()
  SET(${VAR_NAME} "${DEFAULT_VAL}" ${CACHE_STR} ${DATA_TYPE}
    "Set in ${CMAKE_CURRENT_LIST_FILE}")
ENDMACRO()

MACRO(ATDM_SET_ENABLE  VAR_NAME  VAR_DEFAULT)
  IF ("${${VAR_DEFAULT}}" STREQUAL "")
    MESSAGE("-- " "Setting default ${VAR_NAME}=${VAR_DEFAULT}")
    SET(${VAR_NAME} ${VAR_DEFAULT} CACHE BOOL
      "Set in ${CMAKE_CURRENT_LIST_FILE}")
  ENDIF()
  IF (ATDM_CONFIG_VERBOSE_DEBUG)
    PRINT_VAR(${VAR_NAME})
  ENDIF()
ENDMACRO()

# Sets the var 'ATDM_${VAR_BASE_NAME}' from
# '$ENV{ATDM_CONFIG_${VAR_BASE_NAME}}' or if not set gives a default.
MACRO(ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT VAR_BASE_NAME VAR_DEFAULT_VAL)
  SET(ATDM_${VAR_BASE_NAME} "$ENV{ATDM_CONFIG_${VAR_BASE_NAME}}")
  IF ("${ATDM_${VAR_BASE_NAME}}" STREQUAL "")
    SET(ATDM_${VAR_BASE_NAME} "${VAR_DEFAULT_VAL}")
  ENDIF()
  IF (ATDM_CONFIG_VERBOSE_DEBUG)
    PRINT_VAR(ATDM_${VAR_BASE_NAME})
  ENDIF()
ENDMACRO()

