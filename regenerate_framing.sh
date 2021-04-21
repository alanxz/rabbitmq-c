#!/bin/bash

set -e

RMQ_VERSION=3.8.14
DATA=${PWD}/codegen/rabbitmq-server-${RMQ_VERSION}/deps/rabbitmq_codegen/amqp-rabbitmq-0.9.1.json
export PYTHONPATH=${PWD}/codegen/rabbitmq-server-${RMQ_VERSION}/deps/rabbitmq_codegen

rm -rf codegen
mkdir codegen

wget -c https://github.com/rabbitmq/rabbitmq-server/releases/download/v${RMQ_VERSION}/rabbitmq-server-${RMQ_VERSION}.tar.xz -O - | tar -xJ -C codegen

python librabbitmq/codegen.py header ${DATA} include/rabbitmq-c/framing.h
python librabbitmq/codegen.py body ${DATA} librabbitmq/amqp_framing.c

clang-format -i include/rabbitmq-c/framing.h librabbitmq/amqp_framing.c