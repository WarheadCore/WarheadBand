if ! command -v lsb_release &>/dev/null ; then
    sudo apt-get install -y lsb-release
fi

# UBUNTU_VERSION=$(lsb_release -sr);

sudo apt-get remove -y libunwind-14
sudo apt-get install -y libunwind-dev

# Added repo for newest lib
sudo apt update

# Install boost
sudo apt-get -y install libboost-all-dev

if [[ $CONTINUOUS_INTEGRATION ]]; then
  sudo apt-get -y install build-essential libtool make cmake cmake-data clang openssl \
  libssl-dev libmysqlclient-dev libmysql++-dev libreadline6-dev zlib1g-dev libbz2-dev \
  libncurses5-dev ccache
else
  sudo apt-get install -y git gcc g++ gdb gdbserver \
  libssl-dev libbz2-dev libreadline-dev libncurses-dev \
  mysql-server
fi
