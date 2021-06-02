#!/bin/bash

set -e

# Start mysql
sudo systemctl start mysql

# Start import
./acore.sh "db-assembler" "import-all"

# Stop mysql
sudo systemctl stop mysql