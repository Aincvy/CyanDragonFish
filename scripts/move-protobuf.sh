#! /bin/env bash
BASEDIR=$(dirname "$0")
echo "script folder: $BASEDIR"
cd "$BASEDIR/../protobuf-gen"
cp -f *.h ../include/msg
cp -f *.cc ../src/msg 

echo "copy over!"