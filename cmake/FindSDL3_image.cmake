# Locate SDL3_image library
# This module defines
# SDL3_IMAGE_LIBRARY, the name of the library to link against
# SDL3_IMAGE_FOUND, if false, do not try to link to SDL3_image
# SDL3_IMAGE_INCLUDE_DIR, where to find SDL_image.h
#
# Additional Note: If you see an empty SDL3_IMAGE_LIBRARY_TEMP in your configuration
# and no SDL3_IMAGE_LIBRARY, it means CMake did not find your SDL3_Image library
# (SDL3_image.dll, libsdl3_image.so, SDL3_image.framework, etc).
# Set SDL3_IMAGE_LIBRARY_TEMP to point to your SDL3 library, and configure again.
# Similarly, if you see an empty SDL3MAIN_LIBRARY, you should set this value
# as appropriate. These values are used to generate the final SDL3_IMAGE_LIBRARY
# variable, but when these values are unset, SDL3_IMAGE_LIBRARY does not get created.
#
# $SDL3 is an environment variable that would
# correspond to the ./configure --prefix=$SDL3
# used in building SDL3.
# l.e.galup 9-20-02
#
# Modified by Eric Wing.
# Added code to assist with automated building by using environmental variables
# and providing a more controlled/consistent search behavior.
# Added new modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).
# Also corrected the header search path to follow "proper" SDL3 guidelines.
# Added a search for SDL3main which is needed by some platforms.
# Added a search for threads which is needed by some platforms.
# Added needed compile switches for MinGW.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL3_IMAGE_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# Note that the header path has changed from SDL3/SDL.h to just SDL.h
# This needed to change because "proper" SDL3 convention
# is #include "SDL.h", not <SDL3/SDL.h>. This is done for portability
# reasons because not all systems place things in SDL3/ (see FreeBSD).
#
# Ported by Johnny Patterson. This is a literal port for SDL3 of the FindSDL.cmake
# module with the minor edit of changing "SDL" to "SDL3" where necessary. This
# was not created for redistribution, and exists temporarily pending official
# SDL3 CMake modules.
#
# Note that on windows this will only search for the 32bit libraries, to search
# for 64bit change x86/i686-w64 to x64/x86_64-w64

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2014 Kitware, Inc.
# Copyright 2000-2011 Insight Software Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
# nor the names of their contributors may be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
# License text for the above reference.)

FIND_PATH(SDL3_IMAGE_INCLUDE_DIR SDL_image.h
	HINTS
	$ENV{SDL3_IMAGE}
	$ENV{SDL3}
	${SDL3}
	PATH_SUFFIXES include/SDL3 include SDL3
	i686-w64-mingw32/include/SDL3
	x86_64-w64-mingw32/include/SDL3
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/include/SDL3
	/usr/include/SDL3
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

# Lookup the 64 bit libs on x64
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	FIND_LIBRARY(SDL3_IMAGE_LIBRARY_TEMP
		/usr/include
		$ENV{SDL3_IMAGE}
		HINTS
		$ENV{SDL3}
		${SDL3}
		NAMES SDL3_image
		PATH_SUFFIXES lib64 lib
		lib/x64
		x86_64-w64-mingw32/lib
		PATHS
		/sw
		/opt/local
		/opt/csw
		/opt
	)
# On 32bit build find the 32bit libs
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 8)
	FIND_LIBRARY(SDL3_IMAGE_LIBRARY_TEMP
		$ENV{SDL3_IMAGE}
		NAMES SDL3_image
		HINTS
		$ENV{SDL3}
		${SDL3}
		PATH_SUFFIXES lib
		lib/x86
		i686-w64-mingw32/lib
		PATHS
		/sw
		/opt/local
		/opt/csw
		/opt
	)
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)

SET(SDL3_IMAGE_FOUND "NO")
	IF(SDL3_IMAGE_LIBRARY_TEMP)
	# Set the final string here so the GUI reflects the final state.
	SET(SDL3_IMAGE_LIBRARY ${SDL3_IMAGE_LIBRARY_TEMP} CACHE STRING "Where the SDL3_image Library can be found")
	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
	SET(SDL3_IMAGE_LIBRARY_TEMP "${SDL3_IMAGE_LIBRARY_TEMP}" CACHE INTERNAL "")
	SET(SDL3_IMAGE_FOUND "YES")
ENDIF(SDL3_IMAGE_LIBRARY_TEMP)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL3_image REQUIRED_VARS SDL3_IMAGE_LIBRARY)
