#!/bin/bash

usage() { echo "Usage: $0 [-u username] [-p port] -h host" 1>&2; exit 1; }

while getopts "u:p:h:" opt; do
    case "${opt}" in
        u)
            username=${OPTARG}
            ;;
        h)
            host=${OPTARG}
            ;;
        p)
            port=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $(expr $OPTIND - 1)

if [ -z "${host}" ]; then
    usage
fi

if [ ! -f "./.password" ]; then
    echo "You need to write the root password in a .password file in this directory." 1>&2; exit 1;
fi

# If no username was given then use root
if [ -z "${username}" ]; then
    username=root
fi

# If no port was given use 22
if [ -z "${port}" ]; then
    port=22
fi

pi_addr=$username@$host
user_dir=/tmp/$USER

# Create username specific directory on the Pi
sshpass -f .password ssh $pi_addr -p $port mkdir -p $user_dir

# Transfer all files passed as arguments
sshpass -f .password scp -P $port -O $@ $pi_addr:$user_dir
