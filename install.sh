#!/opt/local/bin/bash
APPNAME=Panther
APPDIR=/Applications/${APPNAME}.app
DIR=${APPDIR}/Contents/MacOS
mkdir -p ${DIR}
cp build/panther ${DIR}/${APPNAME}
cp -r data ${APPDIR}/data
cp os/macos/Info.plist ${DIR}/../Info.plist
chmod +x ${DIR}/${APPNAME}
echo ${DIR}/${APPNAME}
