cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0003 NEW)

project(SMHasher C CXX)

include(CheckCCompilerFlag)
# Check if the same compile family is used for both C and CXX
if (NOT (CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID))
    message(WARNING "CMAKE_C_COMPILER_ID (${CMAKE_C_COMPILER_ID}) is different "
                    "from CMAKE_CXX_COMPILER_ID (${CMAKE_CXX_COMPILER_ID}). "
                    "The final binary may be unusable.")
endif()
IF (CMAKE_MAJOR_VERSION GREATER 2)
  cmake_policy(SET CMP0056 OLD)
  cmake_policy(SET CMP0065 NEW) # ENABLE_EXPORTS
  cmake_policy(SET CMP0066 OLD)

  include (TestBigEndian)
  TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
  if(IS_BIG_ENDIAN)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -DBIG_ENDIAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBIG_ENDIAN")
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
       "Choose the type of build, options are: Debug Release
  RelWithDebInfo MinSizeRel."
  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
   EXEC_PROGRAM(cat ARGS "/proc/cpuinfo" OUTPUT_VARIABLE CPUINFO)
   STRING(REGEX REPLACE "^.*(sse2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "sse2" "${SSE_THERE}" SSE2_TRUE)
   STRING(REGEX REPLACE "^.*(sse4_2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "sse4_2" "${SSE_THERE}" SSE42_TRUE)
   set(FALKHASH_OBJ      falkhash-elf64.o)
   STRING(REGEX REPLACE "^.*(aes).*$" "\\1" AES_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "aes" "${AES_THERE}" AES_TRUE)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
   EXEC_PROGRAM("/usr/sbin/sysctl -n machdep.cpu.features" OUTPUT_VARIABLE
      CPUINFO)
   STRING(REGEX REPLACE "^.*[^S](SSE2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "SSE2" "${SSE_THERE}" SSE2_TRUE)
   STRING(REGEX REPLACE "^.*(SSE4.2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "SSE4.2" "${SSE_THERE}" SSE42_TRUE)
   set(FALKHASH_OBJ      falkhash-macho64.o)
   STRING(REGEX REPLACE "^.*(AES).*$" "\\1" AES_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "AES" "${AES_THERE}" AES_TRUE)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
   EXEC_PROGRAM("grep Features /var/run/dmesg.boot" OUTPUT_VARIABLE
      CPUINFO)
   STRING(REGEX REPLACE "^.*[^S](SSE2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "SSE2" "${SSE_THERE}" SSE2_TRUE)
   STRING(REGEX REPLACE "^.*(SSE4.2).*$" "\\1" SSE_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "SSE4.2" "${SSE_THERE}" SSE42_TRUE)
   set(FALKHASH_OBJ      falkhash-elf64.o)
   STRING(REGEX REPLACE "^.*(AES).*$" "\\1" AES_THERE ${CPUINFO})
   STRING(COMPARE EQUAL "AES" "${AES_THERE}" AES_TRUE)
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

if (CMAKE_COMPILER_IS_GNUCC
    OR (CMAKE_C_COMPILER_ID STREQUAL AppleClang)
    OR (CMAKE_C_COMPILER_ID STREQUAL Clang)
    OR (CMAKE_C_COMPILER_ID STREQUAL Intel))
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  set(CMAKE_C_FLAGS_DEBUG "-g")
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
  IF (SSE2_TRUE)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -msse2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
  ENDIF (SSE2_TRUE)
  IF (SSE42_TRUE)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -msse4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4")
  ENDIF (SSE42_TRUE)
  IF (AES_TRUE)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -maes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
  ENDIF (AES_TRUE)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /arch:SSE2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
else()
  message(WARNING "Unknown ${CMAKE_C_COMPILER_ID} compiler")
# TODO: BSD and ARM8 crc detection
# TODO: 32bit filter
endif()

IF (SSE2_TRUE)
   set(SSE2_FOUND true CACHE BOOL "SSE2 available")
ELSE (SSE2_TRUE)
   set(SSE2_FOUND false CACHE BOOL "SSE2 not available")
ENDIF (SSE2_TRUE)
IF (SSE42_TRUE)
   set(SSE4_2_FOUND true CACHE BOOL "SSE4.2 available")
ELSE (SSE42_TRUE)
  set(SSE4_2_FOUND false CACHE BOOL "SSE4.2 not available")
ENDIF (SSE42_TRUE)
IF (AES_TRUE)
   set(AES_FOUND true CACHE BOOL "AES-NI available")
ELSE (AES_TRUE)
  set(AES_FOUND false CACHE BOOL "AES-NI not available")
ENDIF (AES_TRUE)

IF(AES_FOUND AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
  set(SSE4_OBJ        ${FALKHASH_OBJ})
ENDIF()

IF(SSE4_2_FOUND)
  SET(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(SSE2_SRC        hasshe2.c)
  set(SSE4_SRC        crc32_hw.c)
# 64bit only:
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  #enable_language(ASM-NASM)
  #set(CMAKE_ASM_FLAGS "-f elf64")
  set(SSE4_SRC1       crc32_hw1.c)
  set(METROCRC64_SRC  metrohash64crc.cpp)
  set(METROCRC128_SRC metrohash128crc.cpp)
ENDIF()
  set(SIPHASH_SRC     siphash_ssse3.c)
ELSEIF(SSE2_FOUND)
  set(SSE2_SRC        hasshe2.c)
  set(SIPHASH_SRC     siphash_sse2.c)
ELSE()
  set(SIPHASH_SRC     siphash.c)
ENDIF(SSE4_2_FOUND)
set(SIPHASH_SRC     siphash.c)

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(64ONLY_SRC
    xxhash.c metrohash64.cpp metrohash128.cpp farmhash.cc
    FarmTest.cc farmhash-c.c farmhash-c-test.cc cmetrohash64.c
    opt_cmetrohash64_1.c fasthash.cpp)
ENDIF()

IF(EXISTS "./BeagleHashes_test.c")
    set(BEAGLE_SRC BeagleHashes_test.c)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_BEAGLE_HASHES")
ENDIF()
IF(EXISTS "./tsip.c")
    set(TSIP_SRC tsip.c)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_TSIP")
ENDIF()

add_library(SMHasherSupport
  AvalancheTest.cpp
  Bitslice.cpp
  Bitvec.cpp
  DifferentialTest.cpp
  KeysetTest.cpp
  Platform.cpp
  Random.cpp
  RunTests.cpp
  SpeedTest.cpp
  Stats.cpp
  TAP.cpp
  Types.cpp
  Hashes.cpp
  
  City.cpp
  CityTest.cpp
  MurmurHash1.cpp
  MurmurHash2.cpp
  MurmurHash3.cpp
  PMurHash.c
  Spooky.cpp
  SpookyTest.cpp
  SuperFastHash.cpp
  crc.cpp
  lookup3.cpp
  md5.cpp
  mum.cc
  sha1.cpp
  t1ha.c
  
  ${64ONLY_SRC}
  ${BEAGLE_SRC}
  ${METROCRC128_SRC}
  ${METROCRC64_SRC}
  ${SIPHASH_SRC}
  ${SSE2_SRC}
  ${SSE4_OBJ}
  ${SSE4_SRC1}
  ${SSE4_SRC}
  ${TSIP_SRC}
)

add_executable(
  SMHasher
  main.cpp
)

target_link_libraries(
  SMHasher
  SMHasherSupport
  ${CMAKE_THREAD_LIBS_INIT}
  gsl
  gslcblas
)
