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
            text_render_data.clear();
            text_render_times.clear();

            text_render_data += "shader/rop/tex/calls: " +
                    ks::ToString(shader_switches) + "/" +
                    ks::ToString(raster_ops)+ "/" +
                    ks::ToString(texture_switches) + "/" +
                    ks::ToString(draw_calls)+
                    "\n";

            text_render_times += "render: " + ks::ToStringFormat(render_ms,3,7,'0') + "ms\n";
        }

        void RenderStats::GenUpdateText()
        {
            text_update_data.clear();
            text_update_times.clear();

            text_update_times += "update: " + ks::ToStringFormat(update_ms,3,7,'0') + "ms\n";
            text_update_times += "sync: " + ks::ToStringFormat(sync_ms,3,7,'0') + "ms\n";
            text_update_data += "buffer count/mem: " + ks::ToString(buffer_count) +
                           "/" + ks::ToString(buffer_mem_bytes) + " bytes";
        }

        void RenderStats::GenCustomText()
        {
            text_custom = custom_info;
        }
    }
}
