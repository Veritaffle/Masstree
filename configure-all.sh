#!/bin/bash

# VARIANTS=("pthread" "atomicnv_signal" "atomicnv_thread" "atomicnv" "atomicnv-schedyield")
# CONFIGS=("debug" "release")
VARIANTS=("atomicnv_schedyield" "atomicnv")
CONFIGS=("release")

#	TODO: produce asm
CXXFLAGS_BASE="-g -W -Wall -std=c++20 -pthread "
CXXFLAGS_DEBUG="-O0 -fsanitize=thread "
CXXFLAGS_RELEASE="-O3 "

LDFLAGS_BASE=""
LDFLAGS_DEBUG="-fsanitize=thread "
LDFLAGS_RELEASE=""

CONFIGFLAGS_BASE=""
CONFIGFLAGS_PTHREAD=""
CONFIGFLAGS_ATOMICNV_SIGNAL="--with-nodeversion=atomicallfences  "
CONFIGFLAGS_ATOMICNV_THREAD="--with-nodeversion=atomicallfences --enable-atomic_thread_fence_default "
CONFIGFLAGS_ATOMICNV_SCHEDYIELD="--with-nodeversion=atomic --enable-relax_fence_sched_yield "
CONFIGFLAGS_ATOMICNV="--with-nodeversion=atomic "

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
			"pthread")
				CONFIGFLAGS+=$CONFIGFLAGS_PTHREAD
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
			"atomicnv")
				CONFIGFLAGS+=$CONFIGFLAGS_ATOMICNV
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