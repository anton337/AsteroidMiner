/*
 * =====================================================================================
 *
 *       Filename:  triangulate.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/04/2013 02:26:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{

    if(argc!=3)
    {
        std::cout << "try: ./a.out lower upper" << std::endl;
    }

    int lower = atoi(argv[1]);

    int upper = atoi(argv[2]);

    char input[1024];

    int num_objs = 0;

    while(std::cin.getline(input,1024))
    {
        if(input[0] == 'o')
        {
            if(num_objs > upper)
            {
                return 0;
            }
            num_objs++;
        }
        if(num_objs <= lower)
        {
            continue;
        }
        if(input[0] != 'f')
        {
            if(input[0] == 'v' && input[1] == ' ')
            {
                std::cout << input << std::endl;
            }
        }
        else
        {
            std::string backup = input;
            char* chars_array;
            std::vector<char*> v;
            chars_array = strtok(input," ");
            while(chars_array != NULL)
            {
                v.push_back(chars_array);
                chars_array = strtok(NULL," ");
            }
            for(int i=0;i<v.size();i++)
            {
                chars_array = strtok(v[i],"/");
                v[i] = chars_array;
            }
            if(v.size()==5)
            {
                std::cout << v[0] << " " << v[1] << " " << v[2] << " " << v[4] << std::endl;
                std::cout << v[0] << " " << v[4] << " " << v[2] << " " << v[3] << std::endl;
            }
            else
            if(v.size()==4)
            {
                std::cout << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << std::endl;
            }
            else
            {
//                std::cout << v[0] << " " << v[1] << " " << v[2] << " " << v[4] << std::endl;
//                std::cout << v[0] << " " << v[4] << " " << v[2] << " " << v[3] << std::endl;

                std::cout << "error: " << "v.size=" << v.size() << " ; " << backup << std::endl;
                for(int i=0;i<v.size();i++)
                    std::cout << i << ") " << v[i] << "; " << std::endl;
                return 1;
            }
        }
    }

    return 0;

}

