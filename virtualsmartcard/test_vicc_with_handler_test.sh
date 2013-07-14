#! /bin/sh

HANDLER_TEST="${HANDLER_TEST:=handler_test}"

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

if test -z "$HANDLER_TEST"
then
    if test -n "`which handler_test`"
    then
        HANDLER_TEST=handler_test
    else
        echo "handler_test not found"
        exit 1
    fi
fi

for flag in -A -T
do
    $VICC --type handler_test --port 35963 --reversed >/dev/null 2>&1 &
    VICC_PID=$!
    if test $? -ne 0
    then
        echo "could not start vicc"
        exit 1
    fi

    sleep 1

    $HANDLER_TEST \
        -l src/vpcd/.libs/libvpcd.so \
        -1 -2 -3 -4 $flag \
        -d localhost:35963
    HANDLER_TEST_RESULT=$?

    kill $VICC_PID 2>/dev/null

    if test $HANDLER_TEST_RESULT -ne 0
    then
        echo "could not perform test"
        exit 1
    fi
done
