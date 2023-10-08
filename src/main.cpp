
#ifdef _WIN32
	#include <windows.h>
#endif

#include <test/game.hpp>

int main(int /*argc*/, char* /*argv*/[]) {
#ifdef _WIN32
	SetConsoleTitle(L"Test");
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	test::TestGame game;
	game.init();

	return 0;
}
