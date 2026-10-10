#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "config/config_store.hpp"

namespace lamp {

// ConfigStore over a model of the ESP-IDF NVS page layout (nvs_storage.cpp,
// nvs_pagemanager.cpp, nvs_page.cpp), for budget tests that must fail where
// the device would. Five pages of 126 entries of 32 bytes. Entries are only
// appended to a page; an erase marks them and frees nothing until garbage
// collection. A string occupies 1 + ceil((len + 1) / 32) contiguous entries in
// one page. A blob is written in chunks, each 1 + ceil(chunk / 32) entries in
// a page's tail, then a one-entry index; the new version lands before the old
// one is erased; a blob's first chunk moves to a fresh page rather than split
// into a tail under 400 bytes. requestNewPage activates a free page while two
// are free, else copies the live items of the page with the fewest live
// entries into the last free page and frees it, refusing when no page has a
// free or erased entry. A string write gets one requestNewPage retry.
// Identical rewrites are skipped. A failed blob write cleans up as IDF 5.5's
// writeMultiPageBlob does: on the page of each chunk it recorded, it erases
// the first chunk numbered by its position (0, 1, ...) rather than by the new
// version's offset, so it can take the old version's chunk instead of its own.
// failBlobKey forces that failure on the next write of that lamp key, as
// requestNewPage failing after its first chunk is recorded. Reads and boot()
// clean up as IDF does (readBlob, boot). Erasing a blob takes its index and that version's
// chunks; chunks of another version stay until IDF's boot scan. Keys of other
// namespaces are written with putSystem.
class PagedNvs : public ConfigStore {
 public:
  static constexpr size_t kPages = 5;
  static constexpr size_t kEntries = 126;
  static constexpr size_t kEntrySize = 32;
  static constexpr size_t kChunkMax = kEntrySize * (kEntries - 1);

  size_t failedWrites = 0;
  std::string failBlobKey;  // a lamp key; cleared once the failure fires

  PagedNvs() {
    for (size_t i = 0; i < kPages; ++i) free_.push_back(i);
    activate();
  }

  std::string read(const char* key, const char* defaultValue) override {
    const Item* item = findLive(ns(key), Kind::String);
    return item ? item->data.substr(0, item->data.size() - 1) : std::string(defaultValue);
  }
  size_t readString(const char* key, char* out, size_t cap) override {
    const Item* item = findLive(ns(key), Kind::String);
    if (!item || item->data.size() > cap) return 0;
    std::memcpy(out, item->data.data(), item->data.size());
    return item->data.size() - 1;
  }
  size_t write(const char* key, const char* value) override {
    return putString(ns(key), value) ? std::strlen(value) : 0;
  }
  bool has(const char* key) override {
    const std::string k = ns(key);
    return findLive(k, Kind::String) || findLive(k, Kind::Primitive) ||
           findLive(k, Kind::BlobIndex);
  }
  // nvs_get_blob: the size comes from the index alone, so a blob over `cap`
  // fails before any chunk is read. Reading a chunk whose data fails its CRC
  // erases that chunk and fails; reading a blob with a chunk missing erases
  // the blob and fails (readMultiPageBlob).
  size_t readBlob(const char* key, void* out, size_t cap) override {
    const std::string k = ns(key);
    const Item* index = findLive(k, Kind::BlobIndex);
    if (!index || index->data.size() > cap) return 0;
    std::string value;
    if (!blobValue(k, value)) {
      if (!killCorruptChunk(k)) eraseKey(k);
      return 0;
    }
    std::memcpy(out, value.data(), value.size());
    return value.size();
  }
  size_t writeBlob(const char* key, const void* data, size_t len) override {
    return putBlob(ns(key), std::string(static_cast<const char*>(data), len)) ? len : 0;
  }
  bool erase(const char* key) override {
    eraseKey(ns(key));
    return true;
  }
  bool clear() override {
    for (auto& page : pages_) {
      for (auto& item : page.items) {
        if (item.live && item.key.rfind("lamp/", 0) == 0) kill(page, item);
      }
    }
    return true;
  }

  // A key outside the lamp namespace: a one-entry primitive when `len` is 0,
  // else a blob of `len` bytes whose content changes with `version`.
  bool putSystem(const std::string& key, size_t len, uint32_t version = 0) {
    if (len == 0) return putPrimitive(key, version);
    std::string data(len, '\0');
    for (size_t i = 0; i < len; ++i) data[i] = static_cast<char>(version + i);
    return putBlob(key, data);
  }

  void eraseSystem(const std::string& key) { eraseKey(key); }

  // A data chunk of the lamp key's blob fails its CRC from now on.
  void corruptBlob(const char* key) {
    for (auto& page : pages_) {
      for (auto& item : page.items) {
        if (item.live && item.key == ns(key) && item.kind == Kind::BlobData) item.crcBad = true;
      }
    }
  }

  // Storage::init after a reset: eraseMismatchedBlobIndexes drops an index
  // whose chunks are missing, and eraseOrphanDataBlobs every chunk no index
  // names.
  void boot() {
    for (auto& page : pages_) {
      for (auto& index : page.items) {
        if (!index.live || index.kind != Kind::BlobIndex) continue;
        uint8_t found = 0;
        for (const auto& p : pages_) {
          for (const auto& item : p.items) {
            found += item.live && item.key == index.key && item.kind == Kind::BlobData &&
                     item.chunk >= index.chunk && item.chunk < index.chunk + index.chunkCount;
          }
        }
        if (found != index.chunkCount) kill(page, index);
      }
    }
    for (auto& page : pages_) {
      for (auto& item : page.items) {
        if (!item.live || item.kind != Kind::BlobData) continue;
        bool named = false;
        for (const auto& p : pages_) {
          for (const auto& index : p.items) {
            named |= index.live && index.kind == Kind::BlobIndex && index.key == item.key &&
                     item.chunk >= index.chunk && item.chunk < index.chunk + index.chunkCount;
          }
        }
        if (!named) kill(page, item);
      }
    }
  }

  size_t liveEntries() const {
    size_t n = 0;
    for (const auto& page : pages_) n += page.used;
    return n;
  }

 private:
  enum class Kind : uint8_t { Primitive, String, BlobData, BlobIndex };
  enum class State : uint8_t { Free, Active, Full };
  struct Item {
    std::string key;
    Kind kind;
    uint8_t chunk;  // BlobData: version offset + index; BlobIndex: version offset
    uint8_t chunkCount;
    size_t span;
    std::string data;
    bool live;
    bool crcBad = false;
  };
  struct Page {
    State state = State::Free;
    std::vector<Item> items;
    size_t next = 0;
    size_t used = 0;
  };

  static std::string ns(const char* key) { return std::string("lamp/") + key; }

  Page& current() { return pages_[list_.back()]; }

  void activate() {
    const size_t i = free_.front();
    free_.pop_front();
    pages_[i] = Page{};
    pages_[i].state = State::Active;
    list_.push_back(i);
  }

  bool requestNewPage() {
    if (free_.empty()) return false;
    if (free_.size() >= 2) {
      activate();
      return true;
    }
    size_t victim = list_.size();
    size_t maxUnused = 0;
    for (size_t at = 0; at < list_.size(); ++at) {
      const size_t unused = kEntries - pages_[list_[at]].used;
      if (unused > maxUnused) {
        maxUnused = unused;
        victim = at;
      }
    }
    if (maxUnused == 0) return false;
    const size_t erased = list_[victim];
    activate();
    Page& fresh = current();
    for (const Item& item : pages_[erased].items) {
      if (!item.live) continue;
      fresh.items.push_back(item);
      fresh.next += item.span;
      fresh.used += item.span;
    }
    pages_[erased] = Page{};
    list_.erase(list_.begin() + static_cast<long>(victim));
    free_.push_back(erased);
    return true;
  }

  static size_t spanOf(Kind kind, size_t len) {
    return kind == Kind::Primitive || kind == Kind::BlobIndex
               ? 1
               : 1 + (len + kEntrySize - 1) / kEntrySize;
  }

  // Page::writeItem: false is ESP_ERR_NVS_PAGE_FULL.
  static bool pageWrite(Page& page, Item item) {
    if (page.state == State::Full || page.next + item.span > kEntries) return false;
    page.next += item.span;
    page.used += item.span;
    page.items.push_back(std::move(item));
    return true;
  }

  static size_t tailroom(const Page& page) {
    if (page.state == State::Full) return 0;
    return page.next < kEntries - 1 ? (kEntries - page.next - 1) * kEntrySize : 0;
  }

  void kill(Page& page, Item& item) {
    item.live = false;
    page.used -= item.span;
  }

  const Item* findLive(const std::string& key, Kind kind) const {
    for (const auto& page : pages_) {
      for (const auto& item : page.items) {
        if (item.live && item.key == key && item.kind == kind) return &item;
      }
    }
    return nullptr;
  }

  bool blobValue(const std::string& key, std::string& out) const {
    const Item* index = findLive(key, Kind::BlobIndex);
    if (!index) return false;
    out.clear();
    for (uint8_t c = 0; c < index->chunkCount; ++c) {
      bool found = false;
      for (const auto& page : pages_) {
        for (const auto& item : page.items) {
          if (!found && item.live && item.key == key && item.kind == Kind::BlobData &&
              item.chunk == index->chunk + c) {
            if (item.crcBad) return false;
            out += item.data;
            found = true;
          }
        }
      }
      if (!found) return false;
    }
    return true;
  }

  // Page::readVariableLengthItemData on a chunk that fails its CRC erases it.
  bool killCorruptChunk(const std::string& key) {
    for (auto& page : pages_) {
      for (auto& item : page.items) {
        if (item.live && item.key == key && item.kind == Kind::BlobData && item.crcBad) {
          kill(page, item);
          return true;
        }
      }
    }
    return false;
  }

  // Every live item under `key` that `match` picks.
  template <typename Match>
  void eraseWhere(const std::string& key, Match match) {
    for (auto& page : pages_) {
      for (auto& item : page.items) {
        if (item.live && item.key == key && match(item)) kill(page, item);
      }
    }
  }

  // nvs_erase_key: a single item, or a blob's index and its version's chunks.
  void eraseKey(const std::string& key) {
    const Item* index = findLive(key, Kind::BlobIndex);
    if (index) {
      const uint8_t start = index->chunk;
      const uint8_t count = index->chunkCount;
      eraseWhere(key, [&](const Item& item) {
        return item.kind == Kind::BlobIndex ||
               (item.kind == Kind::BlobData && item.chunk >= start && item.chunk < start + count);
      });
    }
    eraseWhere(key, [](const Item& item) {
      return item.kind == Kind::String || item.kind == Kind::Primitive;
    });
  }

  bool fail() {
    ++failedWrites;
    return false;
  }

  // Storage::writeItem for a single-item type.
  bool putSingle(const std::string& key, Kind kind, const std::string& data) {
    const Item* old = findLive(key, kind);
    if (old && old->data == data) return true;
    Item item{key, kind, 0xff, 0, spanOf(kind, data.size()), data, true};
    if (!pageWrite(current(), item)) {
      current().state = State::Full;
      if (!requestNewPage() || !pageWrite(current(), item)) return fail();
    }
    // The new item is the last one written; every other live copy goes.
    Item* fresh = &current().items.back();
    fresh->live = false;
    eraseKey(key);
    fresh->live = true;
    return true;
  }

  bool putPrimitive(const std::string& key, uint32_t value) {
    return putSingle(key, Kind::Primitive, std::string(reinterpret_cast<char*>(&value), 4));
  }

  bool putString(const std::string& key, const std::string& value) {
    if (value.size() + 1 > kChunkMax) return fail();
    return putSingle(key, Kind::String, value + '\0');
  }

  // Storage::writeMultiPageBlob, then the old version erased.
  bool putBlob(const std::string& key, const std::string& data) {
    const Item* old = findLive(key, Kind::BlobIndex);
    std::string existing;
    if (old && blobValue(key, existing) && existing == data) return true;
    const uint8_t start = old && old->chunk == 0 ? 128 : 0;
    if (data.size() > (kPages - 1) * kChunkMax) return fail();

    std::vector<size_t> usedPages;  // the page of each chunk recorded
    uint8_t chunks = 0;
    size_t offset = 0;
    bool ok = true;
    while (true) {
      Page& page = current();
      const size_t tail = tailroom(page);
      if (chunks == 0 && tail < data.size() && tail < kChunkMax / 10) {
        page.state = State::Full;
        if (!requestNewPage() || tailroom(current()) == tail) {
          ok = false;
          break;
        }
        continue;
      }
      if (tail == 0) {
        ok = false;
        break;
      }
      const size_t len = std::min(data.size() - offset, tail);
      pageWrite(page, Item{key, Kind::BlobData, static_cast<uint8_t>(start + chunks), 0,
                           spanOf(Kind::BlobData, len), data.substr(offset, len), true});
      usedPages.push_back(list_.back());
      if (key == ns(failBlobKey.c_str())) {
        failBlobKey.clear();
        ok = false;
        break;
      }
      ++chunks;
      offset += len;
      if (offset < data.size() || tail - len < kEntrySize) {
        page.state = State::Full;
        if (!requestNewPage()) {
          ok = false;
          break;
        }
      }
      if (offset == data.size()) {
        pageWrite(current(), Item{key, Kind::BlobIndex, start, chunks, 1,
                                  std::string(data.size(), '\0'), true});
        break;
      }
    }
    if (!ok) {
      for (size_t ii = 0; ii < usedPages.size(); ++ii) {
        Page& p = pages_[usedPages[ii]];
        for (auto& item : p.items) {
          if (item.live && item.key == key && item.kind == Kind::BlobData && item.chunk == ii) {
            kill(p, item);
            break;
          }
        }
      }
      return fail();
    }
    eraseWhere(key, [&](const Item& item) {
      if (item.kind == Kind::BlobIndex) return item.chunk != start;
      if (item.kind == Kind::BlobData) return item.chunk < start || item.chunk >= start + 128;
      return true;
    });
    return true;
  }

  std::array<Page, kPages> pages_;
  std::deque<size_t> list_;
  std::deque<size_t> free_;
};

}  // namespace lamp
