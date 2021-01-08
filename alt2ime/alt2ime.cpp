
#include <stdio.h>
#include "Stuff.h"
#include <system_error>

Stuff stuff;


int main()
{
	printf("----ALT2IME----\n");
	::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
		try
		{
			if (event == CTRL_C_EVENT)
			{
				printf("Break\n");
				stuff.Stop();
				return TRUE;
			}
		}
		catch (std::system_error& e)
		{
			printf("Error: %s\n", e.what());
		}
		return FALSE;
	}, TRUE);

	stuff.Run();

	printf("---- ---- ---- ----\n");
	return 0;

}

