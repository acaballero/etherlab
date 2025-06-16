//
// Created by Angel Dust on 15/06/2025
//

#include <utility>

template <typename TValue, typename TError> struct Result {
    enum class Type {
        Success,
        Error,
    } type;

    union {
        TValue value_;
        TError error_;
    };

    bool is_ok() const {
        return type == Type::Success;
    }

    operator bool() const {
        return is_ok();
    }

    bool is_error() const {
        return type == Type::Error;
    }

    const TValue &value() const & {
        return value_;
    }

    const TValue &operator*() const & {
        return value_;
    }

    TValue &&operator*() && {
        return std::move(value_);
    }

    const TError &error() const & {
        return error_;
    }

    Result() = delete;

    constexpr Result(TValue &&value) : type{Type::Success}, value_{std::forward<TValue>(value)} {
    }

    constexpr Result(TError error) : type{Type::Error}, error_{error} {
    }

    ~Result() {
        if (is_ok()) {
            value_.~TValue();
        } else {
            error_.~TError();
        }
    }
};
