/**
 * @file graph_labels.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-10-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef GRAPH_LABELS
#define GRAPH_LABELS

#include "graphlib/graph_types.hpp"
#include "graphlib/memory_utils.hpp"
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace graphlib {

/**
 * @brief An inutable graph label storage
 *
 * This class represents an umutable id to label mapping for graph (see @ref
 * DirectedGraphTopology, @ref DirectedGraphTopologyView).
 *
 * It provides retrieve utilities of the labels.
 *
 * The memory layout is | Header | Payload | where Header contains information
 * about number of contained labels and offsets to the individual parts of payload.
 * Payloads consists of the following parts:
 *  id_to_offset_map: on position edge_id constains offsets of label data relative to the start of data_span for given edge
 *  data_span: flattened label data for all edges
 */
class LabelStorageView {
public:
  struct alignas(64) LabelStorageViewHeader {
    std::size_t n_objects;
    offset_t id_to_offset_map_offset;
    offset_t data_offset;
    LabelStorageViewHeader() {
      n_objects = 0;
      id_to_offset_map_offset = 0;
      data_offset = 0;
    }
  };

private:
  const LabelStorageViewHeader *header;
  std::vector<std::byte> raw_data;
  std::span<offset_t> id_to_offset_map;
  std::span<const std::byte> data_span;

public:
  explicit LabelStorageView(std::vector<std::byte> bytes) {
    raw_data = bytes;
    header = offset_to_ptr<const LabelStorageViewHeader>(raw_data.data(), 0);
    id_to_offset_map =
        std::span(offset_to_ptr<offset_t>(raw_data.data(),
                                          header->id_to_offset_map_offset),
                  header->n_objects + 1);
    data_span = std::span(raw_data.data() + header->data_offset,
                          raw_data.size() - header->data_offset);
  }
  LabelStorageView() = delete;

  /**
   * @brief Get byte representation of the stored label for given id
   *
   *
   * @param id Id if the labeled object
   * @return std::optional<std::span<const std::byte>> label or None if the id
   * is not assigned a value.
   */
  std::optional<std::span<const std::byte>> getRawLabelData(IdType id) {
    if (id >= header->n_objects)
      return std::nullopt;
    offset_t start = id_to_offset_map[id];
    offset_t size = id_to_offset_map[id + 1] - start;
    if (size == 0)
      return std::nullopt;
    return std::optional<std::span<const std::byte>>(
        data_span.subspan(start, size));
  }

  std::size_t getNumberOfLabels(){
    return header->n_objects;
  }
}; // End of LabelStorageView class


/**
 * @brief A mutable graph label storage
 *
 * This class represents a mutable id to label mapping for graph (see @ref
 * DirectedGraphTopology, @ref DirectedGraphTopologyView).
 *
 * It provides store and retrieve utilities of the labels and serialization of
 * itself into binary format.
 *
 * The label type must be either trially copyable or the user must provide
 * function for serialization and deserialization required by @ref Serializable.
 *
 * @tparam Data graph label data type
 *
 */
template <Serializable Data> class LabelStorage {
private:
  std::unordered_map<IdType, Data> id_to_label_map;

public:
  explicit LabelStorage(LabelStorageView &view) {
    auto no_labels = view.getNumberOfLabels();
    for (size_t i = 0; i < no_labels; i++){
      if (auto label = view.getRawLabelData(i))
        id_to_label_map.insert(i, Data::deserialize(label.value()));
    }
  }
  LabelStorage() = default;


  auto cbegin() const noexcept {
    return id_to_label_map.cbegin();
  }

  auto cend() const noexcept {
    return id_to_label_map.cend();
  }


  /**
   * @brief Assign data to an id.
   *
   * If data is already assigned to the id, original data is overwritten.
   *
   * @param id id of an object
   * @param data data to be assigned
   */
  void setData(IdType id, Data &&data) noexcept {
    id_to_label_map.insert({id, std::forward<Data>(data)});
  }

  /**
   * @brief Clear data of a an object with given id.
   *
   * If data is not assigned, method has no effect.
   *
   * @param id id of an object
   */
  void clearData(IdType id) noexcept { id_to_label_map.erase(id); }

  /**
   * @brief Get a pointer to data of an object with given id.
   *
   * @param id id of an object
   * @return a pointer to the data assigned to the vertex with given id
   */
  Data* getData(IdType id) noexcept {
    auto data = id_to_label_map.find(id);
    if (data == id_to_label_map.end())
      return nullptr;
    return &data->second;
  }


  const Data* getConstData(IdType id) const noexcept {
    auto data = id_to_label_map.find(id);
    if (data == id_to_label_map.end())
      return nullptr;
    return &data->second;
  }
}; // End of LabelStorage class

} // namespace graphlib

#endif
