/*
   Copyright (C) 2015 Preet Desai (preet.desai@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <catch/catch.hpp>

#include <ks/draw/KsDrawDefaultDrawKey.hpp>

using namespace ks;
using namespace ks::draw;

TEST_CASE("KsTestDrawKey","[kstestdrawkey]")
{
    DefaultDrawKey key;

    key.SetShader(2);
    key.SetDepthConfig(3);
    key.SetBlendConfig(4);
    key.SetStencilConfig(5);
    key.SetTextureSet(6);
    key.SetUniformSet(7);
    key.SetPrimitive(ks::gl::Primitive::Triangles);

    REQUIRE(key.GetShader() == 2);
    REQUIRE(key.GetDepthConfig() == 3);
    REQUIRE(key.GetBlendConfig() == 4);
    REQUIRE(key.GetStencilConfig() == 5);
    REQUIRE(key.GetTextureSet() == 6);
    REQUIRE(key.GetUniformSet() == 7);
    REQUIRE(key.GetPrimitive() == gl::Primitive::Triangles);
}
