#include <type_traits>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

#include "file_system/managed_file_descriptor.hpp"
#include "gtest/gtest.h"

namespace {

[[nodiscard]] int open_read_only_null()
{
    return ::open("/dev/null", O_RDONLY);
}

[[nodiscard]] bool descriptor_is_valid(int fd)
{
    return ::fcntl(fd, F_GETFD) != -1;
}

}  // namespace

TEST(ManagedFileDescriptor, DefaultConstructedIsClosed)
{
    const file_system::ManagedFileDescriptor file;

    EXPECT_FALSE(file.is_open());
    EXPECT_EQ(file.file_descriptor(), -1);
}

TEST(ManagedFileDescriptor, NegativeDescriptorIsTreatedAsClosed)
{
    const file_system::ManagedFileDescriptor closed{-1};
    const file_system::ManagedFileDescriptor also_closed{-2};

    EXPECT_FALSE(closed.is_open());
    EXPECT_EQ(closed.file_descriptor(), -1);
    EXPECT_FALSE(also_closed.is_open());
    EXPECT_EQ(also_closed.file_descriptor(), -2);
}

TEST(ManagedFileDescriptor, OwnsProvidedDescriptor)
{
    const int fd = open_read_only_null();
    ASSERT_GE(fd, 0);

    const file_system::ManagedFileDescriptor file{fd};

    EXPECT_TRUE(file.is_open());
    EXPECT_EQ(file.file_descriptor(), fd);
    EXPECT_TRUE(descriptor_is_valid(fd));
}

TEST(ManagedFileDescriptor, DestructorClosesOwnedDescriptor)
{
    int fd = -1;
    {
        file_system::ManagedFileDescriptor file{open_read_only_null()};
        fd = file.file_descriptor();
        ASSERT_GE(fd, 0);
        ASSERT_TRUE(descriptor_is_valid(fd));
    }

    EXPECT_FALSE(descriptor_is_valid(fd));
}

TEST(ManagedFileDescriptor, MoveConstructorTransfersOwnership)
{
    file_system::ManagedFileDescriptor source{open_read_only_null()};
    const int fd = source.file_descriptor();
    ASSERT_GE(fd, 0);

    file_system::ManagedFileDescriptor destination{std::move(source)};

    EXPECT_FALSE(source.is_open());
    EXPECT_EQ(source.file_descriptor(), -1);
    EXPECT_TRUE(destination.is_open());
    EXPECT_EQ(destination.file_descriptor(), fd);
    EXPECT_TRUE(descriptor_is_valid(fd));
}

TEST(ManagedFileDescriptor, MoveConstructorFromClosedSourceRemainsClosed)
{
    file_system::ManagedFileDescriptor source;
    file_system::ManagedFileDescriptor destination{std::move(source)};

    EXPECT_FALSE(source.is_open());
    EXPECT_EQ(source.file_descriptor(), -1);
    EXPECT_FALSE(destination.is_open());
    EXPECT_EQ(destination.file_descriptor(), -1);
}

TEST(ManagedFileDescriptor, MoveAssignmentClosesPreviousDescriptor)
{
    file_system::ManagedFileDescriptor destination{open_read_only_null()};
    const int previous_fd = destination.file_descriptor();
    ASSERT_GE(previous_fd, 0);

    file_system::ManagedFileDescriptor source{open_read_only_null()};
    const int transferred_fd = source.file_descriptor();
    ASSERT_GE(transferred_fd, 0);

    destination = std::move(source);

    EXPECT_FALSE(descriptor_is_valid(previous_fd));
    EXPECT_FALSE(source.is_open());
    EXPECT_EQ(source.file_descriptor(), -1);
    EXPECT_TRUE(destination.is_open());
    EXPECT_EQ(destination.file_descriptor(), transferred_fd);
    EXPECT_TRUE(descriptor_is_valid(transferred_fd));
}

TEST(ManagedFileDescriptor, MoveAssignmentFromClosedSourceClosesDestination)
{
    file_system::ManagedFileDescriptor destination{open_read_only_null()};
    const int previous_fd = destination.file_descriptor();
    ASSERT_GE(previous_fd, 0);
    file_system::ManagedFileDescriptor source;

    destination = std::move(source);

    EXPECT_FALSE(descriptor_is_valid(previous_fd));
    EXPECT_FALSE(destination.is_open());
    EXPECT_EQ(destination.file_descriptor(), -1);
    EXPECT_FALSE(source.is_open());
}

TEST(ManagedFileDescriptor, SelfMoveAssignmentLeavesOwnershipIntact)
{
    file_system::ManagedFileDescriptor file{open_read_only_null()};
    const int fd = file.file_descriptor();
    ASSERT_GE(fd, 0);
    file_system::ManagedFileDescriptor& alias = file;

    file = std::move(alias);

    EXPECT_TRUE(file.is_open());
    EXPECT_EQ(file.file_descriptor(), fd);
    EXPECT_TRUE(descriptor_is_valid(fd));
}

TEST(ManagedFileDescriptor, CopyIsDeletedAndMoveIsNoexcept)
{
    static_assert(!std::is_copy_constructible_v<file_system::ManagedFileDescriptor>);
    static_assert(!std::is_copy_assignable_v<file_system::ManagedFileDescriptor>);
    static_assert(std::is_nothrow_move_constructible_v<file_system::ManagedFileDescriptor>);
    static_assert(std::is_nothrow_move_assignable_v<file_system::ManagedFileDescriptor>);
}
