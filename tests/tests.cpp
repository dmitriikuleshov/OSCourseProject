#include "tools.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

// Fixture for tests
class SharedMemoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Clear the file before each test
        std::ofstream ofs(MAIN_FILE_NAME, std::ofstream::trunc);
        ofs.close();
    }
};

// Tests for `sem_setvalue`
TEST(SemaphoreTests, SetInitialValue) {
    sem_t semaphore;
    sem_init(&semaphore, 0, 0);

    EXPECT_EQ(sem_setvalue(&semaphore, 5), 5);
    int value = 0;
    sem_getvalue(&semaphore, &value);
    EXPECT_EQ(value, 5);

    sem_destroy(&semaphore);
}

TEST(SemaphoreTests, IncreaseValue) {
    sem_t semaphore;
    sem_init(&semaphore, 0, 2);

    EXPECT_EQ(sem_setvalue(&semaphore, 7), 7);
    int value = 0;
    sem_getvalue(&semaphore, &value);
    EXPECT_EQ(value, 7);

    sem_destroy(&semaphore);
}

TEST(SemaphoreTests, DecreaseValue) {
    sem_t semaphore;
    sem_init(&semaphore, 0, 8);

    EXPECT_EQ(sem_setvalue(&semaphore, 3), 3);
    int value = 0;
    sem_getvalue(&semaphore, &value);
    EXPECT_EQ(value, 3);

    sem_destroy(&semaphore);
}

// Tests for `shm_open_file`
TEST(SharedMemoryTests, CreateFile) {
    int fd = shm_open_file("test_file");
    ASSERT_NE(fd, -1);
    struct stat statBuf;
    fstat(fd, &statBuf);
    EXPECT_EQ(statBuf.st_size, 0);
    close(fd);
    shm_unlink("test_file");
}

TEST(SharedMemoryTests, AccessExistingFile) {
    int fd1 = shm_open_file("test_file");
    close(fd1);

    int fd2 = shm_open_file("test_file");
    ASSERT_NE(fd2, -1);
    close(fd2);
    shm_unlink("test_file");
}

TEST(SharedMemoryTests, FileAccessPermissions) {
    int fd = shm_open_file("test_file");
    struct stat statBuf;
    fstat(fd, &statBuf);
    EXPECT_EQ(statBuf.st_mode & ACCESS_PERM, ACCESS_PERM);
    close(fd);
    shm_unlink("test_file");
}

// Tests for `read_from`
TEST_F(SharedMemoryTest, ReadValidJson) {
    nlohmann::json testJson = {{"key", "value"}};
    write_to(MAIN_FILE_NAME, testJson);

    nlohmann::json result = read_from(MAIN_FILE_NAME);
    EXPECT_EQ(result, testJson);
}

TEST_F(SharedMemoryTest, ReadInvalidJson) {
    std::ofstream ofs(MAIN_FILE_NAME);
    ofs << "Invalid JSON";
    ofs.close();

    // It can read everything
    EXPECT_NO_THROW(read_from(MAIN_FILE_NAME));
}

TEST_F(SharedMemoryTest, WriteValidJson) {
    nlohmann::json testJson = {{"key", "value"}};
    write_to(MAIN_FILE_NAME, testJson);

    nlohmann::json testRead = read_from(MAIN_FILE_NAME);

    EXPECT_EQ(testJson, testRead);
}

TEST_F(SharedMemoryTest, OverwriteExistingJson) {
    nlohmann::json initialJson = {{"key1", "value1"}};
    write_to(MAIN_FILE_NAME, initialJson);

    nlohmann::json updatedJson = {{"key2", "value2"}};
    write_to(MAIN_FILE_NAME, updatedJson);

    nlohmann::json result = read_from(MAIN_FILE_NAME);
    EXPECT_EQ(result, updatedJson);
}

TEST_F(SharedMemoryTest, WriteEmptyJson) {
    nlohmann::json emptyJson;
    write_to(MAIN_FILE_NAME, emptyJson);

    std::ifstream ifs(MAIN_FILE_NAME);
    std::string fileContent((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());

    EXPECT_EQ(fileContent, ""); // "{}"
}
