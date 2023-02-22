#include<string>

#include "CMemoryPool.h"

int main(){
    MemoryPool<std::string> pool {1L << 20};
}