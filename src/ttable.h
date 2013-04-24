#ifndef _PARALIGN_TTABLE_H_
#define _PARALIGN_TTABLE_H_

#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include <iostream>
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
        items_.push_back(WordIdDouble(p.first, p.second));
    }
  }

  bool Empty() const {
    return items_.empty();
  }

  size_t Size() const {
    return items_.size();
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
    BOOST_FOREACH(const WordIdDouble &i, items_) {
      sum += i.v;
    }
    if (sum == 0)
      LOG(WARNING) << "Division by zero in TTableEntry::Normalize";
    BOOST_FOREACH(WordIdDouble &i, items_) {
      i.v /= sum;
    }
  }

  // Treats items as holding probability and normalizes using
  // mean-field approximation. Does nothing when the entry is empty.
  void NormalizeVB(double alpha) {
    if (Empty()) return;
    double sum = alpha * items_.size();
    BOOST_FOREACH(const WordIdDouble &i, items_) {
      sum += i.v;
    }
    BOOST_FOREACH(WordIdDouble &i, items_) {
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
  typedef KV<WordId, double> WordIdDouble;
  std::vector<WordIdDouble> items_;
};

// Writes # of items, followed by tab-separated `WordIdDouble` pairs,
// without any line breaks in between.
inline std::ostream &operator<<(std::ostream &out, const TTableEntry &e) {
  out << e.items_.size();
  BOOST_FOREACH(const TTableEntry::WordIdDouble &i, e.items_) {
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
    const TTableEntry::WordIdDouble &xp = x.items_[xi], &yp = y.items_[yi];
    if (xp.k < yp.k) {
      z->items_.push_back(xp);
      ++xi;
    } else if (yp.k < xp.k) {
      z->items_.push_back(yp);
      ++yi;
    } else {
      double v = xp.v + yp.v;
      if (v != 0)
        z->items_.push_back(TTableEntry::WordIdDouble(xp.k, v));
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
  typedef KV<WordId, std::pair<off_t, size_t> > IndexRecord;
  typedef KV<WordId, double> EntryRecord;

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
    DoMmap(index, static_cast<void **>(static_cast<void *>(&entry_base_)), &entry_length_);
  }

  double Query(WordId src, WordId tgt) const {
    const IndexRecord *index_record = LookUp(src, index_base_, num_entry_);
    if (index_record == NULL)
      return kDefaultProbability;
    off_t offset = index_record->v.first;
    size_t num_entry_record = index_record->v.second;
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

    void *map = mmap(NULL, st.st_size, PROT_READ, 0, fd, 0);
    if (map == MAP_FAILED) {
      const char *err_msg = std::strerror(errno);
      LOG(FATAL) << "Cannot mmap file " << path << ": " << err_msg;
    }

    close(fd);

    *addr = map;
    *length = st.st_size;
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
class TTable {
 public:
  TTable(const std::string &prefix, size_t parts) : tables_(new PartialTTable[parts]), parts_(parts) {
    for (size_t i = 0; i < parts; ++i) {
      std::string index_path = prefix + ".index." + boost::lexical_cast<string>(i);
      std::string entry_path = prefix + ".entry." + boost::lexical_cast<string>(i);
      tables_[i].Load(index_path, entry_path);
    }
  }

  double Query(WordId src, WordId tgt) const {
    WordId part = src % parts_;
    if (part < 0)
      part += parts_;
    return tables_[part].Query(src, tgt);
  }

 private:
  boost::scoped_array<PartialTTable> tables_;
  size_t parts_;
};

// Writer to a single piece of the distributed translation table
class TTableWriter {
 public:
  TTableWriter(const std::string &prefix, bool local);
  void Write(WordId src, const TTableEntry &entry);
};
}// namespace paralign

#endif  // _PARALIGN_TTABLE_H_
