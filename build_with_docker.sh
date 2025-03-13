#!/bin/bash
docker run -it --rm -v $(pwd):/build wayru-builder ./compile.sh
