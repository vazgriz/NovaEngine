#pragma once

namespace Nova {
    class ISystem {
    public:
        virtual void update(float delta) = 0;
    };
}