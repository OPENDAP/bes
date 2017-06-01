#!/bin/sh
#
# Build the reference guide for this code and publish it using github.io.

DOXYGEN_CONF=doxy.conf
HTML_DOCS=html
BRANCH=`git branch | grep '*' | cut -d ' ' -f 2`

if ! git branch --list | grep gh-pages
then
    echo "Could not find gh-pages branch"
    return 1
fi

# Switch to the gh-pages branch and clean the HTML_DOCS directory if
# it exists.
git checkout -q gh-pages

if test -d $HTML_DOCS
then
    git rm -rf $HTML_DOCS
    git commit -m "Removed old docs"
fi

git checkout -q $BRANCH

# Build the docs. Puts them in a top-level dir named 'html' and that
# must match $HTML_DOCS. Edit $DOXYGEN_CONF if $HTML_DOCS changes!
doxygen $DOXYGEN_CONF

# Now switch to the gh-pages branch and commit and push the docs.

git checkout -q gh-pages

git add ${HTML_DOCS}
git commit -m "Added new docs"
git push -q

git checkout -q $BRANCH
