#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#include <protocol/http/http_context.h>
#include <utils/buffer.h>

using namespace std;

static void AssertEqual(const string &a, const string &b, const string &msg = "")
{
    if (a != b)
    {
        cerr << "ASSERT FAILED: " << msg << " | got='" << a << "' expected='" << b << "'\n";
        assert(false);
    }
}

static void TestParse(const string &raw, bool expect_ok, const string &exp_method = "", const string &exp_uri = "",
                      const string &exp_version = "", const vector<pair<string, string>> &exp_qs = {})
{
    Buffer buf(4096);
    buf.Write(raw);
    HttpContext ctx;
    bool ok = ctx.ParseRequest(buf);
    if (expect_ok)
    {
        if (!ok)
        {
            cerr << "Expected ok but failed for: " << raw << "\n";
            assert(false);
        }
        AssertEqual(ctx.GetRequest().method, exp_method, "method");
        AssertEqual(ctx.GetRequest().uri, exp_uri, "uri");
        AssertEqual(ctx.GetRequest().version, exp_version, "version");
        // check query params
        for (const auto &kv : exp_qs)
        {
            auto it = ctx.GetRequest().query_params.find(kv.first);
            if (it == ctx.GetRequest().query_params.end())
            {
                cerr << "Missing query key: " << kv.first << "\n";
                assert(false);
            }
            AssertEqual(it->second, kv.second, string("query val for ") + kv.first);
        }
    }
    else
    {
        if (ok)
        {
            cerr << "Expected failure but succeeded for: " << raw << "\n";
            assert(false);
        }
    }
}

int main()
{
    // Valid simple GET
    TestParse("GET /index.html HTTP/1.1\r\n", true, "GET", "/index.html", "1.1");

    // With query string
    TestParse("GET /search?q=abc&lang=en HTTP/1.0\r\n", true, "GET", "/search", "1.0", {{"q", "abc"}, {"lang", "en"}});

    // Encoded query values and keys
    TestParse("GET /path?name=John+Doe&tag=a%26b HTTP/1.1\r\n", true, "GET", "/path", "1.1",
              {{"name", "John Doe"}, {"tag", "a&b"}});

    // Missing HTTP version -> fail
    TestParse("GET /nover\r\n", false);

    // Unsupported/malformed method -> fail (lowercase)
    TestParse("get /lower HTTP/1.1\r\n", false);

    // Request line too long
    string long_uri(9000, 'a');
    string long_line = string("GET /") + long_uri + " HTTP/1.1\r\n";
    TestParse(long_line, false);

    // URI with multiple ? characters: path includes first part, rest becomes query
    TestParse("GET /a?b=c?d=e HTTP/1.1\r\n", true, "GET", "/a", "1.1", {{"b", "c?d=e"}});

    // Parameter without value
    TestParse("GET /pv?flag HTTP/1.1\r\n", true, "GET", "/pv", "1.1", {{"flag", ""}});

    // Asterisk form (OPTIONS *) - should be supported by our relaxed regex; expect OK
    TestParse("OPTIONS * HTTP/1.1\r\n", true, "OPTIONS", "*", "1.1");

    // Encoded path (UTF-8 Chinese)
    TestParse("GET /%E4%B8%AD%E6%96%87 HTTP/1.1\r\n", true, "GET", "/中文", "1.1");

    // Encoded spaces in path (%20) should decode to spaces; plus is not decoded in path
    TestParse("GET /path%20with%20space HTTP/1.1\r\n", true, "GET", "/path with space", "1.1");

    // Encoded slash in path should be decoded
    TestParse("GET /a%2Fb HTTP/1.1\r\n", true, "GET", "/a/b", "1.1");

    cout << "All http_context tests passed." << endl;
    return 0;
}
