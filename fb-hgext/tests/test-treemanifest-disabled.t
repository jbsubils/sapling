  $ CACHEDIR=`pwd`/hgcache
  $ PYTHONPATH=$TESTDIR/..:$PYTHONPATH
  $ export PYTHONPATH

  $ . "$TESTDIR/library.sh"

  $ hg init client1
  $ cd client1
  $ cat >> .hg/hgrc <<EOF
  > [extensions]
  > fastmanifest=$TESTDIR/../fastmanifest
  > treemanifest=$TESTDIR/../treemanifest
  > 
  > [remotefilelog]
  > reponame=master
  > cachepath=$CACHEDIR
  > usefastdatapack=True
  > 
  > [fastmanifest]
  > usetree=True
  > usecache=False
  > EOF

  $ echo a > a
  $ mkdir dir
  $ echo b > dir/b
  $ hg commit -Aqm 'initial commit'

  $ hg init ../client2
  $ cd ../client2
  $ hg pull ../client1
  pulling from ../client1
  requesting all changes
  adding changesets
  adding manifests
  adding file changes
  added 1 changesets with 2 changes to 2 files
  new changesets a8dee6dcff44
  (run 'hg update' to get a working copy)
