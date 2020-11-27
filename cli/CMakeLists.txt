cmake_minimum_required(VERSION 2.6)

PROJECT(ucentral-mqtt-cli C)
ADD_DEFINITIONS(-Os -ggdb -Wextra -Wall -Werror --std=gnu99 -Wmissing-declarations -Wno-unused-parameter)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

ADD_EXECUTABLE(ucentral-mqtt-cli cli.c)
TARGET_LINK_LIBRARIES(ucentral-mqtt-cli mosquitto uuid)

INSTALL(TARGETS ucentral-mqtt-cli
	RUNTIME DESTINATION sbin
)
