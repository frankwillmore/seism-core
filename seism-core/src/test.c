#include <stdio.h>

int main(){

#ifdef H5_SUBFILING
    printf("defined.\n");
#endif
    return 9;
}
