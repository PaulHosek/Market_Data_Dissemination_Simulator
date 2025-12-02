//
// Created by paul on 01-Dec-25.
//

#include "Disseminator.h"

#include <thread>
#include <boost/preprocessor/arithmetic/add.hpp>

#include "../utils/types.h"

Disseminator::Disseminator(types::QueueType_Quote& quotes, types::QueueType_Trade& trades)
    : trades_(trades),
      quotes_(quotes)
{}

Disseminator::~Disseminator() {
    stop();
}


void Disseminator::start() {
    process();
}

void Disseminator::stop() {
    for (auto& worker : workers_) {
        worker.request_stop();
    }
    workers_.clear();
}

void Disseminator::process() {
    if (!workers_.empty()){
        return;
    }
    workers_.emplace_back([this](std::stop_token st) {consume_quotes(quotes_, st);});
    workers_.emplace_back([this](std::stop_token st) {consume_trades(trades_,st);});
}


// makes not much sense to have template here since it will only be quotes -> maybe have one generic function instead
void Disseminator::consume_quotes(types::QueueType_Quote& q, const std::stop_token& stoken) {

    types::Quote qt;
    while (!stoken.stop_requested()) {
        if (q.pop(qt)) {
            symbols_quotes_[qt.symbol].push(std::move(qt));
            // TODO: forward to subscribers here
        } else {
            q.wait();
        }
    }
}

void Disseminator::consume_trades(types::QueueType_Trade& q, const std::stop_token& stoken)
{
    types::Trade tr;
    while (!stoken.stop_requested()) {
        if (q.pop(tr)) {
            symbols_trades_[tr.symbol].push(std::move(tr));
            // TODO: forward to subscribers here
        } else {
            q.wait();
        }
    }
}





// TODO specific case first with 1 queueue type, then convert using template and concept or compile type polymorphism

