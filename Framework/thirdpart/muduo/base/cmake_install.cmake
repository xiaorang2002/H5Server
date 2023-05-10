# Install script for directory: /mnt/hgfs/H5Server/Framework/thirdpart/muduo/base

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/lib/libmuduo_base.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/base" TYPE FILE FILES
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/AsyncLogging.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Atomic.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/BlockingQueue.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/BoundedBlockingQueue.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Condition.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/CountDownLatch.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/CurrentThread.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Date.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Exception.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/FileUtil.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/GzipFile.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/LogFile.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/LogStream.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Logging.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Mutex.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/ProcessInfo.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Singleton.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/StringPiece.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Thread.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/ThreadLocal.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/ThreadLocalSingleton.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/ThreadPool.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/TimeZone.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Timestamp.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/Types.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/WeakCallback.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/copyable.h"
    "/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/noncopyable.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/mnt/hgfs/H5Server/Framework/thirdpart/muduo/base/tests/cmake_install.cmake")

endif()

