# VARIANTS=("pthread" "nv")
# CONFIGS=("debug" "release")
VARIANTS=("pthread")
CONFIGS=("debug")
CXXFLAGS_BASE="-g -W -Wall "
#	TODO: produce asm
CXXFLAGS_DEBUG="-O0 "
CXXFLAGS_RELEASE="-O3 "
CXXFLAGS_PTHREAD=""
CXXFLAGS_NV="--enable-atomic_nv"

cd "build/"

for variant in "${VARIANTS[@]}"; do
	for config in "${CONFIGS[@]}"; do
		BUILD_DIR="${variant}-${config}"
		INSTALL_DIR="$PWD/${BUILD_DIR}/install"

		CXXFLAGS="$CXXFLAGS_BASE"
		case "$config" in
			"debug") CXXFLAGS+=$CXXFLAGS_DEBUG ;;
			"release") CXXFLAGS+=$CXXFLAGS_RELEASE ;;
		esac

		case "$variant" in
			"pthread") CXXFLAGS+=$CFLAGS_PTHREAD ;;
			"NV") CXXFLAGS+=$CFLAGS_NV ;;
		esac

		# echo "$CXXFLAGS"

		echo "Building $variant-$config configuration..."
		mkdir -p "$BUILD_DIR"
		cd "$BUILD_DIR"
		../../configure CXXFLAGS="$CXXFLAGS" --prefix="$INSTALL_DIR"
		# make -j$(nproc)
		# make install
		cd ..
	done
done

cd ..