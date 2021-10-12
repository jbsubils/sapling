/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

use std::num::NonZeroU64;

use async_trait::async_trait;

use edenapi_types::CommitGraphEntry;
use edenapi_types::CommitKnownResponse;
use edenapi_types::{
    AnyFileContentId, AnyId, BonsaiChangesetContent, BookmarkEntry, CloneData,
    CommitHashLookupResponse, CommitHashToLocationResponse, CommitLocationToHashRequest,
    CommitLocationToHashResponse, CommitRevlogData, EdenApiServerError, EphemeralPrepareResponse,
    FetchSnapshotRequest, FetchSnapshotResponse, FileEntry, FileSpec, HgFilenodeData,
    HgMutationEntryContent, HistoryEntry, LookupResponse, TreeAttributes, TreeEntry,
    UploadHgChangeset, UploadToken, UploadTokensResponse, UploadTreeEntry, UploadTreeResponse,
};
use minibytes::Bytes;
use types::{HgId, Key, RepoPathBuf};

use crate::errors::EdenApiError;
use crate::response::{Response, ResponseMeta};

#[async_trait]
pub trait EdenApi: Send + Sync + 'static {
    async fn health(&self) -> Result<ResponseMeta, EdenApiError> {
        Err(EdenApiError::NotSupported)
    }

    async fn capabilities(&self, repo: String) -> Result<Vec<String>, EdenApiError> {
        let _ = repo;
        Err(EdenApiError::NotSupported)
    }

    async fn files(
        &self,
        repo: String,
        keys: Vec<Key>,
    ) -> Result<Response<FileEntry>, EdenApiError> {
        let _ = (repo, keys);
        Err(EdenApiError::NotSupported)
    }

    async fn files_attrs(
        &self,
        repo: String,
        reqs: Vec<FileSpec>,
    ) -> Result<Response<FileEntry>, EdenApiError> {
        let _ = (repo, reqs);
        Err(EdenApiError::NotSupported)
    }

    async fn history(
        &self,
        repo: String,
        keys: Vec<Key>,
        length: Option<u32>,
    ) -> Result<Response<HistoryEntry>, EdenApiError> {
        let _ = (repo, keys, length);
        Err(EdenApiError::NotSupported)
    }

    async fn trees(
        &self,
        repo: String,
        keys: Vec<Key>,
        attributes: Option<TreeAttributes>,
    ) -> Result<Response<Result<TreeEntry, EdenApiServerError>>, EdenApiError> {
        let _ = (repo, keys, attributes);
        Err(EdenApiError::NotSupported)
    }

    async fn complete_trees(
        &self,
        repo: String,
        rootdir: RepoPathBuf,
        mfnodes: Vec<HgId>,
        basemfnodes: Vec<HgId>,
        depth: Option<usize>,
    ) -> Result<Response<Result<TreeEntry, EdenApiServerError>>, EdenApiError> {
        let _ = (repo, rootdir, mfnodes, basemfnodes, depth);
        Err(EdenApiError::NotSupported)
    }

    async fn commit_revlog_data(
        &self,
        repo: String,
        hgids: Vec<HgId>,
    ) -> Result<Response<CommitRevlogData>, EdenApiError> {
        let _ = (repo, hgids);
        Err(EdenApiError::NotSupported)
    }

    async fn clone_data(&self, repo: String) -> Result<CloneData<HgId>, EdenApiError> {
        let _ = repo;
        Err(EdenApiError::NotSupported)
    }

    async fn pull_fast_forward_master(
        &self,
        repo: String,
        old_master: HgId,
        new_master: HgId,
    ) -> Result<CloneData<HgId>, EdenApiError> {
        let _ = (repo, old_master, new_master);
        Err(EdenApiError::NotSupported)
    }

    async fn full_idmap_clone_data(&self, repo: String) -> Result<CloneData<HgId>, EdenApiError> {
        let _ = repo;
        Err(EdenApiError::NotSupported)
    }

    async fn commit_location_to_hash(
        &self,
        repo: String,
        requests: Vec<CommitLocationToHashRequest>,
    ) -> Result<Vec<CommitLocationToHashResponse>, EdenApiError> {
        let _ = (repo, requests);
        Err(EdenApiError::NotSupported)
    }

    async fn commit_hash_to_location(
        &self,
        repo: String,
        master_heads: Vec<HgId>,
        hgids: Vec<HgId>,
    ) -> Result<Vec<CommitHashToLocationResponse>, EdenApiError> {
        let _ = (repo, master_heads, hgids);
        Err(EdenApiError::NotSupported)
    }

    /// Return a subset of commits that are known by the server.
    /// This is similar to the "known" command in HG wireproto.
    async fn commit_known(
        &self,
        repo: String,
        hgids: Vec<HgId>,
    ) -> Result<Response<CommitKnownResponse>, EdenApiError> {
        let _ = (repo, hgids);
        Err(EdenApiError::NotSupported)
    }

    /// Return part of the commit graph that are ancestors of `heads`,
    /// excluding ancestors of `common`.
    ///
    /// This is `heads % common` (or `only(heads, common)`) expressed using HG
    /// revset, or `changelog.findmissing` in HG Python code base.
    ///
    /// If any of the nodes in `heads` and `common` are unknown by the server,
    /// it should be an error.
    async fn commit_graph(
        &self,
        repo: String,
        heads: Vec<HgId>,
        common: Vec<HgId>,
    ) -> Result<Vec<CommitGraphEntry>, EdenApiError> {
        let _ = (repo, heads, common);
        Err(EdenApiError::NotSupported)
    }

    /// Return matching full hashes of hex hash prefix
    async fn hash_prefixes_lookup(
        &self,
        repo: String,
        prefixes: Vec<String>,
    ) -> Result<Response<CommitHashLookupResponse>, EdenApiError> {
        let _ = (repo, prefixes);
        Err(EdenApiError::NotSupported)
    }

    async fn bookmarks(
        &self,
        repo: String,
        bookmarks: Vec<String>,
    ) -> Result<Response<BookmarkEntry>, EdenApiError> {
        let _ = (repo, bookmarks);
        Err(EdenApiError::NotSupported)
    }

    /// Lookup items and return signed upload tokens if an item has been uploaded
    /// Supports: file content, hg filenode, hg tree, hg changeset
    async fn lookup_batch(
        &self,
        repo: String,
        items: Vec<AnyId>,
        bubble_id: Option<NonZeroU64>,
    ) -> Result<Response<LookupResponse>, EdenApiError> {
        let _ = (repo, items, bubble_id);
        Err(EdenApiError::NotSupported)
    }

    /// Upload files content
    async fn process_files_upload(
        &self,
        repo: String,
        data: Vec<(AnyFileContentId, Bytes)>,
        bubble_id: Option<NonZeroU64>,
    ) -> Result<Response<UploadToken>, EdenApiError> {
        let _ = (repo, data, bubble_id);
        Err(EdenApiError::NotSupported)
    }

    /// Upload list of hg filenodes
    async fn upload_filenodes_batch(
        &self,
        repo: String,
        items: Vec<HgFilenodeData>,
    ) -> Result<Response<UploadTokensResponse>, EdenApiError> {
        let _ = (repo, items);
        Err(EdenApiError::NotSupported)
    }

    /// Upload list of trees
    async fn upload_trees_batch(
        &self,
        repo: String,
        items: Vec<UploadTreeEntry>,
    ) -> Result<Response<UploadTreeResponse>, EdenApiError> {
        let _ = (repo, items);
        Err(EdenApiError::NotSupported)
    }

    /// Upload list of changesets
    async fn upload_changesets(
        &self,
        repo: String,
        changesets: Vec<UploadHgChangeset>,
        mutations: Vec<HgMutationEntryContent>,
    ) -> Result<Response<UploadTokensResponse>, EdenApiError> {
        let _ = (repo, changesets, mutations);
        Err(EdenApiError::NotSupported)
    }

    async fn upload_bonsai_changeset(
        &self,
        repo: String,
        changeset: BonsaiChangesetContent,
        bubble_id: Option<NonZeroU64>,
    ) -> Result<Response<UploadTokensResponse>, EdenApiError> {
        let _ = (repo, changeset, bubble_id);
        Err(EdenApiError::NotSupported)
    }

    async fn ephemeral_prepare(
        &self,
        repo: String,
    ) -> Result<Response<EphemeralPrepareResponse>, EdenApiError> {
        let _ = repo;
        Err(EdenApiError::NotSupported)
    }

    /// Fetch information about the requested snapshot
    async fn fetch_snapshot(
        &self,
        repo: String,
        request: FetchSnapshotRequest,
    ) -> Result<Response<FetchSnapshotResponse>, EdenApiError> {
        let _ = (repo, request);
        Err(EdenApiError::NotSupported)
    }

    /// Download single file from upload token
    async fn download_file(&self, repo: String, token: UploadToken) -> Result<Bytes, EdenApiError> {
        let _ = (repo, token);
        Err(EdenApiError::NotSupported)
    }
}
