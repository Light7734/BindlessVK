#include <tracy/Tracy.hpp>

void *operator new(std ::size_t count)
{
	auto ptr = malloc(count);
	TracyAlloc(ptr, count);
	return ptr;
}
void operator delete(void *ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}
