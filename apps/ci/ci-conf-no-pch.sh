#!/bin/bash

set -e

cat >>conf/config.sh <<CONFIG_SH
MTHREADS=$(($(grep -c ^processor /proc/cpuinfo) + 2))
CWARNINGS=ON
CDEBUG=OFF
CTYPE=Release
CSCRIPTS=static
CBUILD_TESTING=ON
CSERVERS=ON
CTOOLS=ON
CSCRIPTPCH=OFF
CCOREPCH=OFF
CCUSTOMOPTIONS='-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_FLAGS="-Werror" -DCMAKE_CXX_FLAGS="-Werror"'
DB_CHARACTERS_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
DB_AUTH_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
DB_WORLD_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
CONFIG_SH

case $COMPILER in

  # this is in order to use the "default" clang version of the OS, without forcing a specific version
  "clang" )
    time sudo apt-get install -y clang
    echo "CCOMPILERC=\"clang\"" >> ./conf/config.sh
    echo "CCOMPILERCXX=\"clang++\"" >> ./conf/config.sh
    ;;

  "clang11" )
    time sudo apt-get install -y clang-11
    echo "CCOMPILERC=\"clang-11\"" >> ./conf/config.sh
    echo "CCOMPILERCXX=\"clang++-11\"" >> ./conf/config.sh
    ;;

  * )
    echo "Unknown compiler $COMPILER"
    exit 1
    ;;
esac

if [[ $EXTRA_LOGS ]]; then
  echo "CEXTRA_LOGS=1" >> ./conf/config.sh
else
  echo "CEXTRA_LOGS=0" >> ./conf/config.sh
fi
