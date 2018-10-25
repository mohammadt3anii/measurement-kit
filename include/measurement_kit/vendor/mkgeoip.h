// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_MKGEOIP_H
#define MEASUREMENT_KIT_MKGEOIP_H

#include "mkcurl.h"

#ifdef __cplusplus
extern "C" {
#endif

/// mkgeoip_ubuntu_get_url returns Ubuntu-GeoIP-service's URL.
const char *mkgeoip_ubuntu_get_url(void);

/// mkgeoip_ubuntu_parser_t is the context for parsing the HTTP response
/// returned by Ubuntu's GEOIP service.
typedef struct mkgeoip_ubuntu_parser mkgeoip_ubuntu_parser_t;

/// mkgeoip_ubuntu_parser_new creates a new mkgeoip_ubuntu_parser_t instance.
mkgeoip_ubuntu_parser_t *mkgeoip_ubuntu_parser_new(void);

/// mkgeoip_ubuntu_parser_get_probe_ip extracts the probe IP from @p response
/// if available, otherwise returns NULL. The returned string is managed by
/// @p parser and is valid until @p parser is valid _and_ you don't call again
/// mkgeoip_ubuntu_parser_get_probe_ip.
const char *mkgeoip_ubuntu_parser_get_probe_ip(
    mkgeoip_ubuntu_parser_t *parser, const mkcurl_response_t *response);

/// mkgeoip_ubuntu_parser_delete deletes @p parser.
void mkgeoip_ubuntu_parser_delete(mkgeoip_ubuntu_parser_t *parser);

/// mkgeoip_mmdb_t saves the results of MMDB queries.
typedef struct mkgeoip_mmdb mkgeoip_mmdb_t;

/// mkgeoip_mmdb_open opens the database at @p path and returns the
/// database instance on success, or NULL on failure.
mkgeoip_mmdb_t *mkgeoip_mmdb_open(const char *path);

/// mkgeoip_mmdb_lookup_cc returns the country code of @p ip using the
/// @p mmdb database, or NULL in case of error. The returned string will
/// be valid until @p mmdb is valid _and_ you don't call other lookup
/// APIs using the same @p mmdb instance.
const char *mkgeoip_mmdb_lookup_cc(mkgeoip_mmdb_t *mmdb, const char *ip);

/// mkgeoip_mmdb_lookup_asn is like mkgeoip_mmdb_lookup_cc but returns
/// the ASN on success and zero on failure.
int64_t mkgeoip_mmdb_lookup_asn(mkgeoip_mmdb_t *mmdb, const char *ip);

/// mkgeoip_mmdb_lookup_org is like mkgeoip_mmdb_lookup_cc but returns
/// the organization bound to @p ip on success, NULL on failure. The
/// returned string will be valid until @p mmdb is valid _and_ you don't
/// call other lookup APIs using the same @p mmdb instance.
const char *mkgeoip_mmdb_lookup_org(mkgeoip_mmdb_t *mmdb, const char *ip);

/// mkgeoip_mmdb_close closes @p mmdb.
void mkgeoip_mmdb_close(mkgeoip_mmdb_t *mmdb);

#ifdef __cplusplus
}  // extern "C"

#include <memory>

/// mkgeoip_ubuntu_parser_deleter is a deleter for mkgeoip_ubuntu_parser_t.
struct mkgeoip_ubuntu_parser_deleter {
  void operator()(mkgeoip_ubuntu_parser_t *p) {
    mkgeoip_ubuntu_parser_delete(p);
  }
};

/// mkgeoip_ubuntu_parser_uptr is a unique pointer to mkgeoip_ubuntu_parser_t.
using mkgeoip_ubuntu_parser_uptr = std::unique_ptr<
    mkgeoip_ubuntu_parser_t, mkgeoip_ubuntu_parser_deleter>;

/// mkgeoip_mmdb_deleter is a deleter for mkgeoip_mmdb_t.
struct mkgeoip_mmdb_deleter {
  void operator()(mkgeoip_mmdb_t *p) { mkgeoip_mmdb_close(p); }
};

/// mkgeoip_mmdb_uptr is a unique pointer to mkgeoip_mmdb_t.
using mkgeoip_mmdb_uptr = std::unique_ptr<mkgeoip_mmdb_t, mkgeoip_mmdb_deleter>;

// By default the implementation is not included. You can force it being
// included by providing the following definition to the compiler.
//
// If you're just into understanding the API, you can stop reading here.
#ifdef MKGEOIP_INLINE_IMPL

#include <ctype.h>

#include <functional>
#include <string>
#include <vector>

#include <maxminddb.h>

const char *mkgeoip_ubuntu_get_url() {
  return "https://geoip.ubuntu.com/lookup";
}

struct mkgeoip_ubuntu_parser {
  std::string probe_ip;
};

mkgeoip_ubuntu_parser_t *mkgeoip_ubuntu_parser_new() {
  return new mkgeoip_ubuntu_parser_t;
}

#ifndef MKGEOIP_MKCURL_RESPONSE_GET_BODY_BINARY_V2
// MKGEOIP_MKCURL_RESPONSE_GET_BODY_BINARY_V2 allows to mock
// mkcurl_response_get_body_binary_v2
#define MKGEOIP_MKCURL_RESPONSE_GET_BODY_BINARY_V2  \
  mkcurl_response_get_body_binary_v2
#endif

const char *mkgeoip_ubuntu_parser_get_probe_ip(
    mkgeoip_ubuntu_parser_t *parser, const mkcurl_response_t *response) {
  if (parser == nullptr || response == nullptr) return nullptr;
  if (mkcurl_response_get_error(response) != 0 ||
      mkcurl_response_get_status_code(response) != 200) {
    return nullptr;
  }
  std::string body;
  {
    const uint8_t *base = nullptr;
    size_t count = 0;
    if (!MKGEOIP_MKCURL_RESPONSE_GET_BODY_BINARY_V2(
            response, &base, &count) ||
        base == nullptr || count <= 0) {
      return nullptr;
    }
    body = std::string{(char *)base, count};
  }
  {
    static const std::string open_tag = "<Ip>";
    static const std::string close_tag = "</Ip>";
    auto pos = body.find(open_tag);
    if (pos == std::string::npos) return nullptr;
    body = body.substr(pos + open_tag.size());  // Find EOS in the worst case
    pos = body.find(close_tag);
    if (pos == std::string::npos) return nullptr;
    body = body.substr(0, pos);
    for (auto ch : body) {
      if (isspace(ch)) continue;
      ch = (char)tolower(ch);
      auto ok = isdigit(ch) || (ch >= 'a' && ch <= 'f') || ch == '.' || ch == ':';
      if (!ok) return nullptr;
      parser->probe_ip += ch;
    }
  }
  return parser->probe_ip.c_str();
}

void mkgeoip_ubuntu_parser_delete(mkgeoip_ubuntu_parser_t *parser) {
  delete parser;
}

struct mkgeoip_mmdb_s_deleter {
  void operator()(MMDB_s *p) {
    MMDB_close(p);
    delete p;
  }
};

using mkgeoip_mmdb_s_uptr = std::unique_ptr<MMDB_s, mkgeoip_mmdb_s_deleter>;

struct mkgeoip_mmdb {
  mkgeoip_mmdb_s_uptr mmdbs;
  std::string saved_string;
};

#ifndef MKGEOIP_MMDB_OPEN
// MKGEOIP_MMDB_OPEN allows to mock MMDB_open
#define MKGEOIP_MMDB_OPEN MMDB_open
#endif

mkgeoip_mmdb_t *mkgeoip_mmdb_open(const char *path) {
  if (path == nullptr) return nullptr;
  mkgeoip_mmdb_uptr mmdb{new mkgeoip_mmdb_t};
  mmdb->mmdbs.reset(new MMDB_s);
  if (MKGEOIP_MMDB_OPEN(path, MMDB_MODE_MMAP, mmdb->mmdbs.get()) != 0) {
    return nullptr;
  }
  return mmdb.release();
}

#ifndef MKGEOIP_MMDB_GET_VALUE
// MKGEOIP_MMDB_GET_VALUE allows to mock MMDB_get_value
#define MKGEOIP_MMDB_GET_VALUE MMDB_get_value
#endif

#ifndef MKGEOIP_MMDB_LOOKUP_STRING
// MKGEOIP_MMDB_LOOKUP_STRING allows to mock MMDB_lookup_string
#define MKGEOIP_MMDB_LOOKUP_STRING MMDB_lookup_string
#endif

static void mkgeoip_lookup_mmdb(
    MMDB_s *mmdbp, const std::string &ip,
    std::function<void(MMDB_entry_s *)> fun) {
  auto gai_error = 0;
  auto mmdb_error = 0;
  auto record = MKGEOIP_MMDB_LOOKUP_STRING(mmdbp, ip.c_str(), &gai_error,
                                           &mmdb_error);
  if (gai_error == 0 && mmdb_error == 0 && record.found_entry == true) {
    fun(&record.entry);
  }
}

const char *mkgeoip_mmdb_lookup_cc(mkgeoip_mmdb_t *mmdb, const char *ip) {
  if (mmdb == nullptr || ip == nullptr) {
    return nullptr;
  }
  const char *rv = nullptr;
  mkgeoip_lookup_mmdb(
      mmdb->mmdbs.get(), ip, [&](MMDB_entry_s *entry) {
        MMDB_entry_data_s data{};
        auto mmdb_error = MKGEOIP_MMDB_GET_VALUE(
            entry, &data, "registered_country", "iso_code", nullptr);
        if (mmdb_error != 0) return;
        if (!data.has_data || data.type != MMDB_DATA_TYPE_UTF8_STRING) {
          return;
        }
        mmdb->saved_string = std::string{data.utf8_string, data.data_size};
        rv = mmdb->saved_string.c_str();
      });
  return rv;
}

int64_t mkgeoip_mmdb_lookup_asn(mkgeoip_mmdb_t *mmdb, const char *ip) {
  if (mmdb == nullptr || ip == nullptr) {
    return 0;
  }
  int64_t rv;
  mkgeoip_lookup_mmdb(
      mmdb->mmdbs.get(), ip, [&](MMDB_entry_s *entry) {
        MMDB_entry_data_s data{};
        auto mmdb_error = MKGEOIP_MMDB_GET_VALUE(
            entry, &data, "autonomous_system_number", nullptr);
        if (mmdb_error != 0) return;
        if (!data.has_data || data.type != MMDB_DATA_TYPE_UINT32) {
          return;
        }
        rv = data.uint32;
      });
  return rv;
}

const char *mkgeoip_mmdb_lookup_org(mkgeoip_mmdb_t *mmdb, const char *ip) {
  if (mmdb == nullptr || ip == nullptr) {
    return nullptr;
  }
  const char *rv = nullptr;
  mkgeoip_lookup_mmdb(
      mmdb->mmdbs.get(), ip, [&](MMDB_entry_s *entry) {
        MMDB_entry_data_s data{};
        auto mmdb_error = MKGEOIP_MMDB_GET_VALUE(
            entry, &data, "autonomous_system_organization", nullptr);
        if (mmdb_error != 0) return;
        if (!data.has_data || data.type != MMDB_DATA_TYPE_UTF8_STRING) {
          return;
        }
        mmdb->saved_string = std::string{data.utf8_string, data.data_size};
        rv = mmdb->saved_string.c_str();
      });
  return rv;
}

void mkgeoip_mmdb_close(mkgeoip_mmdb_t *mmdb) { delete mmdb; }

#endif  // MKGEOIP_INLINE_IMPL
#endif  // __cplusplus
#endif  // MEASUREMENT_KIT_MKGEOIP_H
