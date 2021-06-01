if ! command -v lsb_release &>/dev/null ; then
    sudo apt-get install -y lsb-release
fi

UBUNTU_VERSION=$(lsb_release -sr);

sudo apt update

# Insstall boost 1.74 from ppa:mhier/libboost-latest for all os versions
sudo apt-get -y install libboost1.74-dev

if [[ $CONTINUOUS_INTEGRATION ]]; then
  sudo apt-get -y install build-essential libtool make cmake cmake-data clang openssl libgoogle-perftools-dev \
  libssl-dev libmysqlclient-dev libmysql++-dev libreadline6-dev zlib1g-dev libbz2-dev mysql-client \
  libncurses5-dev ccache
else
  sudo apt-get install -y git gcc g++ gdb gdbserver \
  libssl-dev libbz2-dev libreadline-dev libncurses-dev \
  mysql-server
fi
