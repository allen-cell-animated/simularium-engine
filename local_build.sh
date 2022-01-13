#!/bin/bash
sourceDir=`dirname "$0"`
echo $sourceDir

if [[ -n "$sourceDir" ]]; then
  export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
  cmake $sourceDir -DBUILD_ONLY="s3;awstransfer;transfer" -DENABLE_UNITY_BUILD=OFF -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DAUTORUN_UNIT_TESTS=OFF
  make
  mkcert -install
  mkcert localhost
  openssl dhparam -out dh.pem 2048
fi
