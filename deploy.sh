#!/usr/bin/bash

usage() { echo "Usage: $0 [-u username] -h host" 1>&2; exit 1; }

while getopts "uh:" opt; do
    case "${opt}" in
        u)
            username=${OPTARG}
            ;;
        h)
            host=${OPTARG}
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

pi_addr=$username@$host
user_dir=/tmp/$USER

# Create username specific directory on the Pi
sshpass -f .password ssh $pi_addr mkdir -p $user_dir

# Transfer all files passed as arguments
sshpass -f .password scp -O $@ $pi_addr:$user_dir
