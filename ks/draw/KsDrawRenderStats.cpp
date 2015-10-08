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

#include <ks/draw/KsDrawRenderStats.hpp>

namespace ks
{
    namespace draw
    {
        RenderStats::RenderStats()
        {
            ClearRenderStats();
            ClearUpdateStats();
        }

        void RenderStats::ClearRenderStats()
        {
            render_ms = 0.0;
            shader_switches = 0;
            texture_switches = 0;
            raster_ops = 0;
            draw_calls = 0;
        }

        void RenderStats::ClearUpdateStats()
        {
            update_ms = 0;
            sync_ms = 0;
            buffer_count = 0;
            buffer_mem_bytes = 0;
            texture_count = 0;
            texture_mem_bytes = 0;
        }

        void RenderStats::GenRenderText()
        {
            render_text.clear();

            render_text += "render: " + ks::to_string_format(render_ms,3,7,'0') + "ms\n";
            render_text += "shader switches: " + ks::to_string(shader_switches) + "\n";
            render_text += "draw calls: " + ks::to_string(draw_calls) + "\n";
        }

        void RenderStats::GenUpdateText()
        {
            update_text.clear();

            update_text += "update: " + ks::to_string_format(update_ms,3,7,'0') + "ms\n";
            update_text += "sync: " + ks::to_string_format(sync_ms,3,7,'0') + "ms\n";
            update_text += "buffer count/mem: " + ks::to_string(buffer_count) +
                           "/" + ks::to_string(buffer_mem_bytes) + " bytes";
        }
    }
}
