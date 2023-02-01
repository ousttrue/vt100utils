#include "../demos/tokenizer.h"
#include <catch2/catch_test_macros.hpp>

auto src = R"(abc; aga;;jkjk)";

TEST_CASE("string tests", "[Tokenizer]") {

  Tokenizer tok(src, ';');

  REQUIRE(tok.next());
  REQUIRE(tok.current() == "abc");
  REQUIRE(tok.next());
  REQUIRE(tok.current() == " aga");
  REQUIRE(tok.next());
  REQUIRE(tok.current() == "");
  REQUIRE(tok.next());
  REQUIRE(tok.current() == "jkjk");
}
