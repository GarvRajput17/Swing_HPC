cmake_minimum_required(VERSION 3.5)
project(sst_swing_worker)

# Find SST CMake package if available
# You may need to set SST_DIR to your SST install or adjust include/link paths.
find_package(SST REQUIRED)

add_library(SwingWorker SHARED SwingWorker.cc)
target_include_directories(SwingWorker PRIVATE ${SST_INCLUDE_DIRS} ..)
target_link_libraries(SwingWorker ${SST_LIBRARIES})
install(TARGETS SwingWorker DESTINATION lib)
