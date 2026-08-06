#!/bin/bash

# Find all C/C++ source files
files=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|cpp|h|hpp)$' | grep -v '^Drivers/' | grep -v '^Middlewares/')

if [ -z "${files}" ]; then
    exit 0
fi

for file in ${files}; do
    dos2unix "${file}" > /dev/null 2>&1
    clang-format -style=file -i "${file}"
    git add "${file}"
done

echo "Code formatted with clang-format."

exit 0
