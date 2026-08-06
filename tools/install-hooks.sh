#!/usr/bin/env bash

set -eu

if ! command -v dos2unix &> /dev/null; then
    echo "dos2unix could not be found. Please install it and re-run this script." >&2
    exit 1
fi

if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found. Please install it and re-run this script." >&2
    exit 1
fi

# Get the root of the repository
project_root="$(git rev-parse --show-toplevel)"

if [ ! -f "${project_root}"/.clang-format ]; then
    echo "No .clang-format file found. Please add one to the root of your repository." >&2
    exit 1
fi

# Copy the pre-commit hook
cp "${project_root}"/tools/pre-commit.sh "${project_root}"/.git/hooks/pre-commit
chmod +x "${project_root}"/.git/hooks/pre-commit

echo "Pre-commit hook installed successfully."
