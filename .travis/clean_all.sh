#!/usr/bin/env sh

for directory in `find $TRAVIS_BUILD_DIR -maxdepth 1 -mindepth 1 -type d -not -name .svn`
do
  echo $directory
  cd $directory
  if make clean
  then
    echo Success!
  else
    cd -
    exit 1
    fi
    cd -
done
