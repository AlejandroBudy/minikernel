#!/usr/bin/env bash
docker build -t soa .
docker run -ti --rm -v $PWD:/home/soa soa