//
// Created by henry on 10/3/24.
//
#include "hge/core/events.h"

using namespace hge;

hg::Publisher<EventTypes::type, Event> *hge::Events() {
    static std::unique_ptr<hg::Publisher<EventTypes::type, Event>> events;

    if (!events) {
        events = std::make_unique<hg::Publisher<EventTypes::type, Event>>();
    }

    return events.get();
}
