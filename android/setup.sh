# Run this script before building in Android Studio.

XU4_DL=http://xu4.sourceforge.net/download
SDK_ARCHIVE=xu4-android-sdk-1.1.tar.bz2

if [ ! -f Android.mk ]; then
	echo "setup.sh must be run from the android directory!"
	exit 2
fi

mkdir -p assets
# cp ../render.pak ../*.mod assets

if [ ! -d ../src/glv/android ]; then
	git submodule init
	git submodule update ../src/glv
fi

if [ ! -d sdk ]; then
	if [ ! -f ${SDK_ARCHIVE} ]; then
		curl -sSL -o ${SDK_ARCHIVE} ${XU4_DL}/${SDK_ARCHIVE}
	fi
	tar xjf ${SDK_ARCHIVE}
fi
