#!/bin/bash
sourceDir=`dirname "$0"`
echo $sourceDir

if [[ -n "$sourceDir" ]]; then
  export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
  # ENABLE_UNITY_BUILD is for AWS CPP SDK.  Turn it on for faster build but requires more memory to build.
  cmake $sourceDir -DBUILD_ONLY="s3;awstransfer;transfer" -DENABLE_UNITY_BUILD=OFF -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DAUTORUN_UNIT_TESTS=OFF
  make
  mkcert -install
  mkcert localhost
  openssl dhparam -out dh.pem 2048
fi
