# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/MarineRadarSim_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/MarineRadarSim_autogen.dir/ParseCache.txt"
  "MarineRadarSim_autogen"
  )
endif()
