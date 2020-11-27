cmake_minimum_required(VERSION 2.6)

PROJECT(ucentral-mqtt C)
ADD_DEFINITIONS(-Os -ggdb -Wextra -Wall -Werror --std=gnu99 -Wmissing-declarations -Wno-unused-parameter)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

ADD_EXECUTABLE(ucentral-mqtt main.c ubus.c stats.c cmd.c)

TARGET_LINK_LIBRARIES(ucentral-mqtt mosquitto ubox ubus blobmsg_json)

INSTALL(TARGETS ucentral-mqtt
	RUNTIME DESTINATION sbin
)
