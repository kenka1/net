#pragma once

#include <semaphore.h>

#include <cstring>
#include <expected>
#include <print>
#include <system_error>

namespace net::core {

class Semaphore {
  public:
    Semaphore() {
        int res, saved_errno;
        do {
            res = sem_init(&sem_, 1, 1);
            if (res == -1) {
                saved_errno = errno;
                continue;
            }

        } while (res == -1 && saved_errno == EINTR);

        if (res == -1)
            throw std::system_error(std::error_code(saved_errno, std::generic_category()));
    }

    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    Semaphore(Semaphore &&other) noexcept = delete;
    Semaphore &operator=(Semaphore &&other) = delete;

    ~Semaphore() {
        if (sem_destroy(&sem_) == -1) {
            const int saved_errno = errno;
            std::println(stderr, "Error code: {} {}", saved_errno, ::strerror(saved_errno));
        }
    }

    std::expected<void, std::error_code> Wait() noexcept {
        int res, saved_errno;
        do {
            res = sem_wait(&sem_);
            if (res == -1) {
                saved_errno = errno;
                continue;
            }
        } while (res == -1 && saved_errno == EINTR);

        if (res == -1)
            return std::unexpected(std::error_code(saved_errno, std::generic_category()));

        return {};
    }

    std::expected<void, std::error_code> Post() noexcept {
        int res, saved_errno;
        do {
            res = sem_post(&sem_);
            if (res == -1) {
                saved_errno = errno;
                continue;
            }
        } while (res == -1 && saved_errno == EINTR);

        if (res == -1)
            return std::unexpected(std::error_code(saved_errno, std::generic_category()));

        return {};
    }

  private:
    sem_t sem_;
};

class SemLockGuard {
  public:
    SemLockGuard(Semaphore &sem) noexcept : sem_(sem) {
        if (auto wait_res = sem_.Wait(); !wait_res)
            std::println(stderr, "Error code: {} {}", wait_res.error().value(),
                         wait_res.error().message());
    }

    SemLockGuard(const SemLockGuard &) = delete;
    SemLockGuard &operator=(const SemLockGuard &) = delete;
    SemLockGuard(SemLockGuard &&) = delete;
    SemLockGuard &operator=(SemLockGuard &&) = delete;

    ~SemLockGuard() {
        if (auto post_res = sem_.Post(); !post_res)
            std::println(stderr, "Error code: {} {}", post_res.error().value(),
                         post_res.error().message());
    }

  private:
    Semaphore &sem_;
};
}  // namespace net::core
