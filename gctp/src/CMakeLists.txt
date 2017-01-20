cmake_minimum_required (VERSION 2.8.10)
PROJECT (GCTP_SRC C CXX)

#-----------------------------------------------------------------------------
# Setup include Directories
#-----------------------------------------------------------------------------
LINK_DIRECTORIES (
    ${GCTP_SRC_BINARY_DIR}
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

SET (GCTP_SRC_CSRCS
    ${GCTP_SRC_SOURCE_DIR}/gctp.c
    ${GCTP_SRC_SOURCE_DIR}/alberfor.c
    ${GCTP_SRC_SOURCE_DIR}/alberinv.c
    ${GCTP_SRC_SOURCE_DIR}/alconfor.c
    ${GCTP_SRC_SOURCE_DIR}/alconinv.c
    ${GCTP_SRC_SOURCE_DIR}/azimfor.c
    ${GCTP_SRC_SOURCE_DIR}/aziminv.c
    ${GCTP_SRC_SOURCE_DIR}/bceafor.c
    ${GCTP_SRC_SOURCE_DIR}/bceainv.c
    ${GCTP_SRC_SOURCE_DIR}/br_gctp.c
    ${GCTP_SRC_SOURCE_DIR}/ceafor.c
    ${GCTP_SRC_SOURCE_DIR}/ceainv.c
    ${GCTP_SRC_SOURCE_DIR}/cproj.c
    ${GCTP_SRC_SOURCE_DIR}/eqconfor.c
    ${GCTP_SRC_SOURCE_DIR}/eqconinv.c
    ${GCTP_SRC_SOURCE_DIR}/equifor.c
    ${GCTP_SRC_SOURCE_DIR}/equiinv.c
    ${GCTP_SRC_SOURCE_DIR}/for_init.c
    ${GCTP_SRC_SOURCE_DIR}/gnomfor.c
    ${GCTP_SRC_SOURCE_DIR}/gnominv.c
    ${GCTP_SRC_SOURCE_DIR}/goodfor.c
    ${GCTP_SRC_SOURCE_DIR}/goodinv.c
    ${GCTP_SRC_SOURCE_DIR}/gvnspfor.c
    ${GCTP_SRC_SOURCE_DIR}/gvnspinv.c
    ${GCTP_SRC_SOURCE_DIR}/hamfor.c
    ${GCTP_SRC_SOURCE_DIR}/haminv.c
    ${GCTP_SRC_SOURCE_DIR}/imolwfor.c
    ${GCTP_SRC_SOURCE_DIR}/imolwinv.c
    ${GCTP_SRC_SOURCE_DIR}/inv_init.c
    ${GCTP_SRC_SOURCE_DIR}/isinusfor.c
    ${GCTP_SRC_SOURCE_DIR}/isinusinv.c
    ${GCTP_SRC_SOURCE_DIR}/lamazfor.c
    ${GCTP_SRC_SOURCE_DIR}/lamazinv.c
    ${GCTP_SRC_SOURCE_DIR}/lamccfor.c
    ${GCTP_SRC_SOURCE_DIR}/lamccinv.c
    ${GCTP_SRC_SOURCE_DIR}/merfor.c
    ${GCTP_SRC_SOURCE_DIR}/merinv.c
    ${GCTP_SRC_SOURCE_DIR}/millfor.c
    ${GCTP_SRC_SOURCE_DIR}/millinv.c
    ${GCTP_SRC_SOURCE_DIR}/molwfor.c
    ${GCTP_SRC_SOURCE_DIR}/molwinv.c
    ${GCTP_SRC_SOURCE_DIR}/obleqfor.c
    ${GCTP_SRC_SOURCE_DIR}/obleqinv.c
    ${GCTP_SRC_SOURCE_DIR}/omerfor.c
    ${GCTP_SRC_SOURCE_DIR}/omerinv.c
    ${GCTP_SRC_SOURCE_DIR}/orthfor.c
    ${GCTP_SRC_SOURCE_DIR}/orthinv.c
    ${GCTP_SRC_SOURCE_DIR}/paksz.c
    ${GCTP_SRC_SOURCE_DIR}/polyfor.c
    ${GCTP_SRC_SOURCE_DIR}/polyinv.c
    ${GCTP_SRC_SOURCE_DIR}/psfor.c
    ${GCTP_SRC_SOURCE_DIR}/psinv.c
    ${GCTP_SRC_SOURCE_DIR}/report.c
    ${GCTP_SRC_SOURCE_DIR}/robfor.c
    ${GCTP_SRC_SOURCE_DIR}/robinv.c
    ${GCTP_SRC_SOURCE_DIR}/sinfor.c
    ${GCTP_SRC_SOURCE_DIR}/sininv.c
    ${GCTP_SRC_SOURCE_DIR}/sphdz.c
    ${GCTP_SRC_SOURCE_DIR}/somfor.c
    ${GCTP_SRC_SOURCE_DIR}/sominv.c
    ${GCTP_SRC_SOURCE_DIR}/sterfor.c
    ${GCTP_SRC_SOURCE_DIR}/sterinv.c
    ${GCTP_SRC_SOURCE_DIR}/stplnfor.c
    ${GCTP_SRC_SOURCE_DIR}/stplninv.c
    ${GCTP_SRC_SOURCE_DIR}/tmfor.c
    ${GCTP_SRC_SOURCE_DIR}/tminv.c
    ${GCTP_SRC_SOURCE_DIR}/untfz.c
    ${GCTP_SRC_SOURCE_DIR}/utmfor.c
    ${GCTP_SRC_SOURCE_DIR}/utminv.c
    ${GCTP_SRC_SOURCE_DIR}/vandgfor.c
    ${GCTP_SRC_SOURCE_DIR}/vandginv.c
    ${GCTP_SRC_SOURCE_DIR}/wivfor.c
    ${GCTP_SRC_SOURCE_DIR}/wivinv.c
    ${GCTP_SRC_SOURCE_DIR}/wviifor.c
    ${GCTP_SRC_SOURCE_DIR}/wviiinv.c
)

SET (GCTP_HEADERS
    ${HDFEOS_GCTP_INC_DIR}/bcea.h
    ${HDFEOS_GCTP_INC_DIR}/cproj.h
    ${HDFEOS_GCTP_INC_DIR}/cproj_prototypes.h
    ${HDFEOS_GCTP_INC_DIR}/gctp_prototypes.h
    ${HDFEOS_GCTP_INC_DIR}/proj.h
    ${HDFEOS_GCTP_INC_DIR}/isin.h
)

ADD_LIBRARY (${HDFEOS_GCTP_SRC_LIB_TARGET} ${LIB_TYPE} ${GCTP_SRC_CSRCS} ${GCTP_HEADERS})
IF(MSVC)
  TARGET_LINK_LIBRARIES (${HDFEOS_GCTP_SRC_LIB_TARGET} "ws2_32.lib")
ENDIF(MSVC)
TARGET_LINK_LIBRARIES (${HDFEOS_GCTP_SRC_LIB_TARGET} ${LINK_LIBS})
SET_GLOBAL_VARIABLE (HDFEOS_LIBRARIES_TO_EXPORT "${HDFEOS_LIBRARIES_TO_EXPORT};${HDFEOS_GCTP_SRC_LIB_TARGET}")
HDFEOS_SET_LIB_OPTIONS (${HDFEOS_GCTP_SRC_LIB_TARGET} ${HDFEOS_GCTP_SRC_LIB_NAME} ${LIB_TYPE})

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
IF (NOT HDFEOS_INSTALL_NO_DEVELOPMENT)
  INSTALL (
      FILES
          ${GCTP_HEADERS}
      DESTINATION
          ${HDFEOS_INSTALL_INCLUDE_DIR}
      COMPONENT
          headers
  )
ENDIF (NOT HDFEOS_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
IF (HDFEOS_EXPORTED_TARGETS)

  INSTALL_TARGET_PDB (${HDFEOS_GCTP_SRC_LIB_TARGET} ${HDFEOS_INSTALL_LIB_DIR} libraries)
  
  INSTALL (
      TARGETS 
          ${HDFEOS_GCTP_SRC_LIB_TARGET}
      EXPORT
          ${HDFEOS_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDFEOS_INSTALL_LIB_DIR} COMPONENT libraries 
      ARCHIVE DESTINATION ${HDFEOS_INSTALL_LIB_DIR} COMPONENT libraries
      RUNTIME DESTINATION ${HDFEOS_INSTALL_BIN_DIR} COMPONENT libraries
  )
ENDIF (HDFEOS_EXPORTED_TARGETS)
