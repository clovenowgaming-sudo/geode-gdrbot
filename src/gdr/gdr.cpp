#include "gdr.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <utility>

namespace gdr {
namespace {

// ---------------------------------------------------------------- generic value

struct Value {
    enum class Type { Nil, Bool, Int, Float, Str, Arr, Map };

    Type type = Type::Nil;
    bool b = false;
    long long i = 0;
    double f = 0.0;
    std::string s;
    std::vector<Value> arr;
    std::vector<std::pair<std::string, Value>> map;

    Value const* find(std::string_view key) const {
        for (auto& [k, v] : map) {
            if (k == key) return &v;
        }
        return nullptr;
    }

    double asDouble(double def = 0.0) const {
        if (type == Type::Int) return static_cast<double>(i);
        if (type == Type::Float) return f;
        if (type == Type::Bool) return b ? 1.0 : 0.0;
        return def;
    }

    long long asInt(long long def = 0) const {
        if (type == Type::Int) return i;
        if (type == Type::Float) return static_cast<long long>(f);
        if (type == Type::Bool) return b ? 1 : 0;
        return def;
    }

    bool asBool(bool def = false) const {
        if (type == Type::Bool) return b;
        if (type == Type::Int) return i != 0;
        if (type == Type::Float) return f != 0.0;
        return def;
    }

    std::string asString(std::string def = {}) const {
        if (type == Type::Str) return s;
        return def;
    }
};

// --------------------------------------------------------------------- msgpack

class Reader {
public:
    Reader(std::vector<std::uint8_t> const& data) : m_data(data) {}

    Value parse() {
        return parseValue(0);
    }

    bool ok() const {
        return m_ok;
    }

private:
    std::vector<std::uint8_t> const& m_data;
    std::size_t m_pos = 0;
    bool m_ok = true;

    bool need(std::size_t n) {
        if (!m_ok || m_pos + n > m_data.size()) {
            m_ok = false;
            return false;
        }
        return true;
    }

    std::uint8_t u8() {
        if (!need(1)) return 0;
        return m_data[m_pos++];
    }

    std::uint16_t u16() {
        if (!need(2)) return 0;
        std::uint16_t v = static_cast<std::uint16_t>(m_data[m_pos]) << 8;
        v |= static_cast<std::uint16_t>(m_data[m_pos + 1]);
        m_pos += 2;
        return v;
    }

    std::uint32_t u32() {
        if (!need(4)) return 0;
        std::uint32_t v = 0;
        for (int k = 0; k < 4; k++) {
            v = (v << 8) | m_data[m_pos + k];
        }
        m_pos += 4;
        return v;
    }

    std::uint64_t u64() {
        if (!need(8)) return 0;
        std::uint64_t v = 0;
        for (int k = 0; k < 8; k++) {
            v = (v << 8) | m_data[m_pos + k];
        }
        m_pos += 8;
        return v;
    }

    Value readString(std::size_t len) {
        Value v;
        v.type = Value::Type::Str;
        if (!need(len)) return v;
        v.s.assign(reinterpret_cast<char const*>(m_data.data() + m_pos), len);
        m_pos += len;
        return v;
    }

    Value readArray(std::size_t len, int depth) {
        Value v;
        v.type = Value::Type::Arr;
        v.arr.reserve(len < 4096 ? len : 4096);
        for (std::size_t k = 0; k < len && m_ok; k++) {
            v.arr.push_back(parseValue(depth + 1));
        }
        return v;
    }

    Value readMap(std::size_t len, int depth) {
        Value v;
        v.type = Value::Type::Map;
        for (std::size_t k = 0; k < len && m_ok; k++) {
            auto key = parseValue(depth + 1);
            auto val = parseValue(depth + 1);
            v.map.emplace_back(key.asString(), std::move(val));
        }
        return v;
    }

    Value parseValue(int depth) {
        Value v;
        if (depth > 64 || !m_ok) {
            m_ok = false;
            return v;
        }

        auto const head = u8();
        if (!m_ok) return v;

        if (head <= 0x7f) {
            v.type = Value::Type::Int;
            v.i = head;
            return v;
        }
        if (head >= 0xe0) {
            v.type = Value::Type::Int;
            v.i = static_cast<int8_t>(head);
            return v;
        }
        if (head >= 0x80 && head <= 0x8f) return readMap(head & 0x0f, depth);
        if (head >= 0x90 && head <= 0x9f) return readArray(head & 0x0f, depth);
        if (head >= 0xa0 && head <= 0xbf) return readString(head & 0x1f);

        switch (head) {
            case 0xc0: return v; // nil
            case 0xc2: v.type = Value::Type::Bool; v.b = false; return v;
            case 0xc3: v.type = Value::Type::Bool; v.b = true; return v;
            case 0xca: {
                // u32() already assembles the big endian bytes into host order,
                // so the value doubles as the IEEE-754 bit pattern.
                auto bits = u32();
                float f = 0.f;
                std::memcpy(&f, &bits, sizeof(f));
                v.type = Value::Type::Float;
                v.f = static_cast<double>(f);
                return v;
            }
            case 0xcb: {
                auto bits = u64();
                double d = 0.0;
                std::memcpy(&d, &bits, sizeof(d));
                v.type = Value::Type::Float;
                v.f = d;
                return v;
            }
            case 0xcc: v.type = Value::Type::Int; v.i = u8(); return v;
            case 0xcd: v.type = Value::Type::Int; v.i = u16(); return v;
            case 0xce: v.type = Value::Type::Int; v.i = u32(); return v;
            case 0xcf: v.type = Value::Type::Int; v.i = static_cast<long long>(u64()); return v;
            case 0xd0: v.type = Value::Type::Int; v.i = static_cast<int8_t>(u8()); return v;
            case 0xd1: v.type = Value::Type::Int; v.i = static_cast<int16_t>(u16()); return v;
            case 0xd2: v.type = Value::Type::Int; v.i = static_cast<int32_t>(u32()); return v;
            case 0xd3: v.type = Value::Type::Int; v.i = static_cast<int64_t>(u64()); return v;
            case 0xd9: return readString(u8());
            case 0xda: return readString(u16());
            case 0xdb: return readString(u32());
            case 0xdc: return readArray(u16(), depth);
            case 0xdd: return readArray(u32(), depth);
            case 0xde: return readMap(u16(), depth);
            case 0xdf: return readMap(u32(), depth);
            default: break;
        }

        m_ok = false;
        return v;
    }
};

class Writer {
public:
    void nil() {
        m_out.push_back(0xc0);
    }

    void boolean(bool v) {
        m_out.push_back(v ? 0xc3 : 0xc2);
    }

    void integer(long long v) {
        if (v >= 0) {
            unsigned long long u = static_cast<unsigned long long>(v);
            if (u <= 0x7f) {
                m_out.push_back(static_cast<std::uint8_t>(u));
            } else if (u <= 0xff) {
                m_out.push_back(0xcc);
                m_out.push_back(static_cast<std::uint8_t>(u));
            } else if (u <= 0xffff) {
                m_out.push_back(0xcd);
                pushBE(u, 2);
            } else if (u <= 0xffffffffull) {
                m_out.push_back(0xce);
                pushBE(u, 4);
            } else {
                m_out.push_back(0xcf);
                pushBE(u, 8);
            }
            return;
        }

        if (v >= -32) {
            m_out.push_back(static_cast<std::uint8_t>(static_cast<std::int8_t>(v)));
        } else if (v >= -128) {
            m_out.push_back(0xd0);
            m_out.push_back(static_cast<std::uint8_t>(static_cast<std::int8_t>(v)));
        } else if (v >= -32768) {
            m_out.push_back(0xd1);
            pushBE(static_cast<unsigned long long>(v), 2);
        } else if (v >= -2147483648LL) {
            m_out.push_back(0xd2);
            pushBE(static_cast<unsigned long long>(v), 4);
        } else {
            m_out.push_back(0xd3);
            pushBE(static_cast<unsigned long long>(v), 8);
        }
    }

    void real(double v) {
        std::uint64_t raw = 0;
        std::memcpy(&raw, &v, sizeof(raw));
        m_out.push_back(0xcb);
        pushBE(raw, 8);
    }

    void str(std::string const& s) {
        auto len = s.size();
        if (len <= 0x1f) {
            m_out.push_back(static_cast<std::uint8_t>(0xa0 | len));
        } else if (len <= 0xff) {
            m_out.push_back(0xd9);
            m_out.push_back(static_cast<std::uint8_t>(len));
        } else if (len <= 0xffff) {
            m_out.push_back(0xda);
            pushBE(len, 2);
        } else {
            m_out.push_back(0xdb);
            pushBE(len, 4);
        }
        m_out.insert(m_out.end(), s.begin(), s.end());
    }

    void cstr(char const* s) {
        str(std::string(s));
    }

    void array(std::size_t n) {
        if (n <= 0x0f) {
            m_out.push_back(static_cast<std::uint8_t>(0x90 | n));
        } else if (n <= 0xffff) {
            m_out.push_back(0xdc);
            pushBE(n, 2);
        } else {
            m_out.push_back(0xdd);
            pushBE(n, 4);
        }
    }

    void map(std::size_t n) {
        if (n <= 0x0f) {
            m_out.push_back(static_cast<std::uint8_t>(0x80 | n));
        } else if (n <= 0xffff) {
            m_out.push_back(0xde);
            pushBE(n, 2);
        } else {
            m_out.push_back(0xdf);
            pushBE(n, 4);
        }
    }

    void key(char const* k) {
        str(std::string(k));
    }

    std::vector<std::uint8_t> take() {
        return std::move(m_out);
    }

private:
    std::vector<std::uint8_t> m_out;

    void pushBE(unsigned long long v, int bytes) {
        for (int k = bytes - 1; k >= 0; k--) {
            m_out.push_back(static_cast<std::uint8_t>((v >> (8 * k)) & 0xff));
        }
    }
};

// ------------------------------------------------------------------------ json

void jsonEscape(std::string& out, std::string const& s) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(c);
                }
        }
    }
    out.push_back('"');
}

std::string number(double v) {
    if (std::isfinite(v) && v == std::floor(v) && std::fabs(v) < 1e15) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f", v);
        return buf;
    }
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%.6f", v);
    return buf;
}

class JsonReader {
public:
    explicit JsonReader(std::vector<std::uint8_t> const& data)
        : m_data(reinterpret_cast<char const*>(data.data())), m_size(data.size()) {}

    Value parse() {
        skipSpace();
        auto v = parseValue(0);
        return v;
    }

    bool ok() const {
        return m_ok;
    }

private:
    char const* m_data;
    std::size_t m_size;
    std::size_t m_pos = 0;
    bool m_ok = true;

    char peek() const {
        return m_pos < m_size ? m_data[m_pos] : '\0';
    }

    void skipSpace() {
        while (m_pos < m_size) {
            char c = m_data[m_pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                m_pos++;
            } else {
                break;
            }
        }
    }

    bool consume(char expected) {
        skipSpace();
        if (peek() == expected) {
            m_pos++;
            return true;
        }
        m_ok = false;
        return false;
    }

    std::string parseString() {
        std::string out;
        if (!consume('"')) return out;
        while (m_pos < m_size) {
            char c = m_data[m_pos++];
            if (c == '"') return out;
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (m_pos >= m_size) break;
            char esc = m_data[m_pos++];
            switch (esc) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'u': {
                    if (m_pos + 4 <= m_size) {
                        char hex[5] = {0};
                        std::memcpy(hex, m_data + m_pos, 4);
                        m_pos += 4;
                        auto code = static_cast<unsigned>(std::strtoul(hex, nullptr, 16));
                        if (code < 0x80) {
                            out.push_back(static_cast<char>(code));
                        } else if (code < 0x800) {
                            out.push_back(static_cast<char>(0xc0 | (code >> 6)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                        } else {
                            out.push_back(static_cast<char>(0xe0 | (code >> 12)));
                            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                            out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                        }
                    }
                    break;
                }
                default: out.push_back(esc);
            }
        }
        m_ok = false;
        return out;
    }

    Value parseValue(int depth) {
        Value v;
        if (depth > 64) {
            m_ok = false;
            return v;
        }

        skipSpace();
        char c = peek();
        if (c == '\0') {
            m_ok = false;
            return v;
        }

        if (c == '{') {
            m_pos++;
            v.type = Value::Type::Map;
            skipSpace();
            if (peek() == '}') {
                m_pos++;
                return v;
            }
            while (m_ok) {
                skipSpace();
                auto key = parseString();
                if (!consume(':')) break;
                auto val = parseValue(depth + 1);
                v.map.emplace_back(key, std::move(val));
                skipSpace();
                if (peek() == ',') {
                    m_pos++;
                    continue;
                }
                consume('}');
                break;
            }
            return v;
        }

        if (c == '[') {
            m_pos++;
            v.type = Value::Type::Arr;
            skipSpace();
            if (peek() == ']') {
                m_pos++;
                return v;
            }
            while (m_ok) {
                v.arr.push_back(parseValue(depth + 1));
                skipSpace();
                if (peek() == ',') {
                    m_pos++;
                    continue;
                }
                consume(']');
                break;
            }
            return v;
        }

        if (c == '"') {
            v.type = Value::Type::Str;
            v.s = parseString();
            return v;
        }

        if (m_pos + 4 <= m_size && std::strncmp(m_data + m_pos, "true", 4) == 0) {
            m_pos += 4;
            v.type = Value::Type::Bool;
            v.b = true;
            return v;
        }
        if (m_pos + 5 <= m_size && std::strncmp(m_data + m_pos, "false", 5) == 0) {
            m_pos += 5;
            v.type = Value::Type::Bool;
            v.b = false;
            return v;
        }
        if (m_pos + 4 <= m_size && std::strncmp(m_data + m_pos, "null", 4) == 0) {
            m_pos += 4;
            return v;
        }

        // number
        std::size_t start = m_pos;
        bool isFloat = false;
        while (m_pos < m_size) {
            char d = m_data[m_pos];
            if ((d >= '0' && d <= '9') || d == '-' || d == '+') {
                m_pos++;
            } else if (d == '.' || d == 'e' || d == 'E') {
                isFloat = true;
                m_pos++;
            } else {
                break;
            }
        }
        if (m_pos == start) {
            m_ok = false;
            return v;
        }

        std::string token(m_data + start, m_pos - start);
        if (isFloat) {
            v.type = Value::Type::Float;
            v.f = std::strtod(token.c_str(), nullptr);
        } else {
            v.type = Value::Type::Int;
            v.i = std::strtoll(token.c_str(), nullptr, 10);
        }
        return v;
    }
};

} // namespace

// -------------------------------------------------------------------- encoding

std::vector<std::uint8_t> Replay::exportData(bool asJson) const {
    double dur = duration;
    if (dur <= 0.0 && !inputs.empty()) {
        dur = static_cast<double>(inputs.back().frame) / static_cast<double>(framerate);
    }

    if (asJson) {
        std::string out = "{\n";
        out += "  \"author\": ";
        jsonEscape(out, author);
        out += ",\n  \"description\": ";
        jsonEscape(out, description);
        out += ",\n  \"duration\": " + number(dur);
        out += ",\n  \"gameVersion\": " + number(gameVersion);
        out += ",\n  \"version\": " + number(1.0);
        out += ",\n  \"framerate\": " + number(framerate);
        out += ",\n  \"seed\": " + number(seed);
        out += ",\n  \"coins\": " + number(coins);
        out += ",\n  \"ldm\": ";
        out += ldm ? "true" : "false";
        out += ",\n  \"bot\": { \"name\": ";
        jsonEscape(out, bot.name);
        out += ", \"version\": ";
        jsonEscape(out, bot.version);
        out += " },\n  \"level\": { \"id\": " + number(level.id) + ", \"name\": ";
        jsonEscape(out, level.name);
        out += " },\n  \"inputs\": [";

        bool first = true;
        for (auto const& in : inputs) {
            if (!first) out += ",";
            first = false;
            out += "\n    { \"frame\": " + number(in.frame) + ", \"btn\": " + number(in.button) +
                   ", \"2p\": " + (in.player2 ? "true" : "false") +
                   ", \"down\": " + (in.down ? "true" : "false") + " }";
        }

        out += "\n  ],\n  \"gdrbot\": { \"version\": ";
        jsonEscape(out, bot.version);
        out += ", \"frameFixes\": [";

        first = true;
        for (auto const& fix : frameFixes) {
            if (!first) out += ",";
            first = false;
            out += "\n    { \"frame\": " + number(fix.frame) + ", \"ignore\": " +
                   (fix.ignore ? "true" : "false") + " }";
        }

        out += "\n  ] }\n}\n";
        return std::vector<std::uint8_t>(out.begin(), out.end());
    }

    Writer w;
    w.map(13);
    w.key("author");
    w.str(author);
    w.key("description");
    w.str(description);
    w.key("duration");
    w.real(dur);
    w.key("gameVersion");
    w.real(static_cast<double>(gameVersion));
    w.key("version");
    w.real(1.0);
    w.key("framerate");
    w.real(static_cast<double>(framerate));
    w.key("seed");
    w.integer(seed);
    w.key("coins");
    w.integer(coins);
    w.key("ldm");
    w.boolean(ldm);

    w.key("bot");
    w.map(2);
    w.key("name");
    w.str(bot.name);
    w.key("version");
    w.str(bot.version);

    w.key("level");
    w.map(2);
    w.key("id");
    w.integer(static_cast<long long>(level.id));
    w.key("name");
    w.str(level.name);

    w.key("inputs");
    w.array(inputs.size());
    for (auto const& in : inputs) {
        w.map(4);
        w.key("frame");
        w.integer(static_cast<long long>(in.frame));
        w.key("btn");
        w.integer(in.button);
        w.key("2p");
        w.boolean(in.player2);
        w.key("down");
        w.boolean(in.down);
    }

    w.key("gdrbot");
    w.map(2);
    w.key("version");
    w.str(bot.version);
    w.key("frameFixes");
    w.array(frameFixes.size());
    for (auto const& fix : frameFixes) {
        w.map(2);
        w.key("frame");
        w.integer(static_cast<long long>(fix.frame));
        w.key("ignore");
        w.boolean(fix.ignore);
    }

    return w.take();
}

Replay Replay::importData(std::vector<std::uint8_t> const& data) {
    Replay replay;
    if (data.empty()) return replay;

    std::size_t probe = 0;
    while (probe < data.size() && (data[probe] == ' ' || data[probe] == '\t' ||
                                   data[probe] == '\n' || data[probe] == '\r')) {
        probe++;
    }

    Value root;
    if (probe < data.size() && data[probe] == '{') {
        JsonReader reader(data);
        root = reader.parse();
    } else {
        Reader reader(data);
        root = reader.parse();
    }

    if (root.type != Value::Type::Map) {
        return replay;
    }

    auto get = [&](char const* key) -> Value const* { return root.find(key); };

    if (auto v = get("author")) replay.author = v->asString();
    if (auto v = get("description")) replay.description = v->asString();
    if (auto v = get("duration")) replay.duration = v->asDouble();
    if (auto v = get("gameVersion")) replay.gameVersion = static_cast<float>(v->asDouble(2.208));
    if (auto v = get("version")) replay.version = static_cast<float>(v->asDouble(1.0));
    if (auto v = get("framerate")) replay.framerate = static_cast<float>(v->asDouble(240.0));
    if (auto v = get("seed")) replay.seed = static_cast<int>(v->asInt());
    if (auto v = get("coins")) replay.coins = static_cast<int>(v->asInt());
    if (auto v = get("ldm")) replay.ldm = v->asBool();

    if (replay.framerate <= 1.f) replay.framerate = 240.f;

    if (auto v = get("bot")) {
        if (auto n = v->find("name")) replay.bot.name = n->asString("GDRBot");
        if (auto ver = v->find("version")) replay.bot.version = ver->asString("1.0.0");
    }
    if (auto v = get("level")) {
        if (auto id = v->find("id")) replay.level.id = static_cast<std::uint32_t>(id->asInt());
        if (auto name = v->find("name")) replay.level.name = name->asString();
    }

    if (auto v = get("inputs"); v && v->type == Value::Type::Arr) {
        replay.inputs.reserve(v->arr.size());
        for (auto const& in : v->arr) {
            if (in.type != Value::Type::Map) continue;
            Input input;
            if (auto f = in.find("frame")) input.frame = static_cast<std::uint32_t>(f->asInt());
            if (auto b = in.find("btn")) input.button = static_cast<int>(b->asInt(1));
            if (auto p = in.find("2p")) input.player2 = p->asBool();
            if (auto d = in.find("down")) input.down = d->asBool();
            replay.inputs.push_back(input);
        }
    }

    if (auto v = get("gdrbot")) {
        if (auto fixes = v->find("frameFixes"); fixes && fixes->type == Value::Type::Arr) {
            for (auto const& fix : fixes->arr) {
                if (fix.type != Value::Type::Map) continue;
                FrameFix out;
                if (auto f = fix.find("frame")) out.frame = static_cast<std::uint32_t>(f->asInt());
                if (auto i = fix.find("ignore")) out.ignore = i->asBool();
                replay.frameFixes.push_back(out);
            }
        }
    }

    return replay;
}

} // namespace gdr
