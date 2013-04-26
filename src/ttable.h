#ifndef _PARALIGN_TTABLE_H_
#define _PARALIGN_TTABLE_H_

#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <hdfs.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/scoped_array.hpp>
#include <boost/utility.hpp>

#include "types.h"
#include "contrib/log.h"

namespace paralign {
// A special "word" for generating word from nowhere. Thus any
// integerization for the input should not use 0.
const WordId kNull = 0;

// Return something close to zero when a translation table query leads
// to nothing.
const double kDefaultProbability = 1e-9;

// Pack the struct for easy read/write and save some memory
#pragma pack(push, 1)
template <class K, class V>
struct KV {
  KV() : k(), v() {}
  KV(K kk, V vv) : k(kk), v(vv) {}

  bool operator==(const KV<K, V> &that) const {
    return k == that.k && v == that.v;
  }

  K k;
  V v;
};
#pragma pack(pop)

// Looks up an array of key-value items sorted by the key; returns the
// actual pointer if found; otherwise return NULL. When multiple items
// have the same key, the last one is returned.
template <class K, class V>
KV<K, V> *LookUp(K key, KV<K, V> *base, size_t num) {
  if (num == 0)
    return NULL;
  // Now num >= 1
  size_t low = 0, high = num;
  // Invariant:
  // base[low].k <= key < base[high].k
  // 0 <= low < high <= num
  while (low + 1 != high) {
    size_t mid = low + ((high - low) >> 1);
    // low <= mid < high <= num
    if (key < base[mid].k)
      high = mid;
    else // base[mid].k <= k
      low = mid;
  }
  // base[low].k <= key < base[low+1].k
  if (base[low].k == key)
    return base + low;
  else
    return NULL;
}


typedef KV<WordId, KV<off_t, size_t> > IndexRecord;
typedef KV<WordId, double> EntryRecord;

// A table entry holds an array of `KV<WordId, double>`; all items are
// sorted by word id for efficient plus.
class TTableEntry {
 public:
  // Empty entry
  TTableEntry() : items_() {}

  TTableEntry(const TTableEntry &that) : items_(that.items_) {}

  // Converts a (ordered) map to an entry
  explicit TTableEntry(const std::map<WordId, double> &m) {
    items_.reserve(m.size());
    typedef std::pair<WordId, double> P;
    BOOST_FOREACH(const P &p, m) {
      if (p.second != 0)
        items_.push_back(EntryRecord(p.first, p.second));
    }
  }

  bool Empty() const {
    return items_.empty();
  }

  size_t Size() const {
    return items_.size();
  }

  EntryRecord &operator[](size_t i) {
    return items_[i];
  }

  const EntryRecord &operator[](size_t i) const {
    return items_[i];
  }

  bool operator==(const TTableEntry &that) const {
    return items_ == that.items_;
  }

  // Treats items as holding probability and normalizes them to sum to
  // one. Does nothing when the entry is empty. When not empty, issues
  // a warning when dividing by zero.
  void Normalize() {
    if (Empty()) return;
    double sum = 0;
    BOOST_FOREACH(const EntryRecord &i, items_) {
      sum += i.v;
    }
    if (sum == 0)
      LOG(WARNING) << "Division by zero in TTableEntry::Normalize";
    BOOST_FOREACH(EntryRecord &i, items_) {
      i.v /= sum;
    }
  }

  // Treats items as holding probability and normalizes using
  // mean-field approximation. Does nothing when the entry is empty.
  void NormalizeVB(double alpha) {
    if (Empty()) return;
    double sum = alpha * items_.size();
    BOOST_FOREACH(const EntryRecord &i, items_) {
      sum += i.v;
    }
    BOOST_FOREACH(EntryRecord &i, items_) {
      i.v = exp(boost::math::digamma(i.v + alpha) - boost::math::digamma(sum));
    }
  }

  // Clears items;
  void Clear() {
    items_.clear();
  }

  friend std::ostream &operator<<(std::ostream &, const TTableEntry &);
  friend std::istream &operator>>(std::istream &, TTableEntry &);
  friend void PlusEq(const TTableEntry &, const TTableEntry &, TTableEntry *);

 private:
  std::vector<EntryRecord> items_;
};

// Writes # of items, followed by tab-separated `EntryRecord` pairs,
// without any line breaks in between.
inline std::ostream &operator<<(std::ostream &out, const TTableEntry &e) {
  out << e.items_.size();
  BOOST_FOREACH(const EntryRecord &i, e.items_) {
    out << ' ' << i.k << ' ' << DoubleAsInt64(i.v);
  }
  return out;
}

// Reads what's written by `operator<<`, items must be ordered by word
// id.
inline std::istream &operator>>(std::istream &in, TTableEntry &e) {
  size_t n;
  in >> n;
  e.items_.resize(n);
  for (size_t i = 0; i < n; ++i) {
    in >> e.items_[i].k;
    int64_t v;
    in >> v;
    e.items_[i].v = DoubleFromInt64(v);
  }
  return in;
}

inline void PlusEq(const TTableEntry &x, const TTableEntry &y, TTableEntry *z) {
  z->items_.clear();
  size_t xi = 0, yi = 0;
  while (xi < x.items_.size() && yi < y.items_.size()) {
    const EntryRecord &xp = x.items_[xi], &yp = y.items_[yi];
    if (xp.k < yp.k) {
      z->items_.push_back(xp);
      ++xi;
    } else if (yp.k < xp.k) {
      z->items_.push_back(yp);
      ++yi;
    } else {
      double v = xp.v + yp.v;
      if (v != 0)
        z->items_.push_back(EntryRecord(xp.k, v));
      ++xi;
      ++yi;
    }
  }
  if (xi == x.items_.size()) {
    while (yi < y.items_.size()) {
      z->items_.push_back(y.items_[yi]);
      ++yi;
    }
  } else {
    while (xi < x.items_.size()) {
      z->items_.push_back(x.items_[xi]);
      ++xi;
    }
  }
}

// A single piece of the translation table. Reads raw binary records
// written by `TTableWriter` as read-only mmap. This allows memory
// sharing across multiple processes. Since it's holding an mmap, the
// meaning of copying is quite messy and thus we disallow it. But
// swapping is fine.
class PartialTTable : boost::noncopyable {
 public:
  PartialTTable()
      : index_base_(NULL), entry_base_(NULL),
        num_entry_(0), index_length_(0), entry_length_(0) {}

  ~PartialTTable() {
    if (index_base_)
      DoMunmap(static_cast<void *>(index_base_), index_length_);
    if (entry_base_)
      DoMunmap(static_cast<void *>(entry_base_), entry_length_);
  }

  // Break naming conventions to allow STL-style `swap`
  void swap(PartialTTable &that) {
    std::swap(index_base_, that.index_base_);
    std::swap(entry_base_, entry_base_);
    std::swap(num_entry_, that.num_entry_);
    std::swap(index_length_, that.index_length_);
    std::swap(entry_length_, that.entry_length_);
  }

  void Load(const std::string &index, const std::string &entry) {
    // Throw away old stuff
    PartialTTable old;
    swap(old);
    DoMmap(index, static_cast<void **>(static_cast<void *>(&index_base_)), &index_length_);
    if (index_length_ % sizeof(IndexRecord) != 0)
      LOG(FATAL) << "Index file size (" << index_length_ << " bytes)"
                 << " is not a multiple of index record size ("
                 << sizeof(IndexRecord) << " bytes)";
    num_entry_ = index_length_ / sizeof(IndexRecord);
    DoMmap(entry, static_cast<void **>(static_cast<void *>(&entry_base_)), &entry_length_);
    if (entry_length_ % sizeof(EntryRecord) != 0)
      LOG(FATAL) << "Entry file size (" << entry_length_ << " bytes)"
                 << " is not a multiple of entry record size ("
                 << sizeof(EntryRecord) << " bytes)";
  }

  void Dump(std::ostream &output) const {
    if (index_base_ == NULL) {
      output << "[ No index loaded ]\n";
    } else if (entry_base_ == NULL) {
      output << "[ No entry loaded ]\n";
    } else {
      for (size_t i = 0; i < num_entry_; ++i) {
        WordId src = index_base_[i].k;
        off_t offset = index_base_[i].v.k;
        size_t num_entry = index_base_[i].v.v;
        for (size_t j = 0; j < num_entry; ++j) {
          const EntryRecord &record = (entry_base_ + offset)[j];
          output << src << ' ' << record.k << ' ' << log(record.v) << ' '
                 << record.v << ' ' << DoubleAsInt64(record.v) << '\n';
        }
      }
      // output << "[raw] [index]\n";
      // for (size_t i = 0; i < num_entry_; ++i)
      //   output << "[raw] " << i << '\t' << index_base_[i].k << " -> "
      //          << index_base_[i].v.k << ' ' << index_base_[i].v.v << '\n';
      // output << "[raw] [entry]\n";
      // for (size_t i = 0; i < entry_length_ / sizeof(EntryRecord); ++i) {
      //   const EntryRecord &record = entry_base_[i];
      //   output << "[raw] " << i << '\t'
      //          << record.k << ' ' << log(record.v) << ' '
      //          << record.v << ' ' << DoubleAsInt64(record.v) << '\n';
      // }
    }
  }

  double Query(WordId src, WordId tgt) const {
    const IndexRecord *index_record = LookUp(src, index_base_, num_entry_);
    if (index_record == NULL)
      return kDefaultProbability;
    off_t offset = index_record->v.k;
    size_t num_entry_record = index_record->v.v;
    const EntryRecord *entry_record = LookUp(tgt, entry_base_ + offset, num_entry_record);
    if (entry_record == NULL)
      return kDefaultProbability;
    return entry_record->v;
  }

 private:
  void DoMmap(const std::string &path, void **addr, size_t *length) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
      const char *err_msg = std::strerror(errno);
      LOG(FATAL) << "Cannot read file " << path << ": " << err_msg;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
      const char *err_msg = std::strerror(errno);
      LOG(FATAL) << "Cannot fstat file " << path << ": " << err_msg;
    }

    if (st.st_size == 0) {
      LOG(INFO) << "Zero size file at " << path;
      *addr = NULL;
      *length = 0;
      return;
    }

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
      const char *err_msg = std::strerror(errno);
      LOG(FATAL) << "Cannot mmap file " << path << ": " << err_msg;
    }

    close(fd);

    *addr = map;
    *length = st.st_size;

    LOG(INFO) << "Loaded " << path << " as mmap @"
              << std::hex << map << "[" << *length << "]";
  }

  void DoMunmap(void *addr, size_t length) {
    int r = munmap(addr, length);
    if (r < 0) {
      const char *err_msg = std::strerror(errno);
      LOG(ERROR) << "Failed to munmap addr=" << std::hex << addr
                 << "length=" << length << " : " << err_msg;
    }
  }

  IndexRecord *index_base_;
  EntryRecord *entry_base_;
  size_t num_entry_, index_length_, entry_length_;
};

// Distributed translation table
class TTable : boost::noncopyable {
 public:
  TTable(const std::string &in_dir, size_t parts) : tables_(new PartialTTable[parts]), parts_(parts) {
    for (size_t i = 0; i < parts; ++i) {
      std::string index_path = in_dir + "/index." + boost::lexical_cast<string>(i);
      std::string entry_path = in_dir + "/entry." + boost::lexical_cast<string>(i);
      tables_[i].Load(index_path, entry_path);
    }
    LOG(INFO) << "Read " << parts << " pieces of translation table";
  }

  double Query(WordId src, WordId tgt) const {
    WordId part = src % parts_;
    if (part < 0)
      part += parts_;
    return tables_[part].Query(src, tgt);
  }

  void Dump(std::ostream &output) const {
    for (size_t i = 0; i < parts_; ++i) {
      output << "== PART " << i << " ==\n";
      tables_[i].Dump(output);
    }
  }

 private:
  boost::scoped_array<PartialTTable> tables_;
  size_t parts_;
};

// Writer to a single piece of the distributed translation table
class TTableWriter : boost::noncopyable {
 public:
  TTableWriter(const std::string &output_dir, const std::string &part)
      : fs_(NULL), index_(NULL), entry_(NULL) {
    Open(output_dir, part);
  }

  ~TTableWriter() {
    Close();
  }

  void Open(const std::string &output_dir, const std::string &part) {
    Close();
    std::string user(getenv("USER"));
    std::string protocol = "file";
    std::string path = output_dir;
    std::string namenode = "";
    size_t colon_pos = path.find(':');
    if (colon_pos != std::string::npos) {
      protocol = path.substr(0, colon_pos);
      path.erase(0, colon_pos + 1);
    }
    if (protocol == "file") {
      namenode = "file:///";
    } else if (protocol == "hdfs") {
      size_t not_slash_pos = path.find_first_not_of('/');
      if (not_slash_pos == std::string::npos)
        LOG(FATAL) << "Ill-formed path: " << path;
      path.erase(0, not_slash_pos);
      size_t slash_pos = path.find('/');
      namenode = "hdfs://" + path.substr(0, slash_pos);
      path.erase(0, slash_pos);
    } else {
      LOG(FATAL) << "Unknown protocol: " << protocol;
    }
    if (path.empty())
      LOG(FATAL) << "Empty path in " << output_dir;

    LOG(INFO) << "namenode: " << namenode;
    LOG(INFO) << "user: " << user;
    LOG(INFO) << "out dir: " << path;
    LOG(INFO) << "part: " << part;

    fs_ = hdfsConnectAsUser(namenode.c_str(), 0, user.c_str());
    if (fs_ == NULL)
      LOG(FATAL) << "Cannot connect to file system: " << namenode;

    index_ = hdfsOpenFile(fs_, (path + "/index." + part).c_str(), O_WRONLY, 0, 0, 0);
    if (index_ == NULL)
      LOG(FATAL) << "Cannot open index file for write: " << path << "/index." << part;

    entry_ = hdfsOpenFile(fs_, (path + "/entry." + part).c_str(), O_WRONLY, 0, 0, 0);
    if (entry_ == NULL)
      LOG(FATAL) << "Cannot open entry file for wite: " << path << "/entry." << part;
  }

  void Write(WordId src, const TTableEntry &entry) {
    tOffset begin_offset = hdfsTell(fs_, entry_);
    if (begin_offset < 0)
      LOG(FATAL) << "hdfsTell failed in TTableWriter::Write";
    AddToIndex(src, begin_offset, entry.Size());
    for (size_t i = 0; i < entry.Size(); ++i) {
      const EntryRecord &record = entry[i];
      tSize r = hdfsWrite(fs_, entry_, static_cast<const void *>(&record), sizeof(EntryRecord));
      if (r < 0)
        LOG(FATAL) << "hdfsWrite failed in TTableWriter::Write";
    }
  }

  void WriteIndex() {
    IndexRecord record;
    typedef std::pair<WordId, KV<off_t, size_t> > P;
    BOOST_FOREACH(const P &p, in_mem_index_) {
      record.k = p.first;
      record.v = p.second;
      tSize r = hdfsWrite(fs_, index_, static_cast<const void *>(&record), sizeof(IndexRecord));
      if (r < 0)
        LOG(FATAL) << "hdfsWrite failed in TTableWriter::WriteIndex";
    }
  }

  void Close() {
    if (index_) {
      hdfsCloseFile(fs_, index_);
      index_ = NULL;
    }
    if (entry_) {
      hdfsCloseFile(fs_, entry_);
      entry_ = NULL;
    }
    if (fs_) {
      hdfsDisconnect(fs_);
      fs_ = NULL;
    }
  }

 private:
  void AddToIndex(WordId src, off_t begin_offset, size_t num_record) {
    if (begin_offset % sizeof(EntryRecord) != 0)
      LOG(FATAL) << "Unaligned offset: " << begin_offset;
    in_mem_index_[src] = KV<off_t, size_t>(begin_offset / sizeof(EntryRecord), num_record);
  }

  hdfsFS fs_;
  hdfsFile index_, entry_;
  std::map<WordId, KV<off_t, size_t> > in_mem_index_;
};
}// namespace paralign
#endif  // _PARALIGN_TTABLE_H_
