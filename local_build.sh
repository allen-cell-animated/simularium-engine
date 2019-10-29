#!/bin/bash
sourceDir=`dirname "$0"`
echo $sourceDir

if [[ -n "$sourceDir" ]]; then
  export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
  cmake $sourceDir -DBUILD_ONLY="s3;awstransfer;transfer" -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
  make
fi
