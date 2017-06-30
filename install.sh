#!/opt/local/bin/bash
APPNAME=Panther
DIR=/Applications/${APPNAME}.app/Contents/MacOS
mkdir -p ${DIR}
cp build/panther ${DIR}/${APPNAME}
cp -r data ${DIR}/data
chmod +x ${DIR}/${APPNAME}
echo ${DIR}/${APPNAME}
