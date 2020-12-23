#!/bin/bash

set -e

exec clang-format -style=file $@
