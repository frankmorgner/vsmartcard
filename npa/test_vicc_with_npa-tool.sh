#! /bin/sh

CVCA=cvca.cvcert
DVCA=dvca.cvcert
TERMINAL=at.cvcert
TERM_KEY=at_key.pkcs8

ACCESS=ef_cardaccess
SECURITY=ef_cardsecurity
NPA_KEY=npa_key

if test -z "$DATA"
then
    DATA=../virtualsmartcard/npa-example-data
fi

if test -z "$VICC"
then
    if test -r ../virtualsmartcard/src/vpicc/vicc.in -a -n "`which python`"
    then
        VICC="python ../virtualsmartcard/src/vpicc/vicc.in"
    elif test -x ../virtualsmartcard/src/vpicc/vicc
    then
        VICC=../virtualsmartcard/src/vpicc/vicc
    elif test -n "`which vicc`"
    then
        VICC=vicc
    else
        echo "vicc not found"
        exit 1
    fi
fi

if test -z "$NPA_TOOL"
then
    if test -x ../npa/src/npa-tool
    then
        NPA_TOOL=../npa/src/npa-tool
    elif test -n "`which npa-tool`"
    then
        NPA_TOOL=npa-tool
    else
        echo "npa-tool not found"
        exit 1
    fi
fi

for KA in ecdh dh
do
    $VICC --type nPA --ef-cardsecurity=$DATA/$KA/$SECURITY \
        --ef-cardaccess=$DATA/$KA/$ACCESS --ca-key=$DATA/$KA/$NPA_KEY \
        --cvca=$DATA/$KA/$CVCA --disable-checks &
    VICC_PID=$!
    if test $? -ne 0
    then
        echo "could not start vicc"
        exit 1
    fi

    sleep 1

    $NPA_TOOL --pin=111111 --private-key=$DATA/$KA/$TERM_KEY \
        --cv-certificate=$DATA/$KA/$DVCA,$DATA/$KA/$TERMINAL --disable-checks
    NPA_RESULT=$?

    kill $VICC_PID 2>/dev/null

    if test $NPA_RESULT -ne 0
    then
        echo "could not perform EAC"
        exit 1
    fi
done
