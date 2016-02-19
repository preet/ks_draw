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

#ifndef KS_DRAW_RENDER_STATS_HPP
#define KS_DRAW_RENDER_STATS_HPP

#include <ks/KsGlobal.hpp>

namespace ks
{
    namespace draw
    {
        struct RenderStats final
        {
            RenderStats();
            ~RenderStats() = default;

            void ClearRenderStats();
            void ClearUpdateStats();
            void SetCustomInfo(std::string custom_info);
            void GenRenderText();
            void GenUpdateText();
            void GenCustomText();

            // collected during render
            double render_ms;
            uint shader_switches;
            uint texture_switches;
            uint raster_ops;
            uint draw_calls;

            // collected during update and sync
            double update_ms;
            double sync_ms;
            uint buffer_count;
            uint buffer_mem_bytes;
            uint texture_count;
            uint texture_mem_bytes;

            // set by the rendersystem
            std::string custom_info;

            std::string text_render_times;
            std::string text_update_times;
            std::string text_render_data;
            std::string text_update_data;
            std::string text_custom;
        };
    }
}

#endif // KS_DRAW_RENDER_STATS_HPP
