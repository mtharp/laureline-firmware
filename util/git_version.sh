#!/bin/sh
git describe --always --dirty 2>/dev/null
if [ $? -ne 0 ]
then
    echo unknown
fi
