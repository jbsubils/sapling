/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

#pragma once

#ifndef _WIN32

#include "eden/fs/inodes/InodeNumber.h"
#include "eden/fs/nfs/rpc/Rpc.h"

/*
 * Nfsd protocol described in RFC1813:
 * https://tools.ietf.org/html/rfc1813
 */

namespace facebook::eden {

constexpr uint32_t kNfsdProgNumber = 100003;
constexpr uint32_t kNfsd3ProgVersion = 3;

/**
 * Procedure values.
 */
enum class nfsv3Procs : uint32_t {
  null = 0,
  getattr = 1,
  setattr = 2,
  lookup = 3,
  access = 4,
  readlink = 5,
  read = 6,
  write = 7,
  create = 8,
  mkdir = 9,
  symlink = 10,
  mknod = 11,
  remove = 12,
  rmdir = 13,
  rename = 14,
  link = 15,
  readdir = 16,
  readdirplus = 17,
  fsstat = 18,
  fsinfo = 19,
  pathconf = 20,
  commit = 21,
};

enum class nfsstat3 : uint32_t {
  NFS3_OK = 0,
  NFS3ERR_PERM = 1,
  NFS3ERR_NOENT = 2,
  NFS3ERR_IO = 5,
  NFS3ERR_NXIO = 6,
  NFS3ERR_ACCES = 13,
  NFS3ERR_EXIST = 17,
  NFS3ERR_XDEV = 18,
  NFS3ERR_NODEV = 19,
  NFS3ERR_NOTDIR = 20,
  NFS3ERR_ISDIR = 21,
  NFS3ERR_INVAL = 22,
  NFS3ERR_FBIG = 27,
  NFS3ERR_NOSPC = 28,
  NFS3ERR_ROFS = 30,
  NFS3ERR_MLINK = 31,
  NFS3ERR_NAMETOOLONG = 63,
  NFS3ERR_NOTEMPTY = 66,
  NFS3ERR_DQUOT = 69,
  NFS3ERR_STALE = 70,
  NFS3ERR_REMOTE = 71,
  NFS3ERR_BADHANDLE = 10001,
  NFS3ERR_NOT_SYNC = 10002,
  NFS3ERR_BAD_COOKIE = 10003,
  NFS3ERR_NOTSUPP = 10004,
  NFS3ERR_TOOSMALL = 10005,
  NFS3ERR_SERVERFAULT = 10006,
  NFS3ERR_BADTYPE = 10007,
  NFS3ERR_JUKEBOX = 10008
};

namespace detail {

/**
 * Shorthand struct to inherit from for variant over nfsstat3. The following XDR
 * definition:
 *
 *     union COMMIT3res switch (nfsstat3 status) {
 *      case NFS3_OK:
 *        COMMIT3resok   resok;
 *      default:
 *        COMMIT3resfail resfail;
 *     };
 *
 * Can simply be written as:
 *
 *     struct COMMIT3res
 *         : public detail::Nfsstat3Variant<COMMIT3resok, COMMIT3resfail> {};
 *
 */
template <typename ResOkT, typename DefaultT = std::monostate>
struct Nfsstat3Variant : public std::conditional_t<
                             std::is_same_v<DefaultT, std::monostate>,
                             XdrVariant<nfsstat3, ResOkT>,
                             XdrVariant<nfsstat3, ResOkT, DefaultT>> {
  using ResOk = ResOkT;
  using Default = DefaultT;
};
} // namespace detail

template <typename T>
struct XdrTrait<
    T,
    std::enable_if_t<std::is_base_of_v<
        detail::Nfsstat3Variant<typename T::ResOk, typename T::Default>,
        T>>> : public XdrTrait<typename T::Base> {
  static T deserialize(folly::io::Cursor& cursor) {
    T ret;
    ret.tag = XdrTrait<nfsstat3>::deserialize(cursor);
    switch (ret.tag) {
      case nfsstat3::NFS3_OK:
        ret.v = XdrTrait<typename T::ResOk>::deserialize(cursor);
        break;
      default:
        if constexpr (!std::is_same_v<typename T::Default, std::monostate>) {
          ret.v = XdrTrait<typename T::Default>::deserialize(cursor);
        }
        break;
    }
    return ret;
  }
};

enum class ftype3 : uint32_t {
  NF3REG = 1,
  NF3DIR = 2,
  NF3BLK = 3,
  NF3CHR = 4,
  NF3LNK = 5,
  NF3SOCK = 6,
  NF3FIFO = 7
};

struct specdata3 {
  uint32_t specdata1;
  uint32_t specdata2;
};
EDEN_XDR_SERDE_DECL(specdata3, specdata1, specdata2);

/**
 * The NFS spec specify this struct as being opaque from the client
 * perspective, and thus we are free to use what is needed to uniquely identify
 * a file. In EdenFS, this is perfectly represented by an InodeNumber.
 *
 * As an InodeNumber is unique per mount, an Nfsd program can only handle one
 * mount per instance. This will either need to be extended to support multiple
 * mounts, or an Nfsd instance per mount will need to be created.
 *
 * Note that this structure is serialized as an opaque byte vector, and will
 * thus be preceded by a uint32_t.
 */
struct nfs_fh3 {
  InodeNumber ino;
};

template <>
struct XdrTrait<nfs_fh3> {
  static void serialize(folly::io::Appender& appender, const nfs_fh3& fh) {
    XdrTrait<uint32_t>::serialize(appender, sizeof(nfs_fh3));
    XdrTrait<uint64_t>::serialize(appender, fh.ino.get());
  }

  static nfs_fh3 deserialize(folly::io::Cursor& cursor) {
    uint32_t size = XdrTrait<uint32_t>::deserialize(cursor);
    XCHECK_EQ(size, sizeof(nfs_fh3));
    return {InodeNumber{XdrTrait<uint64_t>::deserialize(cursor)}};
  }
};

inline bool operator==(const nfs_fh3& a, const nfs_fh3& b) {
  return a.ino == b.ino;
}

struct nfstime3 {
  uint32_t seconds;
  uint32_t nseconds;
};
EDEN_XDR_SERDE_DECL(nfstime3, seconds, nseconds);

struct fattr3 {
  ftype3 type;
  uint32_t mode;
  uint32_t nlink;
  uint32_t uid;
  uint32_t gid;
  uint64_t size;
  uint64_t used;
  specdata3 rdev;
  uint64_t fsid;
  uint64_t fileid;
  nfstime3 atime;
  nfstime3 mtime;
  nfstime3 ctime;
};
EDEN_XDR_SERDE_DECL(
    fattr3,
    type,
    mode,
    nlink,
    uid,
    gid,
    size,
    used,
    rdev,
    fsid,
    fileid,
    atime,
    mtime,
    ctime);

/**
 * Values for fattr3::mode
 */
constexpr uint32_t kSUIDBit = 0x800;
constexpr uint32_t kGIDBit = 0x400;
constexpr uint32_t kSaveSwappedTextBit = 0x200;
constexpr uint32_t kReadOwnerBit = 0x100;
constexpr uint32_t kWriteOwnerBit = 0x80;
constexpr uint32_t kExecOwnerBit = 0x40;
constexpr uint32_t kReadGroupBit = 0x20;
constexpr uint32_t kWriteGroupBit = 0x10;
constexpr uint32_t kExecGroupBit = 0x8;
constexpr uint32_t kReadOtherBit = 0x4;
constexpr uint32_t kWriteOtherBit = 0x2;
constexpr uint32_t kExecOtherBit = 0x1;

struct post_op_attr : public XdrOptionalVariant<fattr3> {};

struct wcc_attr {
  uint64_t size;
  nfstime3 mtime;
  nfstime3 ctime;
};
EDEN_XDR_SERDE_DECL(wcc_attr, size, mtime, ctime);

struct pre_op_attr : public XdrOptionalVariant<wcc_attr> {};

struct wcc_data {
  pre_op_attr before;
  post_op_attr after;
};
EDEN_XDR_SERDE_DECL(wcc_data, before, after);

struct post_op_fh3 : public XdrOptionalVariant<nfs_fh3> {};

enum class time_how {
  DONT_CHANGE = 0,
  SET_TO_SERVER_TIME = 1,
  SET_TO_CLIENT_TIME = 2
};

struct set_mode3 : public XdrOptionalVariant<uint32_t> {};
struct set_uid3 : public XdrOptionalVariant<uint32_t> {};
struct set_gid3 : public XdrOptionalVariant<uint32_t> {};
struct set_size3 : public XdrOptionalVariant<uint64_t> {};
struct set_atime : public XdrOptionalVariant<
                       nfstime3,
                       time_how,
                       time_how::SET_TO_CLIENT_TIME> {};
struct set_mtime : public XdrOptionalVariant<
                       nfstime3,
                       time_how,
                       time_how::SET_TO_CLIENT_TIME> {};

struct sattr3 {
  set_mode3 mode;
  set_uid3 uid;
  set_gid3 gid;
  set_size3 size;
  set_atime atime;
  set_mtime mtime;
};
EDEN_XDR_SERDE_DECL(sattr3, mode, uid, gid, size, atime, mtime);

struct diropargs3 {
  nfs_fh3 dir;
  std::string name;
};
EDEN_XDR_SERDE_DECL(diropargs3, dir, name);

// GETATTR Procedure:

struct GETATTR3args {
  nfs_fh3 object;
};
EDEN_XDR_SERDE_DECL(GETATTR3args, object);

struct GETATTR3resok {
  fattr3 obj_attributes;
};
EDEN_XDR_SERDE_DECL(GETATTR3resok, obj_attributes);

struct GETATTR3res : public detail::Nfsstat3Variant<GETATTR3resok> {};

// LOOKUP Procedure:

struct LOOKUP3args {
  diropargs3 what;
};
EDEN_XDR_SERDE_DECL(LOOKUP3args, what);

struct LOOKUP3resok {
  nfs_fh3 object;
  post_op_attr obj_attributes;
  post_op_attr dir_attributes;
};
EDEN_XDR_SERDE_DECL(LOOKUP3resok, object, obj_attributes, dir_attributes);

struct LOOKUP3resfail {
  post_op_attr dir_attributes;
};
EDEN_XDR_SERDE_DECL(LOOKUP3resfail, dir_attributes);

struct LOOKUP3res
    : public detail::Nfsstat3Variant<LOOKUP3resok, LOOKUP3resfail> {};

// ACCESS Procedure:

const uint32_t ACCESS3_READ = 0x0001;
const uint32_t ACCESS3_LOOKUP = 0x0002;
const uint32_t ACCESS3_MODIFY = 0x0004;
const uint32_t ACCESS3_EXTEND = 0x0008;
const uint32_t ACCESS3_DELETE = 0x0010;
const uint32_t ACCESS3_EXECUTE = 0x0020;

struct ACCESS3args {
  nfs_fh3 object;
  uint32_t access;
};
EDEN_XDR_SERDE_DECL(ACCESS3args, object, access);

struct ACCESS3resok {
  post_op_attr obj_attributes;
  uint32_t access;
};
EDEN_XDR_SERDE_DECL(ACCESS3resok, obj_attributes, access);

struct ACCESS3resfail {
  post_op_attr obj_attributes;
};
EDEN_XDR_SERDE_DECL(ACCESS3resfail, obj_attributes);

struct ACCESS3res
    : public detail::Nfsstat3Variant<ACCESS3resok, ACCESS3resfail> {};

// READLINK Procedure:

struct READLINK3args {
  nfs_fh3 symlink;
};
EDEN_XDR_SERDE_DECL(READLINK3args, symlink);

struct READLINK3resok {
  post_op_attr symlink_attributes;
  std::string data;
};
EDEN_XDR_SERDE_DECL(READLINK3resok, symlink_attributes, data);

struct READLINK3resfail {
  post_op_attr symlink_attributes;
};
EDEN_XDR_SERDE_DECL(READLINK3resfail, symlink_attributes);

struct READLINK3res
    : public detail::Nfsstat3Variant<READLINK3resok, READLINK3resfail> {};

// WRITE Procedure:

constexpr inline size_t NFS3_WRITEVERFSIZE = 8;
using writeverf3 = std::array<uint8_t, NFS3_WRITEVERFSIZE>;

enum class stable_how { UNSTABLE = 0, DATA_SYNC = 1, FILE_SYNC = 2 };

struct WRITE3args {
  nfs_fh3 file;
  uint64_t offset;
  uint32_t count;
  stable_how stable;
  std::unique_ptr<folly::IOBuf> data;
};
EDEN_XDR_SERDE_DECL(WRITE3args, file, offset, count, stable, data);

struct WRITE3resok {
  wcc_data file_wcc;
  uint32_t count;
  stable_how committed;
  writeverf3 verf;
};
EDEN_XDR_SERDE_DECL(WRITE3resok, file_wcc, count, committed, verf);

struct WRITE3resfail {
  wcc_data file_wcc;
};
EDEN_XDR_SERDE_DECL(WRITE3resfail, file_wcc);

struct WRITE3res : public detail::Nfsstat3Variant<WRITE3resok, WRITE3resfail> {
};

// CREATE Procedure:

enum class createmode3 { UNCHECKED = 0, GUARDED = 1, EXCLUSIVE = 2 };

constexpr inline size_t NFS3_CREATEVERFSIZE = 8;
using createverf3 = std::array<uint8_t, NFS3_CREATEVERFSIZE>;

struct createhow3 : public XdrVariant<createmode3, sattr3, createverf3> {};

template <>
struct XdrTrait<createhow3> : public XdrTrait<createhow3::Base> {
  static createhow3 deserialize(folly::io::Cursor& cursor) {
    createhow3 ret;
    ret.tag = XdrTrait<createmode3>::deserialize(cursor);
    switch (ret.tag) {
      case createmode3::UNCHECKED:
      case createmode3::GUARDED:
        ret.v = XdrTrait<sattr3>::deserialize(cursor);
        break;
      case createmode3::EXCLUSIVE:
        ret.v = XdrTrait<createverf3>::deserialize(cursor);
        break;
    }
    return ret;
  }
};

struct CREATE3args {
  diropargs3 where;
  createhow3 how;
};
EDEN_XDR_SERDE_DECL(CREATE3args, where, how);

struct CREATE3resok {
  post_op_fh3 obj;
  post_op_attr obj_attributes;
  wcc_data dir_wcc;
};
EDEN_XDR_SERDE_DECL(CREATE3resok, obj, obj_attributes, dir_wcc);

struct CREATE3resfail {
  wcc_data dir_wcc;
};
EDEN_XDR_SERDE_DECL(CREATE3resfail, dir_wcc);

struct CREATE3res
    : public detail::Nfsstat3Variant<CREATE3resok, CREATE3resfail> {};

// MKDIR Procedure:

struct MKDIR3args {
  diropargs3 where;
  sattr3 attributes;
};
EDEN_XDR_SERDE_DECL(MKDIR3args, where, attributes);

struct MKDIR3resok {
  post_op_fh3 obj;
  post_op_attr obj_attributes;
  wcc_data dir_wcc;
};
EDEN_XDR_SERDE_DECL(MKDIR3resok, obj, obj_attributes, dir_wcc);

struct MKDIR3resfail {
  wcc_data dir_wcc;
};
EDEN_XDR_SERDE_DECL(MKDIR3resfail, dir_wcc);

struct MKDIR3res : public detail::Nfsstat3Variant<MKDIR3resok, MKDIR3resfail> {
};

// FSINFO Procedure:

const uint32_t FSF3_LINK = 0x0001;
const uint32_t FSF3_SYMLINK = 0x0002;
const uint32_t FSF3_HOMOGENEOUS = 0x0008;
const uint32_t FSF3_CANSETTIME = 0x0010;

struct FSINFO3args {
  nfs_fh3 fsroot;
};
EDEN_XDR_SERDE_DECL(FSINFO3args, fsroot);

struct FSINFO3resok {
  post_op_attr obj_attributes;
  uint32_t rtmax;
  uint32_t rtpref;
  uint32_t rtmult;
  uint32_t wtmax;
  uint32_t wtpref;
  uint32_t wtmult;
  uint32_t dtpref;
  uint64_t maxfilesize;
  nfstime3 time_delta;
  uint32_t properties;
};
EDEN_XDR_SERDE_DECL(
    FSINFO3resok,
    obj_attributes,
    rtmax,
    rtpref,
    rtmult,
    wtmax,
    wtpref,
    wtmult,
    dtpref,
    maxfilesize,
    time_delta,
    properties);

struct FSINFO3resfail {
  post_op_attr obj_attributes;
};
EDEN_XDR_SERDE_DECL(FSINFO3resfail, obj_attributes);

struct FSINFO3res
    : public detail::Nfsstat3Variant<FSINFO3resok, FSINFO3resfail> {};

// PATHCONF Procedure:

struct PATHCONF3args {
  nfs_fh3 object;
};
EDEN_XDR_SERDE_DECL(PATHCONF3args, object);

struct PATHCONF3resok {
  post_op_attr obj_attributes;
  uint32_t linkmax;
  uint32_t name_max;
  bool no_trunc;
  bool chown_restricted;
  bool case_insensitive;
  bool case_preserving;
};
EDEN_XDR_SERDE_DECL(
    PATHCONF3resok,
    obj_attributes,
    linkmax,
    name_max,
    no_trunc,
    chown_restricted,
    case_insensitive,
    case_preserving);

struct PATHCONF3resfail {
  post_op_attr obj_attributes;
};
EDEN_XDR_SERDE_DECL(PATHCONF3resfail, obj_attributes);

struct PATHCONF3res
    : public detail::Nfsstat3Variant<PATHCONF3resok, PATHCONF3resfail> {};

} // namespace facebook::eden

#endif
