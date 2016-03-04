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

#ifndef KS_DRAW_SYSTEM_HPP
#define KS_DRAW_SYSTEM_HPP

#include <ks/KsGlobal.hpp>

namespace ks
{
    namespace draw
    {
        class System
        {
        public:
            System() = default;

            virtual ~System() = default;

            virtual std::string GetDesc() const = 0;

            virtual void Update(TimePoint const &prev_time,
                                TimePoint const &curr_time) = 0;
        };
    }
}

#endif // KS_DRAW_SYSTEM_HPP
