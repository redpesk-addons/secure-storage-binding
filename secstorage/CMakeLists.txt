###########################################################################
# Copyright 2020 IoT.bzh Company
#
# $RP_BEGIN_LICENSE$
# Commercial License Usage
#  Licensees holding valid commercial IoT.bzh licenses may use this file in
#  accordance with the commercial license agreement provided with the
#  Software or, alternatively, in accordance with the terms contained in
#  a written agreement between you and The IoT.bzh Company. For licensing terms
#  and conditions see https://www.iot.bzh/terms-conditions. For further
#  information use the contact form at https://www.iot.bzh/contact.
#
# GNU General Public License Usage
#  Alternatively, this file may be used under the terms of the GNU General
#  Public license version 3. This license is as published by the Free Software
#  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
#  of this file. Please review the following information to ensure the GNU
#  General Public License requirements will be met
#  https://www.gnu.org/licenses/gpl-3.0.html.
# $RP_END_LICENSE$
###########################################################################

# Add target to project dependency list
PROJECT_TARGET_ADD(service-secure-storage)
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../conf.d/cmake/)
	find_package(BerkeleyDB REQUIRED)

	include_directories(${DB_INCLUDE_DIR})
	# Define project Targets
	ADD_LIBRARY(${TARGET_NAME} MODULE secure-storage-service-binding.c)
	TARGET_LINK_LIBRARIES(${TARGET_NAME} ${DB_LIBRARY})
	#TARGET_LINK_OPTIONS(${TARGET_NAME} PUBLIC -fprofile-arcs)

	# Binder exposes a unique public entry point
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX "afb-"
		LABELS "BINDING"
		OUTPUT_NAME ${TARGET_NAME}
	)

option(SECSTOREADMIN "Enter Secure Storage Admin mode" OFF)
if (SECSTOREADMIN)
  add_definitions(-DSECSTOREADMIN=1)
endif()

option(ALLOW_SECS_GLOBAL "Enable Secure Storage global access mode" ON)
if (ALLOW_SECS_GLOBAL)
  add_definitions(-DALLOW_SECS_GLOBAL=1)
endif()