INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIR})
add_executable (differentiationtest DifferentiationTest.cpp ../AnalysisTest.cpp)

target_link_libraries(differentiationtest Qt5::Test)
target_link_libraries(differentiationtest KF5::Archive KF5::XmlGui ${GSL_LIBRARIES} ${GSL_CBLAS_LIBRARIES})

target_link_libraries(differentiationtest labplot2lib)

add_test(NAME differentiationtest COMMAND differentiationtest)
