/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

use anyhow::{format_err, Error};
use async_trait::async_trait;
use auto_impl::auto_impl;
use changesets::Changesets;
use context::CoreContext;
use futures::compat::Future01CompatExt;
use mononoke_types::{ChangesetId, Generation, RepositoryId};
use std::{any::Any, collections::HashMap, sync::Arc};

/// Trait that knows how to fetch DAG info about commits. Primary user is revsets
/// Concrete implementation may add more efficient caching logic to make request faster
#[async_trait]
#[auto_impl(&, Arc, Box)]
pub trait ChangesetFetcher: Send + Sync + 'static {
    async fn get_generation_number(
        &self,
        ctx: CoreContext,
        cs_id: ChangesetId,
    ) -> Result<Generation, Error>;

    async fn get_parents(
        &self,
        ctx: CoreContext,
        cs_id: ChangesetId,
    ) -> Result<Vec<ChangesetId>, Error>;

    fn get_stats(&self) -> HashMap<String, Box<dyn Any>> {
        HashMap::new()
    }
}

/// Simplest ChangesetFetcher implementation which is just a wrapper around `Changesets` object
pub struct SimpleChangesetFetcher {
    changesets: Arc<dyn Changesets>,
    repo_id: RepositoryId,
}

impl SimpleChangesetFetcher {
    pub fn new(changesets: Arc<dyn Changesets>, repo_id: RepositoryId) -> Self {
        Self {
            changesets,
            repo_id,
        }
    }
}

#[async_trait]
impl ChangesetFetcher for SimpleChangesetFetcher {
    async fn get_generation_number(
        &self,
        ctx: CoreContext,
        cs_id: ChangesetId,
    ) -> Result<Generation, Error> {
        let maybe_cs = self
            .changesets
            .get(ctx, self.repo_id.clone(), cs_id.clone())
            .compat()
            .await?;
        let cs = maybe_cs.ok_or_else(|| format_err!("{} not found", cs_id))?;
        Ok(Generation::new(cs.gen))
    }

    async fn get_parents(
        &self,
        ctx: CoreContext,
        cs_id: ChangesetId,
    ) -> Result<Vec<ChangesetId>, Error> {
        let maybe_cs = self
            .changesets
            .get(ctx, self.repo_id.clone(), cs_id.clone())
            .compat()
            .await?;
        let cs = maybe_cs.ok_or_else(|| format_err!("{} not found", cs_id))?;
        Ok(cs.parents)
    }
}
