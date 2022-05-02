#!/bin/bash

set -e

# Start mysql
sudo systemctl start mysql

APP_NAME=$1

echo "DataDir = \"../data/\"" >> ./env/dist/etc/$APP_NAME.conf
echo "LoginDatabaseInfo     = \"localhost;3306;root;root;acore_auth\"" >> ./env/dist/etc/$APP_NAME.conf
echo "WorldDatabaseInfo     = \"localhost;3306;root;root;acore_world\"" >> ./env/dist/etc/$APP_NAME.conf
echo "CharacterDatabaseInfo = \"localhost;3306;root;root;acore_characters\"" >> ./env/dist/etc/$APP_NAME.conf

if [[ $APP_NAME -eq "worldserver" ]]; then
    git clone --depth=1 --branch=master --single-branch https://github.com/ac-data/ac-data.git ./env/dist/data
fi

(cd ./env/dist/bin/ && timeout 5m ./$APP_NAME --dry-run)

# Stop mysql
sudo systemctl stop mysql
