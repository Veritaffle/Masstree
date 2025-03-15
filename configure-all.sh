VARIANTS=("pthread" "atomicnv_signal" "atomicnv_thread")
CONFIGS=("debug" "release")

#	TODO: produce asm
CXXFLAGS_BASE="-g -W -Wall -std=c++20 -pthread "
CXXFLAGS_DEBUG="-O0 -fsanitize=thread "
CXXFLAGS_RELEASE="-O3 "

LDFLAGS_BASE=""
LDFLAGS_DEBUG="-fsanitize=thread "
LDFLAGS_RELEASE=""

CONFIGFLAGS_BASE=""
CONFIGFLAGS_PTHREAD=""
CONFIGFLAGS_ATOMICNV_SIGNAL="--enable-atomic_nodeversion --enable-atomic_signal_fence_fences "
CONFIGFLAGS_ATOMICNV_THREAD="--enable-atomic_nodeversion --enable-atomic_thread_fence_fences "

CONFIGFLAGS_DEBUG="--with-build_config=debug"
CONFIGFLAGS_RELEASE="--with-build_config=release --disable-assertions --disable-preconditions --disable-invariants "

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