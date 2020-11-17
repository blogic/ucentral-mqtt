cmake_minimum_required(VERSION 2.6)

PROJECT(usync-mqtt-cli C)
ADD_DEFINITIONS(-Os -ggdb -Wextra -Wall -Werror --std=gnu99 -Wmissing-declarations -Wno-unused-parameter)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

ADD_EXECUTABLE(usync-mqtt-cli cli.c)
TARGET_LINK_LIBRARIES(usync-mqtt-cli mosquitto uuid)

INSTALL(TARGETS usync-mqtt-cli
	RUNTIME DESTINATION sbin
)
