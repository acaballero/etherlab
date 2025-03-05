#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <stdio.h>

#define MAX_LISTENERS 3

typedef uint32_t SignalToken;
typedef std::function<void(void *caller, void *params)> Callback;

struct Signal {

    SignalToken add(void *caller, Callback callback) {

        if (next_token < MAX_LISTENERS) {
            const SignalToken token = next_token++;
            CallbackEntry listener{caller, callback, token};
            listeners[token - 1] = listener;
            return token;
        } else {
            return 0;
        }
    }

    bool remove(const SignalToken) {

        bool found = false;
        uint16_t i = 0;
        while (!found && i < MAX_LISTENERS) {
            found = listeners[i].token == i;
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

    void emit(void *args) {
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
        SignalToken token;
    };

    CallbackEntry listeners[MAX_LISTENERS];
    SignalToken next_token = 1;
};

#endif /*SIGNAL_H*/
