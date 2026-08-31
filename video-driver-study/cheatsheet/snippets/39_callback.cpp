/* S39 | C++17
 * 실제 callback typedef/ABI에 맞출 것. Reader 구현·등록 API 별도.
 * Educational snippet; not compiled or hardware-tested.
 */

class Reader {
public:
    void completed(int status) noexcept;
};

extern "C" void done(
    void* user, int status) noexcept
{
    if (!user) return;
    auto* r = static_cast<Reader*>(user);
    r->completed(status);
}
/* register_callback(&done, &reader); */
