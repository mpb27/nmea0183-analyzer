cmake_minimum_required (VERSION 3.11)
project(nmea0183_analyzer)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK.cmake)

set(SOURCES 
src/NmeaAnalyzer.cpp
src/NmeaAnalyzer.h
src/NmeaAnalyzerResults.cpp
src/NmeaAnalyzerResults.h
src/NmeaAnalyzerSettings.cpp
src/NmeaAnalyzerSettings.h
src/NmeaSimulationDataGenerator.cpp
src/NmeaSimulationDataGenerator.h
)

add_analyzer_plugin(Nmea0183Analyzer SOURCES ${SOURCES})

