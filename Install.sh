#!/bin/bash

function show_usage
{
	echo ""
	echo "Usage: $0 [--prefix=INSTALL_PATH_PREFIX] [--debug]"
	echo ""
	echo "examples:"
	echo "    $0"
	echo "    $0 --debug"
	echo "    $0 --prefix=${HOME}/local"
	echo "    $0 --prefix=${HOME}/local-gcc-memcheck --debug"
	echo "    $0 --create-pkg"
	echo ""
}

function create_backup_pkg
{
	PKG_FILENAME="../butter-db-$(date +%Y%m%d).tgz"
	rm -f tags
	make clean || exit 1
	rm -f $PKG_FILENAME
	tar czf $PKG_FILENAME . || exit 1
	mv -f $PKG_FILENAME . || exit 1
}

INSTALL_PATH_PREFIX=""
BUILD_FOR_TEST_MODE="no"
CREATE_PKG="no"

for opt in "$@"
do
case $opt in
	-p=*|--prefix=*)
	INSTALL_PATH_PREFIX=`echo $opt | sed 's/[-a-zA-Z0-9]*=//'`
	;;
	--debug)
	BUILD_FOR_TEST_MODE="yes"
	;;
	--create-pkg)
	CREATE_PKG="yes"
	;;
	*)
	show_usage
	exit 1
	;;
esac
done

echo ""

if [[ "${CREATE_PKG}" == "yes" ]] ; then
	create_backup_pkg
	exit 0
fi

if [[ "${INSTALL_PATH_PREFIX}"x == "x" ]] ; then
	if [[ "${BUILD_FOR_TEST_MODE}" == "no" ]] ; then
		INSTALL_PATH_PREFIX="${HOME}/local"
		echo "NOTE: INSTALL_PATH_PREFIX defaults to \"${INSTALL_PATH_PREFIX}\""
	else
		INSTALL_PATH_PREFIX="${HOME}/local-gcc-memcheck"
		echo "NOTE: INSTALL_PATH_PREFIX defaults to \"${INSTALL_PATH_PREFIX}\""
	fi
fi

if [[ -d ${INSTALL_PATH_PREFIX} ]] ; then
	:
else
	mkdir -p ${INSTALL_PATH_PREFIX}
fi

if [[ -d ${INSTALL_PATH_PREFIX} ]] ; then
	:
else
	echo "ERROR: failed to create the prefix folder"
	exit 1
fi

echo "INSTALL_PATH_PREFIX=${INSTALL_PATH_PREFIX}"
echo "BUILD_FOR_TEST_MODE=${BUILD_FOR_TEST_MODE}"
echo ""

mkdir -p ${INSTALL_PATH_PREFIX}/lib || exit 1
mkdir -p ${INSTALL_PATH_PREFIX}/bin || exit 1
mkdir -p ${INSTALL_PATH_PREFIX}/include || exit 1

echo "INFO: cleaning"

make clean || exit 1

echo "INFO: building"

if [[ "${BUILD_FOR_TEST_MODE}" == "yes" ]] ; then
	make GCC=gcc debug_mode=1 -j || exit 1
else
	make -j || exit 1
fi

echo "INFO: copying files"

cp -f libbutter.so ${INSTALL_PATH_PREFIX}/lib || exit 1
cp -f butter ${INSTALL_PATH_PREFIX}/bin || exit 1
cp -f public.h ${INSTALL_PATH_PREFIX}/include || exit 1

echo "INFO: success"

echo ""

exit 0

# eof
