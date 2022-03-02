#pragma once

class IComponent{
public:
    virtual void Execute() = 0;
    virtual ~IComponent(){}
};