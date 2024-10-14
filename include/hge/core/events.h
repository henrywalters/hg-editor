//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_EVENTS_H
#define HGEDITOR_EVENTS_H

#include <hagame/utils/enum.h>
#include <hagame/utils/pubsub.h>
#include <hagame/core/entity.h>
#include <hagame/utils/file.h>

namespace hge {

    ENUM(EventTypes)
    ENUM_VALUE(EventTypes, SelectEntity)
    ENUM_VALUE(EventTypes, AddEntity)
    ENUM_VALUE(EventTypes, AddChild)
    ENUM_VALUE(EventTypes, AddChildTo)
    ENUM_VALUE(EventTypes, RemoveEntity)
    ENUM_VALUE(EventTypes, DuplicateEntity)
    ENUM_VALUE(EventTypes, AddComponent)
    ENUM_VALUE(EventTypes, UpdateComponent)
    ENUM_VALUE(EventTypes, RemoveComponent)
    ENUM_VALUE(EventTypes, SelectAsset)
    ENUM_VALUE(EventTypes, NewScene)
    ENUM_VALUE(EventTypes, LoadScene)
    ENUM_VALUE(EventTypes, SaveScene)

    struct EntityEvent {
        hg::Entity* entity;
        hg::Entity* target;
    };

    struct ComponentEvent {
        hg::Entity* entity;
        std::string component;
        std::string field;
    };

    using AssetEvent = hg::utils::FileParts;

    using LevelEvent = hg::Scene*;

    struct Event {
        std::variant<EntityEvent, ComponentEvent, AssetEvent, LevelEvent> payload;
    };

    hg::Publisher<EventTypes::type, Event>* Events();
}


#endif //HGEDITOR_EVENTS_H
