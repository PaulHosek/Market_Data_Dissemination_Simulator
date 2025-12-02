//
// Created by paul on 01-Dec-25.
//

#ifndef IDISSEMINATOR_H
#define IDISSEMINATOR_H

#endif //IDISSEMINATOR_H

class IDisseminator {
public:
    virtual ~IDisseminator() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void process() = 0;
};