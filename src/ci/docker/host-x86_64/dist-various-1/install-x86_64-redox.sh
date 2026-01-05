#!/usr/bin/env bash

set -ex

curl https://ci-mirrors.-lang.org/c/2022-11-27-relibc-install.tar.gz | \
tar --extract --gzip --directory /usr/local
