//
// Created by Nikita on 02.05.2021.
//

#ifndef EVOVULKAN_VULKANOBJECT_H
#define EVOVULKAN_VULKANOBJECT_H

#include <combaseapi.h>

namespace EvoVulkan::Types {
    interface IVkObject {
        [[nodiscard]] virtual bool IsReady()    const { return false; };
        [[nodiscard]] virtual bool IsComplete() const { return false; };

        virtual void Destroy()                        { };
        virtual void Free()                           { };
    };
}

#endif //EVOVULKAN_VULKANOBJECT_H