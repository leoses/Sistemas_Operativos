#!/bin/bash
MPOINT="./mount-point"
rm -R -f test
mkdir test
cp ./src/fuseLib.c $MPOINT
cp ./src/fuseLib.c ./test
echo "copiado archivo  fuseLib.h"
cp ./src/myFS.h $MPOINT
cp ./src/myFS.h ./test
echo "copiado archivo   myFS.h"
diff ./src/fuseLib.c $MPOINT/fuseLib.c
echo "primer diff"
diff ./src/myFS.h $MPOINT/myFS.h
echo "segundo  diff"
ls $MPOINT -la


