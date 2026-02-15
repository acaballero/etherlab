#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <stdio.h>
#include "printf.h"
#define MAX_LISTENERS 10

typedef uint32_t SignalToken;
typedef std::function<void(void *caller, const void *params)> Callback;

struct Signal {

    std::string name;

    Signal() {
        for (int i = 0; i < MAX_LISTENERS; i++) {
            listeners[i].token = 0;
        }
    };
    Signal(std::string name) {
        this->name = name;
    }

    SignalToken add(void *caller, Callback callback) {

        SignalToken token = find_token();

        if (token) {
            CallbackEntry listener{caller, callback, token};
            listeners[token - 1] = listener;
            return token;
        } else {
            return 0;
        }
    }

    SignalToken find_token() {
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (!listeners[i].token) {
                return i + 1;
            }
        }

        return 0;
    }

    bool remove(const SignalToken token) {

        bool found = false;
        uint16_t i = 0;
        while (!found && i < MAX_LISTENERS) {
            found = listeners[i].token == token;
            if (!found) {
                i++;
            }
        }
        if (found) {
            while (i < MAX_LISTENERS - 1) {
                listeners[i] = listeners[i + 1];
                i++;
            }

            listeners[MAX_LISTENERS - 1].token = 0;
            listeners[MAX_LISTENERS - 1].caller = nullptr;
        }

        return found;
    }

    void emit(const void *args) {

        int i = 0;
        while (listeners[i].token > 0) {
            listeners[i].callback(listeners[i].caller, args);
            i++;
        }
    }

  private:
    struct CallbackEntry {
        void *caller;
        Callback callback;
        SignalToken token{0};
    };

    CallbackEntry listeners[MAX_LISTENERS];
};

#endif /*SIGNAL_H*/
