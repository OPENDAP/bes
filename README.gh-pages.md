
# How to build the online documentation for the bes

The online documentation uses doxygen to build the reference documentation
for the bes and then pushes those HTML pages up to a GitHub Pages website.

The process is simple, but involves some 'tricks' that make the process 
hard to implement in a robust way.

What to do:
1. Checkout the gh-pages branch of the repo. This is an orphaned branch.
2. Remove the 'html' directory and its contents.
3. Commit that change (but don't push that commit - pushing comes later).
4. Switch back to the master branch.
5. There should be no 'html' directory - if there is, remove it.
6. Build the doxygen pages, that process will make the 'html' directory
7. Switch back to the gh-pages branch
8. Add the 'html' directory (and thus its contents) to the gh-pages branch
9. Commit that change. 
10. Push the gh-pages branch
11. Switch back to master/main
12. Check that the documentation built and uploaded to the GitHb Pages site.

Here's the command sequence, assuming you are on the branch 'master':
```aiignore
git checkout gh-pages
git rm -rf html
git commit -m "Removed old docs"

git checkout master
doxygen doxy.conf
git checkout gh-pages

git add --force html
git commit -m "New docs"
git push

git checkout master
```

Then, in a browser, goto the [BES reference documentation](https://opendap.github.io/bes/html/)
(https://opendap.github.io/bes/html/).

## How to make the special 'orphaned branch' gh-pages

Assuming you are on 'master/main:'
```aiignore
git checkout --orphan gh-pages
git reset --hard
git commit --allow-empty -m "Initializing gh-pages branch"
git push origin gh-pages

git checkout master
```