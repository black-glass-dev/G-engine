#include <doctest/doctest.h>

#include <string>

#include "lumen/render/builtin_shaders.hpp"
#include "lumen/render/shader.hpp"

using lumen::render::adapt_shader_source;

TEST_CASE("desktop GL rewrites GLSL ES 3.00 version line to 330 core") {
    const std::string out = adapt_shader_source("#version 300 es\nvoid main(){}\n", false);
    CHECK(out == "#version 330 core\nvoid main(){}\n");
}

TEST_CASE("GLES keeps the ES version line") {
    const std::string src = "#version 300 es\nvoid main(){}\n";
    CHECK(adapt_shader_source(src, true) == src);
}

TEST_CASE("source without a version line gets one prepended") {
    CHECK(adapt_shader_source("void main(){}", false) == "#version 330 core\nvoid main(){}");
    CHECK(adapt_shader_source("void main(){}", true) == "#version 300 es\nvoid main(){}");
}

TEST_CASE("built-in shaders are authored as GLSL ES 3.00") {
    using namespace lumen::render::builtin;
    CHECK(std::string(kColorVertex).rfind("#version 300 es", 0) == 0);
    CHECK(std::string(kColorFragment).rfind("#version 300 es", 0) == 0);
    CHECK(std::string(kColorVertex).find("u_angle") != std::string::npos);
}
