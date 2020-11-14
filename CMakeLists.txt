cmake_minimum_required(VERSION 2.6)

PROJECT(usync-mqtt C)
ADD_DEFINITIONS(-Os -ggdb -Wextra -Wall -Werror --std=gnu99 -Wmissing-declarations -Wno-unused-parameter)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

ADD_EXECUTABLE(usync-mqtt main.c ubus.c stats.c)

TARGET_LINK_LIBRARIES(usync-mqtt mosquitto ubox ubus blobmsg_json)

INSTALL(TARGETS usync-mqtt
	RUNTIME DESTINATION sbin
)
