#include <stdio.h>
#include "src/tree.h"
#include "src/parse.h"

extern union node* functions;

int main() {
    printf("functions = %p\n", (void*)functions);
    
    if(functions) {
        union node* nptr;
        for(nptr = functions; nptr; nptr = nptr->next) {
            struct nfunc* fn = &nptr->nfunc;
            printf("  Function: %s at %p, next=%p\n", fn->name, (void*)nptr, (void*)nptr->next);
        }
    }
    
    return 0;
}
