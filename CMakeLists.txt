# cmake_minimum_required(VERSION 3.10)

# project(Sensor_local_client VERSION 1.0)

# set ( PROJECT_LINK_LIBS libcoap-3-openssl.so )
# find_package(OpenCV 4 REQUIRED)
# find_package(raspicam REQUIRED)

# link_directories( /usr/local/lib )

# include_directories(includes)

# file(GLOB SOURCES "src/*.c")

# IF  (OpenCV_FOUND AND raspicam_CV_FOUND)
# 	MESSAGE(STATUS "COMPILING OPENCV TESTS")
# 	add_executable (local_client ${SOURCES})  
# 	target_link_libraries (local_client m pthread ${PROJECT_LINK_LIBS} ${raspicam_CV_LIBS} ${OpenCV_LIBS})
# ELSE()
# 	MESSAGE(FATAL_ERROR "OPENCV NOT FOUND IN YOUR SYSTEM")
# ENDIF()


# TEMPLATE FOR CPP
cmake_minimum_required(VERSION 3.10)

project(Sensor_local_client VERSION 1.0)

set ( PROJECT_LINK_LIBS libcoap-3-openssl.so )
find_package(OpenCV 4 REQUIRED)
find_package(raspicam REQUIRED)

link_directories( /usr/local/lib )

include_directories(includes)

file(GLOB SOURCES "src/*.cpp")

IF  (OpenCV_FOUND AND raspicam_CV_FOUND)
	MESSAGE(STATUS "COMPILING OPENCV TESTS")
	add_executable (local_client ${SOURCES})  
	target_link_libraries (local_client m pthread ${PROJECT_LINK_LIBS} ${raspicam_CV_LIBS} ${OpenCV_LIBS})
ELSE()
	MESSAGE(FATAL_ERROR "OPENCV NOT FOUND IN YOUR SYSTEM")
ENDIF()
