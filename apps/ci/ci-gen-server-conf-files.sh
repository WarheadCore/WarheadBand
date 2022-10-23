APP_NAME=$1
CONFIG_FOLDER=${2:-"etc"}
BIN_FOLDER=${3-"bin"}
MYSQL_ROOT_PASSWORD=${4:-""}

echo "AuthDatabaseInfo     = \"localhost;3306;root;$MYSQL_ROOT_PASSWORD;warhead_auth\"" >> ./env/dist/$CONFIG_FOLDER/$APP_NAME.conf

# worldserver or dbimport
if [[ $APP_NAME != "authserver" ]]; then
    {
        echo "WorldDatabaseInfo     = \"localhost;3306;root;$MYSQL_ROOT_PASSWORD;warhead_world\""
        echo "CharactersDatabaseInfo = \"localhost;3306;root;$MYSQL_ROOT_PASSWORD;warhead_characters\""
    } >> ./env/dist/$CONFIG_FOLDER/$APP_NAME.conf
fi

if [[ $APP_NAME == "worldserver" ]]; then
    echo "DataDir = \"./data/\"" >> ./env/dist/$CONFIG_FOLDER/$APP_NAME.conf

    git clone --depth=1 --branch=master --single-branch https://github.com/ac-data/ac-data.git ./env/dist/$BIN_FOLDER/data
fi
