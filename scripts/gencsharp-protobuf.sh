#! /bin/env bash
BASEDIR=$(dirname "$0")
echo "script folder: $BASEDIR"
cd "$BASEDIR/../protobuf"

protoc -I=. --csharp_out=../protobuf-client/ *.proto
echo "generate over!"