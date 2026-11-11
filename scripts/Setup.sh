#!/bin/sh
# macOS: runs Setup.py with the first Python 3 found. Arguments are passed on (see --help).
DIR="$(cd "$(dirname "$0")" && pwd)"
for PY in python3 python; do
    if command -v "$PY" >/dev/null 2>&1 && "$PY" -c 'import sys; sys.exit(sys.version_info < (3, 9))' 2>/dev/null; then
        exec "$PY" "$DIR/Setup.py" "$@"
    fi
done
echo "Setup needs Python 3.9 or newer (python3 not found)" >&2
exit 1
