
if ! command -v lsb_release &>/dev/null ; then
  sudo apt-get install -y lsb-release
fi

UBUNTU_VERSION=$(lsb_release -sr);

sudo apt-get update -y

if [[ $CONTINUOUS_INTEGRATION ]]; then
  sudo apt-get -y install build-essential libtool make cmake cmake-data clang openssl libgoogle-perftools-dev \
  libssl-dev libmysqlclient-dev libmysql++-dev libreadline6-dev zlib1g-dev libbz2-dev mysql-client \
  libboost-system1.7*-dev libboost-filesystem1.7*-dev libboost-program-options1.7*-dev libboost-iostreams1.7*-dev \
  libncurses5-dev ccache
else
  sudo apt-get install -y git gcc g++ \
  libssl-dev libbz2-dev libreadline-dev libncurses-dev \
  libboost-system1.7*-dev libboost-filesystem1.7*-dev libboost-program-options1.7*-dev libboost-iostreams1.7*-dev \
  mysql-server
fi

