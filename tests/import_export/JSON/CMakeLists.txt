add_executable (jsonfiltertest JsonFilterTest.cpp)

target_link_libraries(jsonfiltertest Qt5::Test)
target_link_libraries(jsonfiltertest KF5::Archive KF5::XmlGui ${GSL_LIBRARIES} ${GSL_CBLAS_LIBRARIES})

IF (Qt5SerialPort_FOUND)
    target_link_libraries(jsonfiltertest Qt5::SerialPort )
ENDIF ()
IF (KF5SyntaxHighlighting_FOUND)
    target_link_libraries(jsonfiltertest KF5::SyntaxHighlighting )
ENDIF ()
#TODO: KF5::NewStuff

IF (CANTOR_LIBS_FOUND)
    target_link_libraries(jsonfiltertest ${CANTOR_LIBS} )
ENDIF ()
IF (HDF5_FOUND)
    target_link_libraries(jsonfiltertest ${HDF5_C_LIBRARIES} )
ENDIF ()
IF (FFTW_FOUND)
    target_link_libraries(jsonfiltertest ${FFTW_LIBRARIES} )
ENDIF ()
IF (NETCDF_FOUND)
    target_link_libraries(jsonfiltertest ${NETCDF_LIBRARY} )
ENDIF ()
IF (CFITSIO_FOUND)
    target_link_libraries(jsonfiltertest ${CFITSIO_LIBRARY} )
ENDIF ()
IF (USE_LIBORIGIN)
    target_link_libraries(jsonfiltertest liborigin-static )
ENDIF ()

target_link_libraries(jsonfiltertest labplot2lib)

add_test(NAME jsonfiltertest COMMAND jsonfiltertest)
