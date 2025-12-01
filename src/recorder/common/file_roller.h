//
// Created by xucong on 25-5-30.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#ifndef FILE_ROLLER_H
#define FILE_ROLLER_H

#include <string>
#include <vector>
#include <optional>
#include "common/config/app_config.h"

namespace dcl {
namespace recorder {

class FileRoller {
public:
    FileRoller();
    ~FileRoller() = default;

    int rollFiles();

private:
    std::string bagPath;

    std::vector<std::string> getSortedCompressedFiles() const;

};

}
}
#endif //FILE_ROLLER_H
