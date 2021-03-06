#! /bin/bash
#
# A majority of these test cases were adapted from a piazza post URL:
# https://piazza.com/class/ij4sm86h4q5w3?cid=8
#

function should_fail() {
    result=$?;

    echo -n "==> $1 ";

    if [ $result -lt 1 ]; then
	echo "FAILURE";
	exit 1;
    else
	echo;
    fi
}

function should_succeed() {
    result=$?;

    echo -n "==> $1 ";
    if [ $result -gt 0 ]; then
	echo "FAILURE";
	exit 1;
    else
	echo;
    fi
}

tmp_file=~/foo
tmp_file2=~/foo2
> "$tmp_file"
> "$tmp_file2"


./simpsh --rdonly cantpossiblyexist 2>&1 | grep "Error in opening file cantpossiblyexist" > /dev/null
should_succeed "reports missing file";


./simpsh --rdonly Makefile | grep "Error: Makefile does not exist" > /dev/null;
should_fail "does not report file that exists"


./simpsh --verbose --command 1 2 3 echo foo 2>&1 | grep "ERROR in File Descriptor Options: 1 2 3" > /dev/null
should_succeed "using a non existent file descriptor should report the error"


> "$tmp_file"
(./simpsh \
     --verbose \
     --wronly "$tmp_file" \
     --command 1 2 3 echo foo 2>&1 \
				     --command 0 0 0 echo foo ) 2>/dev/null 1>/dev/null

grep "foo" "$tmp_file" > /dev/null
should_succeed "commands after failing commands should succeed"


./simpsh --verbose --command 2>&1 | grep "Not enough arguments for --command, found 0 need 4" > /dev/null
should_fail "empty command should have no options"


./simpsh --verbose --command 1 2 3 2>&1 | grep "Not enough arguments for --command, found 3 need 4" > /dev/null
should_succeed "command requires at least 4 arguments"



./simpsh \
    --wronly ~/foo \
    --verbose \
    --command 0 0 0 echo foo \
    | grep "command 0 0 0 echo foo" > /dev/null
should_succeed "command tracks all command options"


./simpsh --wronly ~/foo --command 0 0 0 echo "foo";
grep foo ~/foo > /dev/null;
should_succeed "command can write to write only file"


> "$tmp_file"
echo_path=$(which echo);
./simpsh --wronly ~/foo --command 0 0 0 "$echo_path" "foo";
grep foo ~/foo > /dev/null;
should_succeed "path command can write to write only file"


> "$tmp_file"
./simpsh --rdonly ~/foo --command 0 0 0 echo "foo";
grep foo ~/foo > /dev/null;
# NOTE that failure of `echo "foo"` end up in stderr
should_fail "shouldn't be able to write to read only file"


echo "foo" > "$tmp_file"
cat "$tmp_file" | wc -l | grep 1 > /dev/null
should_succeed "the temporary file should have one line"
# the cat of $tmp_file should be empty and not add another line to tmp_file
./simpsh --wronly "$tmp_file" --command 0 0 0 cat "$tmp_file"
cat "$tmp_file" | wc -l | grep 1 > /dev/null
should_succeed "shouldn't be able to write to read only file"


echo "foo" > "$tmp_file"
echo "bar" > "$tmp_file2"

cat "$tmp_file" | grep "foo" > /dev/null
should_succeed "the temporary file should have 'foo'"

cat "$tmp_file2" | grep "bar" > /dev/null
should_succeed "the temporary file 2 should have 'bar'"

# cat of ~/foo should end up in the ~/file2

./simpsh --rdonly "$tmp_file" --command 0 0 0 echo "foo"
echo "$?" | grep "0" > /dev/null
should_succeed "exit status of failed subcommand should be the exit status of simpsh"


echo "5\n4\n3\n2\n1" > ~/list.txt
./simpsh --verbose --rdonly ~/list.txt --wronly "$tmp_file" --command 0 1 0 sort -u > "$tmp_file2"
grep -- "--rdonly" "$tmp_file2" > /dev/null
should_succeed "should find verbose output of rdonly"
grep -- "--wronly"  "$tmp_file2" > /dev/null
should_succeed "should find verbose output of wronly"
grep -- "--command 0 1 0 sort -u"  "$tmp_file2" > /dev/null
should_succeed "should find verbose output of command"

./simpsh \
    --verbose \
    --rdonly ~/list.txt \
    --wronly "$tmp_file" \
    --command 0 1 0 sort -u \
    --wronly "$tmp_file" \
    > "$tmp_file"

grep "command 0 1 0 sort -u"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of command"
tail -1 "$tmp_file" | grep "wronly"  "$tmp_file" > /dev/null
should_succeed "should find wronly after command (order of options)"

./simpsh --verbose --rdonly ~/foo --wronly ~/foo --command 0 1 1 uniq -c > /dev/null
should_succeed "will read and write to a single file"



./simpsh \
    --verbose \
    --rdonly ~/list.txt \
    --wronly "$tmp_file" \
    --command 0 1 0 sort -u \
    --wronly "$tmp_file2" \
    > "$tmp_file"

grep -- "--command 0 1 0 sort -u"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of command"
tail -1 "$tmp_file" | grep --  "--wronly $tmp_file2"  "$tmp_file" > /dev/null
should_succeed "should find wronly after command (order of options)"

echo "B" >> a
echo "A" >> a
echo "hi" >> b


./simpsh \
    --rdonly a \
    --pipe \
    --pipe \
    --creat --trunc --wronly c \
    --creat --append --wronly d \
    --command 3 5 6 tr A-Z a-z \
    --command 0 2 6 sort \
    --command 1 4 6 cat b - \
    --wait > "$tmp_file"
grep -- "0 tr A-Z a-z"  "$tmp_file" > /dev/null
should_succeed "should find verbose output of command"
grep --  "0 sort"  "$tmp_file" > /dev/null
should_succeed "wait works"
grep --  "0 cat b -"  "$tmp_file" > /dev/null
should_succeed "wait works"
grep -- "a" c > /dev/null
should_succeed "wait works"




rm a
rm b
rm c
rm d
rm ~/foo
rm ~/foo2
rm ~/list.txt
echo "Success"

