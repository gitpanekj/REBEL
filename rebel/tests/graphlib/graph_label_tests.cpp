#include <cstring>
#include <format>
#include <graphlib/graph_labels.hpp>
#include <gtest/gtest.h>
#include <vector>

#include "graphlib/graph_types.hpp"
#include "graphlib/serialization.hpp"



namespace graphlib {
template <> struct Serializer<std::string> {
  static std::string deserialize(std::span<std::byte> data) {
    return std::string(reinterpret_cast<const char *>(data.data()),
                       data.size());
  }
  static std::vector<std::byte> serialize(const std::string &s) {
    struct S {
      size_t len;
      size_t offset;
    };
    S header = {.len = s.size(), .offset = sizeof(size_t) * 2};

    std::vector<std::byte> data;
    data.insert(data.end(), reinterpret_cast<std::byte *>(&header),
                reinterpret_cast<std::byte *>(&header) + sizeof(size_t) * 2);
    for (const auto c : s) {
      data.push_back(static_cast<std::byte>(c));
    }
    return data;
  }

  static offset_t serialize(const std::string &s,
                            std::vector<std::byte> &buffer) {
    struct S {
      size_t len;
      size_t offset;
    };
    S header = {.len = s.size(), .offset = sizeof(size_t) * 2};
    buffer.insert(buffer.end(), reinterpret_cast<std::byte *>(&header),
                   reinterpret_cast<std::byte *>(&header) + sizeof(size_t) * 2);
    for (const auto c : s) {
      buffer.push_back(static_cast<std::byte>(c));
    }
    return s.size();
  }
};
}

using namespace graphlib;

TEST(GraphLabels, EmptyLabels) {
  // Arrange
  LabelStorage<int> label_storage{};
  // Act
  std::vector<std::byte> serialized_labels = serialize(label_storage);
  // // Assert
  LabelStorageView view(serialized_labels);

  IdType id = 8;
  auto raw_elem_opt = view.getRawLabelData(id);
  ASSERT_FALSE(raw_elem_opt) << "Id: " << id;
}

TEST(GraphLabels, IntegerLabels) {
  // Arrange
  LabelStorage<int> label_storage{};
  for (int i = 0; i < 10; i++)
    label_storage.setData(i, i * 10);
  for (int i = 3; i < 6; i++)
    label_storage.clearData(i);
  label_storage.clearData(100); // invalid id

  // Act
  std::vector<std::byte> serialized_labels = serialize(label_storage);

  // // Assert
  LabelStorageView view(serialized_labels);

  for (const auto id : {0, 1, 2, 6, 7, 8, 9}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_TRUE(raw_elem_opt) << "Id: " << id;
    auto raw_value = raw_elem_opt.value();

    EXPECT_EQ(raw_value.size(), sizeof(int));
    const int *a = reinterpret_cast<const int *>(raw_value.data());
    EXPECT_EQ(*a, id * 10);
  }

  for (const auto id : {3, 4, 5}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_FALSE(raw_elem_opt) << "Id: " << id;
  }
}

TEST(GraphLabels, TriviallyCopyableStructLabels) {
  // Arrange
  struct Foo {
    char a;
    int b;
    double c;
    Foo(char a, int b, double c) : a(a), b(b), c(c) {}
    Foo() : a(0), b(0), c(0.0) {}
  };
  LabelStorage<Foo> label_storage{};
  for (int i = 0; i < 10; i++)
    label_storage.setData(i, Foo('a' + i, i, double(i)));
  for (int i = 3; i < 6; i++)
    label_storage.clearData(i);
  label_storage.clearData(100); // invalid id

  // Act
  std::vector<std::byte> serialized_labels = serialize(label_storage);

  // // Assert
  LabelStorageView view(serialized_labels);

  for (const auto id : {0, 1, 2, 6, 7, 8, 9}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_TRUE(raw_elem_opt) << "Id: " << id;
    auto raw_value = raw_elem_opt.value();

    EXPECT_EQ(raw_value.size(), sizeof(Foo));
    const Foo *s = reinterpret_cast<const Foo *>(raw_value.data());
    EXPECT_EQ(s->a, 'a' + id) << "Id: " << id;
    EXPECT_EQ(s->b, id) << "Id: " << id;
    EXPECT_EQ(s->c, double(id)) << "Id: " << id;
  }

  for (const auto id : {3, 4, 5}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_FALSE(raw_elem_opt) << "Id: " << id;
  }
}

TEST(GraphLabels, StringLabels) {
  // Arrange
  LabelStorage<std::string> label_storage{};
  for (int i = 0; i < 10; i++)
    label_storage.setData(i, std::format("String %d", i));
  for (int i = 3; i < 6; i++)
    label_storage.clearData(i);

  // Act
  std::vector<std::byte> serialized_labels = serialize(label_storage);
  LabelStorageView view(serialized_labels);

  // Assert
  for (const auto id : {0, 1, 2, 6, 7, 8, 9}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_TRUE(raw_elem_opt) << "Id: " << id;
    auto raw_value = raw_elem_opt.value();

    std::string expected = std::format("String %d", id);
    struct StringView {
      size_t len;
      size_t offset;
    };
    const StringView *s =
        reinterpret_cast<const StringView *>(raw_value.data());
    ASSERT_EQ(s->len, expected.size());
    ASSERT_EQ(s->offset, sizeof(StringView)) << "Id: " << id;
    EXPECT_EQ(
        std::memcmp(expected.data(), raw_value.data() + s->offset, s->len), 0)
        << "Id: " << id;
    for (int i = 0; i < s->len; i++)
      std::cout << (char)(*(raw_value.data() + s->offset + i));
    std::cout << "\n";
  }

  for (const auto id : {3, 4, 5}) {
    auto raw_elem_opt = view.getRawLabelData(id);
    ASSERT_FALSE(raw_elem_opt) << "Id: " << id;
  }
}

// TEST(GraphLabels, ArrayOfStructsLabels) { ASSERT_TRUE(false); }
