#!/bin/bash

FILENAME="../butter-db-$(date +%Y%m%d-%a-%H%M%S).tgz"
echo creating $FILENAME ...

make clean || exit 1
tar czf $FILENAME . || exit 1

echo ""

# eof
