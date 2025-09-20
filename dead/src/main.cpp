#include <iostream>

int main(int argc,char* argv[]){

    if(argc != 2){
        std::cerr<<"Incorrect usage, Correct usage is ...."<<std::endl;
        std::cerr<<"dead <input.dd>"<<std::endl;
        return EXIT_FAILURE;
    }
    std::cout << argv[1] << std::endl;
    return EXIT_SUCCESS;
}
