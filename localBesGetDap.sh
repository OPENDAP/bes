#!/bin/sh

#export REQUESTED_DAP_OBJ=`echo $1 | awk '{print tolower($0)}' -`
#export RESOURCE_ID=$2

export REQUESTED_DAP_OBJ="dds"
export RESOURCE_ID="/data/nc/fnoc1.nc"


show_usage () { 
    echo ""
    echo "$0               OPeNDAP"
    echo ""
    echo "NAME"
    echo ""
    echo "    $0 -- Get a DAP object from a local BES."
    echo ""
    echo "SYNOPSIS"
    echo ""
    echo "    $0 [-v] [-c bes_conf_file] -d dap_request_type -i resource_id "; 
    echo ""
    echo "DESCRIPTION"
    echo ""
    echo " Retrieves a DAP object using the besstandalone application."
    echo " Because retrieving DAP objects from the BES exercises all of"
    echo " machinery of the BES (such as caches) '$0' can easily be used "
    echo " to pre-populate caches from a cron job or some other "
    echo " programatic activity."
    echo ""
    echo "OPTIONS"
    echo "    -h, -?     Shows this page."
    echo ""
    echo "    -v  file   Verbose mode."
    echo ""
    echo "    -i  file   The data file name relative to the "
    echo "               BES.Catalog.catalog.RootDirectory property in the"
    echo "               BES configuration file."
    echo ""
    echo "    -d  dds|das|dods|dmr|dap "
    echo "               The type of the DAP object to request"
    echo ""
    echo "    -c  file   Specifies the BES configuration file"
    echo ""
    echo "OPeNDAP              December 9, 2015"
}

OPTIND=1         # Reset in case getopts has been used previously in the shell.

verbose=0

while getopts "h?vd:i:c:" opt; do
    case "$opt" in
    h|\?)
        show_usage
        exit 0
        ;;
    d)  REQUESTED_DAP_OBJ=$OPTARG
        ;;
    i)  RESOURCE_ID=$OPTARG
        ;;
    v)  verbose=1
        ;;
    c)  BES_CONF=$OPTARG
        ;;
    D)  BES_DEBUG='-d "cerr,all"'
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift


if [ -z $BES_CONF ]
then 
    echo "BES_CONF is not set trying env variable 'prefix'...";  
    if [ ! -z $prefix ] # True if var is set and str length != 0
    then
        test_conf=$prefix/etc/bes/bes.conf
        echo "Env var 'prefix' is set. Checking $test_conf"; 
        if [ -r $test_conf ]
        then
            BES_CONF="$test_conf"
            if [ $verbose = 1 ]
            then
                echo "BES_CONF: " $BES_CONF
            fi
        fi
    fi
fi   
    
if [ -z $BES_CONF ]
then     
    export BES_CONF=/etc/bes/bes.conf
    echo "Last Attempt:  Trying $BES_CONF"; 
    if [ -r $BES_CONF ]
    then
       if [ $verbose = 1 ]
        then
            echo "BES_CONF: " $BES_CONF
        fi
    else
        echo ""
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo ""
        echo "Unable to locate a BES configuration file."
        echo "You may identify your BES configurstion in one of three ways: "
        echo ""
        echo "  1. Using the command line parameter '-c'"
        echo ""
        echo "  2. Setting the environment variable $prefix to the install"
        echo "     location of the BES."
        echo ""
        echo "  3. Placing the bes.conf file in the well known location"
        echo "         /etc/bes/bes.conf"
        echo ""
        echo "The command line parameter is the recommended usage."        
        echo ""
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
         show_usage
        echo ""
       exit;
    fi
fi

if [ $verbose = 1 ]
then
    echo "REQUESTED_DAP_OBJ: $REQUESTED_DAP_OBJ"
    echo "RESOURCE_ID: $RESOURCE_ID"
fi

cmdDoc=`cat <<EOF 
<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="[http-8080-1:27:bes_request]">
  <bes:setContext name="xdap_accept">3.2</bes:setContext>
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  <bes:setContainer name="catalogContainer" space="catalog">$RESOURCE_ID</bes:setContainer>
  <bes:define name="d1" space="default">
    <bes:container name="catalogContainer"/>
  </bes:define>
  <bes:get type="$REQUESTED_DAP_OBJ" definition="d1" />
</bes:request>
EOF
`

COMMAND_FILE=$(mktemp -t getDAP_$$)
if [ $verbose = 1 ]
then
    echo "COMMAND_FILE: " $COMMAND_FILE
fi

echo $cmdDoc > $COMMAND_FILE

besstandalone -c $BES_CONF -i $COMMAND_FILE

rm $COMMAND_FILE
