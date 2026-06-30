#include "TupleArrayVector.h"
#include "translate.h"
#include "StringTableAllocator.h"
#include "EASTL/bonus/tuple_vector.h"
#include "ioSys/dag_fileIo.h"
#include "util/dag_hash.h"
#include "util/dag_hashedKeyMap.h"
namespace translate {
  class LargeStringTableAllocator : public StringTableAllocator {
  public:
    LargeStringTableAllocator() : StringTableAllocator(12, 16) {}
  };
  typedef uint64_t hash_t;

  hash_t hash_method(std::string_view value) { return mem_hash_fnv1<64>(value.data(), value.size()); }


  class translate_table_t {
    // holds the string data
    // utf8 data

  private:
    // LENGTH-1 is to remove None
    std::array<LargeStringTableAllocator, (size_t) Languages::LENGTH - 1> allocator{};
    // tuple vector to hold allocator offsets for a specific language
    // translate_index_t indexes this
    TupleArrayVector<uint32_t, static_cast<size_t>(Languages::LENGTH) - 1, 0xFFFFFFFF> indexToOffs{500};
    HashedKeyMap<hash_t, translate_index_t> hashToIndex;

    LargeStringTableAllocator &get_allocator(Languages lang) {
      if (lang < Languages::English || lang >= Languages::LENGTH)
        EXCEPTION("invalid language {}", static_cast<int>(lang));
      return allocator[(size_t) lang - 1];
    }

    translate_index_t addKey(std::string_view key);

    bool parseCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col, bool skip_first = true);

    std::vector<char> temp_string_storage{};

  public:
    const char *get_value(translate_index_t index, Languages lang);

    translate_index_t get_key(std::string_view string);

    bool loadCsvFull(std::string_view filename);

    int getColForLang(char *buffer, int len, const char *lang);

    ~translate_table_t() = default;
  } TranslatorTable;

  Languages default_language = Languages::English;

  const char *translate_table_t::get_value(translate_index_t index, Languages lang) {
    if (lang < Languages::English || lang >= Languages::LENGTH || indexToOffs.size() <= index)
      return nullptr;
    auto offs = indexToOffs.at(static_cast<size_t>(lang) - 1, index);
    if (offs == 0xFFFFFFFF) // no value filled, this means the entry was empty
      return "";
    return allocator[(size_t) lang - 1].getDataRawUnsafe(offs);
  }

  translate_index_t translate_table_t::get_key(std::string_view string) {
    if (string.empty())
      return INVALID_TRANSLATE_INDEX;
    auto hash = hash_method(string);
    return hashToIndex.findOr(hash, INVALID_TRANSLATE_INDEX);
  }


  const char *localize(std::string_view value, Languages lang, ReturnPolicy return_policy) {
    // does key exist?
    auto key = TranslatorTable.get_key(value);
    if (key == INVALID_TRANSLATE_INDEX) {
      if (return_policy == ReturnPolicy::ReturnKey)
        return value.data();
      return nullptr;
    }
    if (lang == Languages::None)
      lang = default_language;
    return TranslatorTable.get_value(key, lang);
  }

  translate_index_t get_locale_index(std::string_view value) { return TranslatorTable.get_key(value); }
  const char *localize_index(translate_index_t index, Languages lang) {
    if (index == INVALID_TRANSLATE_INDEX)
      return nullptr;
    if (lang == Languages::None)
      lang = default_language;
    return TranslatorTable.get_value(index, lang);
  }

  void set_default_language(Languages lang) {
    if (lang == Languages::None || lang == Languages::LENGTH)
      return;
    default_language = lang;
  }


  std::string lastFileNameForDebug = "";


  void load_csv(std::string_view path) { ZoneScoped auto buffer = TranslatorTable.loadCsvFull(path); }

  static inline int parseCsvString(const char *&p, char *out, bool key) {
    int len = 0;

    bool quoted = false;
    const char *p0 = p;
    G_UNUSED(p0);

    if (*p == '"') {
      quoted = true;
      ++p;
    }

    while (*p) {
      if (quoted) {
        if (*p == '"') {
          ++p;
          if (*p != '"') {
            quoted = false;
            break;
          }
        }
      } else {
        if (*p == ';' || *p == '\r' || *p == '\n')
          break;
      }

      if (*p == '\\') {
        if (p[1] == 'r') {
          p += 2;
          len++;
          if (out)
            *out++ = '\r';
        } else if (p[1] == 'n') {
          p += 2;
          len++;
          if (out)
            *out++ = '\n';
        } else if (p[1] == 't') {
          p += 2;
          len++;
          if (out)
            *out++ = '\t';
        } else {
          if (out)
            *out++ = *p;
          ++p;
          ++len;
        }
      } else {
        if (out)
          *out++ = *p;
        ++p;
        ++len;
      }
    }

#if _DEBUG_INCONVENIENT_SYMBOLS
    if (!key) {
      const char *dbg_string = "'\"^`";
      int n = (int) strlen(dbg_string);
      if (out) {
        strncpy(out, dbg_string, n);
        out += n;
      }
      len += n;
    }
#else
    (void) key;
#endif

    if (quoted)
      LOGE("missing terminating quote in CSV item ({}):\n{}.{}", lastFileNameForDebug, int(p - p0), p0);
    if (out)
      *out = 0;
    if (*p == ';' || *p == '\r' || *p == '\n' || *p == '\0')
      return len;

    while (strchr(" \t\v", *p))
      p++;
    if (!memchr(";\r\n\0", *p, 4))
      LOGE("parse error at <{}.{}>, missing ';' or <CR> after field", p - p0 + 16, p0);
    if (*p == ';')
      p++;
    return len;
  }

#define MAX_KEY_LEN 256


  translate_index_t translate_table_t::addKey(std::string_view key) {
    auto hash = hash_method(key);
    auto idx = hashToIndex.findOr(hash, INVALID_TRANSLATE_INDEX);
    if (idx != INVALID_TRANSLATE_INDEX)
      EXCEPTION("key {} already exists", key);
    idx = indexToOffs.size();
    hashToIndex.emplace(hash, idx);
    // not all indexes may be populated, so 0xFFFFFFFF represents 'default' value. if its seen then assume ""
    indexToOffs.ensureOne();
    return idx;
  }

  bool translate_table_t::parseCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col, bool skip_first) {
    int numRows = 0;
    int max_col = -1;
    for (int i = 0; i < lang_col.size(); i++)
      if (max_col < lang_col[i])
        max_col = lang_col[i];

    if (max_col < 0) {
      LOGE("parseCsvV2Full found no required translations");
      return false;
    }
    std::vector<int> bmap_col;
    bmap_col.resize(max_col + 1);
    mem_set_ff(bmap_col);
    for (int i = 0; i < lang_col.size(); i++)
      if (lang_col[i] >= 0)
        bmap_col[lang_col[i]] = i;

    // Ignore first 3 bytes of UTF8 format
    if (len >= 3 && (unsigned char) buffer[0] == 0xEF && (unsigned char) buffer[1] == 0xBB &&
        (unsigned char) buffer[2] == 0xBF) {
      buffer += 3;
      len -= 3;
    }

    const char quoteSymbol = '\"';

    char *tmp_buf = buffer;

    for (int i = 0; i < len; ++i) {
      if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] == quoteSymbol)
        i++;
      else if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] != quoteSymbol)
        while ((++i) + 1 < len)
          if (buffer[i] == quoteSymbol) {
            if (buffer[i + 1] == quoteSymbol) {
              i++;
              continue; // ignore double quote symbol
            } else
              break;
          }

      if (buffer[i] == '\r' || buffer[i] == '\n') {
        if (buffer[i] == '\r' && i + 1 < len && buffer[i + 1] == '\n')
          ++i;
        ++numRows;
        if (numRows == 1)
          tmp_buf = &buffer[i + 1];
      }
    }
    // V2: skip first row
    if (skip_first) {
      len -= (tmp_buf - buffer);
      buffer = tmp_buf;
      if (len <= 0)
        return false;
      numRows--;
    }

    const char *ptr = buffer;
    this->indexToOffs.reserve(this->indexToOffs.size() + numRows);
    for (int i = 0; i < numRows; ++i) {
      static char key_buffer[MAX_KEY_LEN];
      const char *p = ptr;

      if (p >= buffer + len)
        break;

      int l = parseCsvString(p, NULL, true);
      if (l >= MAX_KEY_LEN) {
        if (l > 4096)
          EXCEPTION("key longer than 4096 bytes in CSV");
        else {
          char tmp[4096];
          l = parseCsvString(ptr, tmp, true);
          tmp[4095] = 0;
          EXCEPTION("Too long key in CSV, len = %d, key = %s", l, tmp);
        }
      }
      l = parseCsvString(ptr, key_buffer, true);


      bool skip_key = false;
      if (!key_buffer[0])
        skip_key = true;


      auto idx = get_key(key_buffer);
      // int idx = cb->getKey(key_buffer);

      if (idx != INVALID_TRANSLATE_INDEX) {
        skip_key = true;
        LOGD3("Duplicate key in CSV: {}, row {}, file {}", key_buffer, i + 1, lastFileNameForDebug);
      }

      if (!skip_key) {
        idx = this->addKey(key_buffer);
      }

      for (int langColNo = 0; langColNo <= max_col; langColNo++) {
        if (*ptr == ';')
          ++ptr;
        if (bmap_col[langColNo] < 0 || skip_key)
          l = parseCsvString(ptr, NULL, false);
        else {
          p = ptr;
          l = parseCsvString(p, NULL, false);
          temp_string_storage.resize(l + 1);
          char *text = temp_string_storage.data();
          parseCsvString(ptr, text, false);
          if (l != 0) {
            // lets only add an element when there is actual data
            auto &curr_allocator = this->get_allocator((Languages) (langColNo + 1));
            auto ref = curr_allocator.addDataRaw(text, l + 1);
            this->indexToOffs.at((Languages) (langColNo), idx) = ref;
          }
        }
      }

      bool insideQuote = false;
      for (;;) {
        G_ASSERTF(ptr < buffer + len, "Non-paired quotation marks in csv file {}", lastFileNameForDebug);

        if (ptr >= buffer + len)
          return true; // we can't read more from this file
        if (*ptr == '\"') {
          ptr++;
          if (*ptr != '\"')
            insideQuote = !insideQuote;
        }

        if ((*ptr == '\r' || *ptr == '\n') && !insideQuote)
          break;

        ptr++;
      }

      if (*ptr == '\r') {
        ++ptr;
        if (ptr < buffer + len && *ptr == '\n')
          ++ptr;
      } else if (*ptr == '\n')
        ++ptr;
    }

    return true;
  }

  bool translate_table_t::loadCsvFull(std::string_view filename) {
    FullFileLoadCB cb{filename};
    if (!cb.fileHandle)
      return false;

    auto len = cb.fileHandle->length();
    if (len == -1)
      return false;
    auto buffer = (char *) malloc(len + 2);
    if (cb.tryRead(buffer, len) != len) {
      free(buffer);
      return false;
    }
    ((char *) buffer)[len] = '\n';
    len++;
    ((char *) buffer)[len] = '\0';

    lastFileNameForDebug = filename;
    std::vector<int> lang_col;
    lang_col.resize(fullLangList.size() - 1);
    for (int i = 0; i < lang_col.size(); i++) {
      lang_col[i] = getColForLang(buffer, len, fullLangList[i + 1]);
      G_ASSERT_LOG(lang_col[i] >= 0, "Language '{}' not found. in file {}", fullLangList[i + 1], filename);
    }

    bool result = parseCsvV2Full(buffer, len, lang_col);
    free(buffer);
    return result;
  }

  int translate_table_t::getColForLang(char *buffer, int len, const char *lang) {
    if (!len || !buffer)
      return -1;

    if ((unsigned char) buffer[0] == 0xEF && (unsigned char) buffer[1] == 0xBB && (unsigned char) buffer[2] == 0xBF) {
      buffer += 3;
      len -= 3;
    }

    const char quoteSymbol = '\"';

    char *tmp_buf = buffer;

    for (int i = 0; i < len; ++i) {
      if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] != quoteSymbol)
        while ((++i) + 1 < len)
          if (buffer[i] == quoteSymbol) {
            if (buffer[i + 1] == quoteSymbol) {
              i++;
              continue; // ignore double quote symbol
            } else
              break;
          }

      if (buffer[i] == '\r' || buffer[i] == '\n') {
        tmp_buf = &buffer[i];
        break;
      }
    }
    len = (tmp_buf - buffer);
    if (len <= 0)
      return -1;

    const char *ptr = buffer;
    static char key_buffer[MAX_KEY_LEN];

    const char *p = ptr;
    int l = parseCsvString(p, NULL, true);
    if (l >= MAX_KEY_LEN)
      EXCEPTION("CSV header is too long, make sure the CSV separator is semicolon");
    (void) l;
    (void) p;
    l = parseCsvString(ptr, key_buffer, true);

    int result = 0;
    while (*ptr && ptr < buffer + len) {
      if (*ptr == ';')
        ++ptr;
      p = ptr;
      l = parseCsvString(p, NULL, true);
      if (l >= MAX_KEY_LEN)
        EXCEPTION("CSV header is too long, make sure the CSV separator is semicolon");
      l = parseCsvString(ptr, key_buffer, true);

      if ((p = strchr(key_buffer, '<')) != NULL)
        if (strncmp(p + 1, lang, strlen(lang)) == 0)
          return result;
      if (strncmp(key_buffer, lang, strlen(lang)) == 0)
        return result;
      result++;
    }
    return -1;
  }
} // namespace translate
