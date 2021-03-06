#ifndef TESTENGINESFML_FILEREADERUTIL_H
#define TESTENGINESFML_FILEREADERUTIL_H

#include <string>
#include <fstream>

namespace urchin
{

    class FileReaderUtil
    {
        public:
            static void nextLine(std::ifstream &, std::string &);

        private:
            FileReaderUtil() = default;
            ~FileReaderUtil() = default;
    };

}

#endif
