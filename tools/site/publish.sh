#!/bin/sh
# Publish the generated site to the gh-pages branch, which GitHub Pages
# serves at the branch root. docs/ is build output, not history: each run
# force-pushes a single fresh commit naming the source revision.
#
#   qjsm tools/site/build.js docs build/emscripten-all
#   tools/site/publish.sh [remote]
set -e

root=$(git rev-parse --show-toplevel)
remote=${1:-origin}
test -f "$root/docs/index.html" || {
  echo "publish.sh: docs/index.html missing -- run tools/site/build.js first" >&2
  exit 1
}

# git wants a path that does not exist yet, not an empty file
idx=$(mktemp -u "${TMPDIR:-/tmp}/shish-site-index.XXXXXX")
trap 'rm -f "$idx"' EXIT

# docs/ is gitignored, hence -f
GIT_INDEX_FILE=$idx git --work-tree="$root/docs" add -Af .
tree=$(GIT_INDEX_FILE=$idx git write-tree)
commit=$(git commit-tree "$tree" -m "site built from $(git rev-parse --short HEAD)")

git push -f "$remote" "$commit:refs/heads/gh-pages"
