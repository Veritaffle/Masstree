#!/bin/bash

# VARIANTS=("beta" "atomicnv_signal" "atomicnv_thread" "atomicnv" "atomicnv_schedyield" )
# VARIANTS=("beta" "atomicnv_signal" "atomicnv_thread" "atomicnv" "atomicnv_pause" )
# VARIANTS=("atomicnv" "atomicnv_pause" "atomicnv_schedyield")
VARIANTS=("stringbag_atomic" "stringbag_atomicref")
# VARIANTS=("atomicinternode")
# VARIANTS=("beta")

# CONFIGS=("debug" "release")
# CONFIGS=("release")
CONFIGS=("debug")

#	TODO: produce asm
CXXFLAGS_BASE="-g -W -Wall -std=c++20 -pthread "
CXXFLAGS_DEBUG="-O0 -fsanitize=thread "
CXXFLAGS_RELEASE="-O3 "

LDFLAGS_BASE=""
LDFLAGS_DEBUG="-fsanitize=thread "
LDFLAGS_RELEASE=""

CONFIGFLAGS_BASE=""
CONFIGFLAGS_BETA="--with-nodeversion=handrolled "

CONFIGFLAGS_ATOMICNV_SIGNAL="--with-nodeversion=atomicallfences "
CONFIGFLAGS_ATOMICNV_THREAD="--with-nodeversion=atomicallfences --enable-atomic_thread_fence_default "
CONFIGFLAGS_ATOMICNV_SCHEDYIELD="--with-nodeversion=atomic --with-relax_fence_pause=schedyield "
CONFIGFLAGS_ATOMICNV_PAUSE="--with-nodeversion=atomic --with-relax_fence_pause=pause "
CONFIGFLAGS_ATOMICNV="--with-nodeversion=atomic --with-relax_fence_pause=none "

CONFIGFLAGS_STRINGBAG_BASE=$CONFIGFLAGS_ATOMICNV_PAUSE
CONFIGFLAGS_STRINGBAG_ATOMIC=$CONFIGFLAGS_STRINGBAG_BASE"--with-stringbag_impl=atomic "
CONFIGFLAGS_STRINGBAG_ATOMICREF=$CONFIGFLAGS_STRINGBAG_BASE" --with-stringbag_impl=atomicref "

CONFIGFLAGS_ATOMICINTERNODE=$CONFIGFLAGS_STRINGBAG_ATOMIC"--with-internode_impl=atomic"

CONFIGFLAGS_DEBUG="--with-build_config=debug "
CONFIGFLAGS_RELEASE="--with-build_config=release --disable-assertions --disable-preconditions --disable-invariants "

set -e
trap "echo 'Caught SIGINT! Exiting...'; exit 1" SIGINT

mkdir -p "build"
cd "build/"

for variant in "${VARIANTS[@]}"; do
	for config in "${CONFIGS[@]}"; do
		BUILD_DIR="${variant}-${config}"

		CXXFLAGS="$CXXFLAGS_BASE"
		LDFLAGS="$LDFLAGS_BASE"
		CONFIGFLAGS="$CONFIGFLAGS_BASE"

		case "$config" in
			"debug")
				CXXFLAGS+=$CXXFLAGS_DEBUG
				LDFLAGS+=$LDFLAGS_DEBUG
				CONFIGFLAGS+=$CONFIGFLAGS_DEBUG
				;;
			"release")
				CXXFLAGS+=$CXXFLAGS_RELEASE
				LDFLAGS+=$LDFLAGS_RELEASE
				CONFIGFLAGS+=$CONFIGFLAGS_RELEASE
				;;
		esac

		case "$variant" in
			"beta")
				CONFIGFLAGS+=$CONFIGFLAGS_BETA
				;;
			"atomicnv_signal")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV_SIGNAL
				;;
			"atomicnv_thread")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV_THREAD
				;;
			"atomicnv_schedyield")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV_SCHEDYIELD
				;;
			"atomicnv_pause")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV_PAUSE
				;;
			"atomicnv")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV
				;;
			"stringbag_atomic")
				CONFIGFLAGS+=$CONFIGFLAGS_STRINGBAG_ATOMIC
				;;
			"stringbag_atomicref")
				CONFIGFLAGS+=$CONFIGFLAGS_STRINGBAG_ATOMICREF
				;;
			"atomicinternode")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICINTERNODE
				;;
		esac

		echo "Building $variant-$config configuration."
		echo "CXXFLAGS: $CXXFLAGS"
		echo "LDFLAGS: $LDFLAGS"
		echo "CONFIGFLAGS: $CONFIGFLAGS"
		mkdir -p "$BUILD_DIR"
		cd "$BUILD_DIR"
		../../configure CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" $CONFIGFLAGS
		cd ..
	done
done

cd ..