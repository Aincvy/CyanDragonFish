#! /bin/env bash
BASEDIR=$(dirname "$0")
echo "script folder: $BASEDIR"
cd "$BASEDIR/../protobuf"

protoc -I=. --cpp_out=../protobuf-gen/ *.proto
echo "generate over!"