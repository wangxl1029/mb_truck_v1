//
//  main.cpp
//  MidCompiler
//
//  Created by alan king on 16/6/10.
//  Copyright © 2016年 alan king. All rights reserved.
//

#include <iostream>
#include "MidCompiler.h"
const char root[] = "/Users/alanking/Desktop/mapbar/15Q3";
const char* provs[] = {
    "beijing",
    "shanghai",
    "liaoning"
};

int main(int argc, const char * argv[]) {
    // insert code here...
    CMidCompiler comp(root);
    
    ;
    if (comp.scan(provs, sizeof(provs)/sizeof(provs[0]))) {
        std::cout << "scan ok." << std::endl;
#if 1
        if (comp.test()) {
            std::cout << "test ok." << std::endl;
        }else{
            std::cerr << "test failed!" << std::endl;
        }
#endif
    }
    else
    {
        std::cerr << "scan failed!" << std::endl;
    }
   
    return 0;
}
