#pragma once

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <semaphore.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "player.hpp"
#include "session.hpp"

const std::string MAIN_FILE_NAME = "main.back";
const std::string MAIN_SEM_NAME = "main.semaphore";
int ACCESS_PERM = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;

int sem_setvalue(sem_t *semaphore, int state) {
    if (!semaphore) {
        throw std::invalid_argument("Semaphore is null.");
    }

    std::mutex mx;
    int s = 0;
    if (sem_getvalue(semaphore, &s) == -1) {
        throw std::runtime_error("Failed to get semaphore value.");
    }

    mx.lock();
    while (s++ < state) {
        if (sem_post(semaphore) == -1) {
            mx.unlock();
            throw std::runtime_error("Failed to increment semaphore value.");
        }
    }
    while (s-- > state + 1) {
        if (sem_wait(semaphore) == -1) {
            mx.unlock();
            throw std::runtime_error("Failed to decrement semaphore value.");
        }
    }
    mx.unlock();

    return s;
}

int shm_open_file(const std::string &filename) {
    int fd = shm_open(filename.c_str(), O_RDWR | O_CREAT, ACCESS_PERM);
    if (fd == -1) {
        throw std::runtime_error(
            "Failed to open or create shared memory file: " + filename);
    }
    return fd;
}

nlohmann::json read_from(const std::string &filename) {
    int fd = shm_open_file(filename);

    struct stat statBuf;
    if (fstat(fd, &statBuf) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get file stats for: " + filename);
    }

    if (statBuf.st_size == 0) {
        close(fd);
        return nlohmann::json(); // Return an empty JSON object
    }

    char *mapped = static_cast<char *>(
        mmap(NULL, statBuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (mapped == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map shared memory for reading.");
    }

    std::string strToJson(mapped, statBuf.st_size);
    munmap(mapped, statBuf.st_size);
    close(fd);

    try {
        return nlohmann::json::parse(strToJson);
    } catch (const nlohmann::json::parse_error &e) {
        throw std::runtime_error("Invalid JSON format in file: " + filename);
    }
}

void write_to(const std::string &filename, const nlohmann::json &data) {
    std::string strFromJson = data.dump();
    size_t sz = strFromJson.size();

    int fd = shm_open_file(filename);

    if (ftruncate(fd, sz) == -1) {
        close(fd);
        throw std::runtime_error("Failed to resize shared memory file: " +
                                 filename);
    }

    char *mapped = static_cast<char *>(
        mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (mapped == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map shared memory for writing.");
    }

    // Write data to the mapped memory
    memcpy(mapped, strFromJson.c_str(), sz);

    // Synchronize memory with the file
    if (msync(mapped, sz, MS_SYNC) == -1) {
        munmap(mapped, sz);
        close(fd);
        throw std::runtime_error("Failed to sync memory to file.");
    }

    munmap(mapped, sz);
    close(fd);
}