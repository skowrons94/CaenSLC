#
# This module detects if CAEN libraries are installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# CAEN_DGTZ_LIBRARIES   = full path to the CAENDigitizer libraries
# CAEN_COMM_LIBRARIES   = full path to the CAENComm libraries
# CAEN_INCLUDE_DIR     = include dir to be used when using the gsoap library


# -----------------------------------------------------
# CAEN Libraries
# -----------------------------------------------------
find_library(CAEN_DGTZ_LIBRARIES
    NAMES CAENDigitizer
    HINTS ${CAEN_LOCATION}/lib ${CAEN_LOCATION}/lib64 
          ${CAEN_LOCATION}/lib32
    DOC "The main CAENDigitizer library"
)
find_library(CAEN_COMM_LIBRARIES
    NAMES CAENComm
    HINTS ${CAEN_LOCATION}/lib ${CAEN_LOCATION}/lib64 
          ${CAEN_LOCATION}/lib32
    DOC "The main CAENComm library"
)
# -----------------------------------------------------
# CAEN Include Directories
# -----------------------------------------------------
find_path(CAEN_INCLUDE_DIR 
    NAMES CAENDigitizer.h
    HINTS ${CAEN_LOCATION} ${CAEN_LOCATION}/include ${CAEN_LOCATION}/include/*
    DOC "The CAEN include directory"
)

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set CAEN_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CAEN DEFAULT_MSG CAEN_INCLUDE_DIR 
  CAEN_COMM_LIBRARIES CAEN_DGTZ_LIBRARIES)
