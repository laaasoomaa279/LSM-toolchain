#ifndef LSM_CORE_RESULT_HPP
#define LSM_CORE_RESULT_HPP

#include <utility>
#include <new>
#include <string>

template <typename T, typename E = std::string>
class Result {
private:
    union {
        T valuePayload;
        E errorPayload;
    };
    bool isOkState;

public:
    Result(const T& val) : isOkState(true) {
        new (&valuePayload) T(val);
    }

    Result(T&& val) : isOkState(true) {
        new (&valuePayload) T(std::move(val));
    }

    struct ErrWrapper { E error; };
    Result(ErrWrapper err) : isOkState(false) {
        new (&errorPayload) E(std::move(err.error));
    }

    ~Result() {
        if (isOkState) {
            valuePayload.~T();
        } else {
            errorPayload.~E();
        }
    }

    Result(const Result& other) : isOkState(other.isOkState) {
        if (isOkState) new (&valuePayload) T(other.valuePayload);
        else new (&errorPayload) E(other.errorPayload);
    }

    Result(Result&& other) noexcept : isOkState(other.isOkState) {
        if (isOkState) new (&valuePayload) T(std::move(other.valuePayload));
        else new (&errorPayload) E(std::move(other.errorPayload));
    }

    Result& operator=(const Result& other) {
        if (this != &other) {
            if (isOkState) valuePayload.~T();
            else errorPayload.~E();

            isOkState = other.isOkState;
            if (isOkState) new (&valuePayload) T(other.valuePayload);
            else new (&errorPayload) E(other.errorPayload);
        }
        return *this;
    }

    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (isOkState) valuePayload.~T();
            else errorPayload.~E();

            isOkState = other.isOkState;
            if (isOkState) new (&valuePayload) T(std::move(other.valuePayload));
            else new (&errorPayload) E(std::move(other.errorPayload));
        }
        return *this;
    }

    static Result<T, E> Ok(T val) {
        return Result<T, E>(std::move(val));
    }

    static Result<T, E> Err(E err) {
        return Result<T, E>(ErrWrapper{std::move(err)});
    }

    bool isOk() const { return isOkState; }
    bool isErr() const { return !isOkState; }

    T& unwrap() {
        return valuePayload;
    }

    const T& unwrap() const {
        return valuePayload;
    }

    E& unwrapErr() {
        return errorPayload;
    }

    const E& unwrapErr() const {
        return errorPayload;
    }
};


template <typename E>
class Result<void, E> {
private:
    E errorPayload;
    bool isOkState;

public:
    Result() : isOkState(true) {}
    Result(E err) : errorPayload(std::move(err)), isOkState(false) {}

    static Result<void, E> Ok() { return Result<void, E>(); }
    static Result<void, E> Err(E err) { return Result<void, E>(std::move(err)); }

    bool isOk() const { return isOkState; }
    bool isErr() const { return !isOkState; }
    const E& unwrapErr() const { return errorPayload; }
};

#endif 