# lfs - hash-preserving large file support using Git-LFS protocol
#
# Copyright 2017 Facebook, Inc.
#
# This software may be used and distributed according to the terms of the
# GNU General Public License version 2 or any later version.

"""lfs - large file support

Configs::

    [lfs]
    # Remote endpoint. Multiple protocols are supported:
    # - http(s)://user:pass@example.com/path
    #   git-lfs endpoint
    # - file:///tmp/path
    #   local filesystem, usually for testing
    # if unset, lfs will prompt setting this when it must use this value.
    # (default: unset)
    url = https://example.com/lfs

    # size of a file to make it use LFS
    threshold = 10M
"""

from __future__ import absolute_import

from mercurial import (
    changegroup,
    cmdutil,
    context,
    exchange,
    extensions,
    filelog,
    revlog,
    scmutil,
    vfs as vfsmod,
)
from mercurial.i18n import _

from . import (
    blobstore,
    wrapper,
)

cmdtable = {}
command = cmdutil.command(cmdtable)

def reposetup(ui, repo):
    # Nothing to do with a remote repo
    if not repo.local():
        return

    threshold = repo.ui.configbytes('lfs', 'threshold', None)

    repo.svfs.options['lfsthreshold'] = threshold
    repo.svfs.lfslocalblobstore = blobstore.local(repo)
    repo.svfs.lfsremoteblobstore = blobstore.remote(repo)

    # Push hook
    repo.prepushoutgoinghooks.add('lfs', wrapper.prepush)

def wrapfilelog(filelog):
    wrapfunction = extensions.wrapfunction

    wrapfunction(filelog, 'addrevision', wrapper.filelogaddrevision)
    wrapfunction(filelog, 'renamed', wrapper.filelogrenamed)
    wrapfunction(filelog, 'size', wrapper.filelogsize)

def extsetup(ui):
    wrapfilelog(filelog.filelog)

    wrapfunction = extensions.wrapfunction
    wrapfunction(changegroup,
                 'supportedoutgoingversions',
                 wrapper.supportedoutgoingversions)
    wrapfunction(changegroup,
                 'allsupportedversions',
                 wrapper.allsupportedversions)

    wrapfunction(context.basefilectx, 'cmp', wrapper.filectxcmp)
    wrapfunction(context.basefilectx, 'isbinary', wrapper.filectxisbinary)
    context.basefilectx.islfs = wrapper.filectxislfs

    revlog.addflagprocessor(
        revlog.REVIDX_EXTSTORED,
        (
            wrapper.readfromstore,
            wrapper.writetostore,
            wrapper.bypasscheckhash,
        ),
    )

    # Make bundle choose changegroup3 instead of changegroup2. This affects
    # "hg bundle" command. Note: it does not cover all bundle formats like
    # "packed1". Using "packed1" with lfs will likely cause trouble.
    names = [k for k, v in exchange._bundlespeccgversions.items() if v == '02']
    for k in names:
        exchange._bundlespeccgversions[k] = '03'

    # bundlerepo uses "vfsmod.readonlyvfs(othervfs)", we need to make sure lfs
    # options and blob stores are passed from othervfs to the new readonlyvfs.
    wrapfunction(vfsmod.readonlyvfs, '__init__', wrapper.vfsinit)

@command('debuglfsupload',
         [('r', 'rev', [], _('upload large files introduced by REV'))])
def debuglfsupload(ui, repo, **opts):
    """upload lfs blobs added by the working copy parent or given revisions"""
    revs = opts.get('rev', [])
    pointers = wrapper.extractpointers(repo, scmutil.revrange(repo, revs))
    wrapper.uploadblobs(repo, pointers)
