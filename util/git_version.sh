#!/bin/sh
git describe --long --always --dirty 2>/dev/null
if [ $? -ne 0 ]
then
    echo unknown
fi
