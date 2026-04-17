#!/bin/bash
cd "$(dirname "$0")"
> output.txt

for testfile in tests/liveness/*.L2f; do
    name=$(basename "$testfile")
    actual=$(./bin/L2 -l "$testfile" 2>&1)
    expected=$(cat "$testfile")

    if [ "$actual" != "$expected" ]; then
        echo "=== DIFF: $name ===" >> output.txt
        diff <(echo "$actual") <(echo "$expected") >> output.txt
        echo "" >> output.txt
    fi
done

echo "Done. $(grep -c '=== DIFF' output.txt) test(s) with differences."