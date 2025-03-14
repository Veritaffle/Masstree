VARIANTS=("pthread")
CONFIGS=("debug" "release")
CXXFLAGS_BASE="-g -W -Wall -std=c++20 -pthread "
#	TODO: produce asm
CXXFLAGS_DEBUG="-O0 -fsanitize=thread "
CXXFLAGS_RELEASE="-O3 "
#	TODO: these shouldn't be CXXFLAGS
CXXFLAGS_PTHREAD=""
CXXFLAGS_NV="--enable-atomic_nv"
LDFLAGS_BASE=""

cd "build/"

for variant in "${VARIANTS[@]}"; do
	for config in "${CONFIGS[@]}"; do
		BUILD_DIR="${variant}-${config}"

		CXXFLAGS="$CXXFLAGS_BASE"
		LDFLAGS="$LDFLAGS_BASE"
		case "$config" in
			"debug")
				CXXFLAGS+=$CXXFLAGS_DEBUG
				LDFLAGS+="-fsanitize=thread"
				;;
			"release")
				CXXFLAGS+=$CXXFLAGS_RELEASE
				;;
		esac

		case "$variant" in
			"pthread")
				CXXFLAGS+=$CXXFLAGS_PTHREAD
				;;
			"NV")
				CXXFLAGS+=$CXXFLAGS_NV
				;;
		esac

		echo "Building $variant-$config configuration..."
		mkdir -p "$BUILD_DIR"
		cd "$BUILD_DIR"
		../../configure CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"
		cd ..
	done
done

cd ..