cmake_minimum_required(VERSION 3.16)
project(homegear_nodes_extra)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(hvac/heating-controller)
include_directories(hvac/heating-mode)
include_directories(hvac/heating-cooling-controller)
include_directories(heating-cooling/heating-cooling-mode)
include_directories(shading/shading)


add_executable(homegear_nodes_extra
        alarm/alarm/Factory.cpp
        alarm/alarm/Factory.h
        alarm/alarm/Alarm.cpp
        alarm/alarm/Alarm.h
        cloud/cloud-device/Factory.cpp
        cloud/cloud-device/Factory.h
        cloud/cloud-device/CloudDevice.cpp
        cloud/cloud-device/CloudDevice.h
        hvac/heating-controller/Factory.cpp
        hvac/heating-controller/Factory.h
        hvac/heating-controller/HeatingController.cpp
        hvac/heating-controller/HeatingController.h
        hvac/heating-controller/MyNode.cpp
        hvac/heating-controller/MyNode.h
        hvac/heating-mode/Factory.cpp
        hvac/heating-mode/Factory.h
        hvac/heating-mode/MyNode.cpp
        hvac/heating-mode/MyNode.h
        hvac/heating-cooling-controller/Factory.cpp
        hvac/heating-cooling-controller/Factory.h
        hvac/heating-cooling-controller/HeatingCoolingController.cpp
        hvac/heating-cooling-controller/HeatingCoolingController.h
        hvac/heating-cooling-controller/MyNode.cpp
        hvac/heating-cooling-controller/MyNode.h
        shading/shading/Factory.cpp
        shading/shading/Factory.h
        shading/shading/MyNode.cpp
        shading/shading/MyNode.h
        shading/shading/InputEvent.h
        shading/shading/InputEvent.cpp
        shading/shading/StateMachine.h
        shading/shading/StateMachine.cpp
        shading/shading/ShadingController.h
        shading/shading/ShadingController.cpp)
