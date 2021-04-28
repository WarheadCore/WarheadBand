if ! command -v lsb_release &>/dev/null ; then
  sudo apt-get install -y lsb-release
fi

DEBIAN_VERSION=$(lsb_release -sr)

sudo apt-get update -y

if [[ $DEBIAN_VERSION -eq "10" ]]; then
  sudo apt-get install -y git cmake make gcc g++ clang default-libmysqlclient-dev \
  libssl-dev libbz2-dev libreadline-dev libncurses-dev mariadb-server \
  libboost-system1.7*-dev libboost-filesystem1.7*-dev libboost-program-options1.7*-dev libboost-iostreams1.7*-dev \
  curl unzip
else # Debian 8 and 9 should work using this
  sudo apt-get install -y git cmake make gcc g++ clang libmysqlclient-dev \
  libssl1.0-dev libbz2-dev libreadline-dev libncurses-dev \
  libboost-system1.7*-dev libboost-filesystem1.7*-dev libboost-program-options1.7*-dev libboost-iostreams1.7*-dev \
  mysql-server url unzip
fi
