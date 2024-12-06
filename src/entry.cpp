#include "testing/test_cases.h"

#include "testing/test_cases.h"
int main()
{
    int i = 0;

    while (1)
    {
        test_all(get_abi(), i);

        ++i;
    }
}