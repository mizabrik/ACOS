#!/bin/bash

get_host() {
  echo "$1" | awk '{ if (match($0,/http:\/\/([^\/]+)/,m)) print m[1] }'
}

get_base() {
  echo "$1" | awk '{ if (match($0,/http:\/\/([^\/]+)\/(.*)\/.*/,m)) print m[1] }'
}

get_uri() {
  echo "$1" | awk '{ if (match($0,/http:\/\/([^\/]+)(\/.*)/,m)) print m[2] }'
}

load() {
  echo -ne "GET ${2} HTTP/1.0\r\nHost: ${1}\r\n\r\n" | nc "${1}" 80
}

host=$(get_host ${1})
uri=$(get_uri ${1})
base=$(get_base ${1})

get_host ${1}
get_uri ${1}
get_base ${1}

content=$(load "${host}" "${uri}")
#content=$(cat file)
http_links=$(echo "${content}" | awk ' { if (match($0,/<[aA][^>]* href="(http:[^"]*)"/, m)) print m[1]}')
#short_links=$(echo "${content}" | awk ' { if (match($0,/<[aA][^>]* href="(http:[^"]*)"/, m)) print m[1]}')

backIFS=IFS
IFS="
"

i=0
for link in ${http_links}; do
  echo "Loading ${link}"
  host="$(get_host "${link}")"
  uri="$(get_uri "${link}")"
  load "${host}" "${uri}" > file$i
  ((i=i+1))
done
