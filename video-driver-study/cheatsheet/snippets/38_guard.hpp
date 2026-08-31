#ifndef STUDY_SNIPPET_38_GUARD_HPP
#define STUDY_SNIPPET_38_GUARD_HPP

/* S38 | C++17
 * task 전용. lock은 획득 후 반환, unlock은 실패·예외 없음. 플랫폼 구현 필요.
 * Educational snippet; not compiled or hardware-tested.
 */

struct Mutex;
void lock(Mutex*) noexcept;
void unlock(Mutex*) noexcept;

class Guard {
public:
    explicit Guard(Mutex& m) noexcept
        : m_(m) { lock(&m_); }
    ~Guard() noexcept { unlock(&m_); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;
private:
    Mutex& m_;
};

#endif /* STUDY_SNIPPET_38_GUARD_HPP */
