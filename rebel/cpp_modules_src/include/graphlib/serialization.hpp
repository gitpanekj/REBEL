#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <vector>
#include <sstream>
#include <cstddef>
#include <cstring>

#include "graphlib/graph_labels.hpp"
#include "graphlib/graph_topology.hpp"
#include "graphlib/graph.hpp"
#include "graphlib/graph_types.hpp"

namespace graphlib {

template <GraphType T>
[[nodiscard]] std::string convertToDot(const T &obj) noexcept;

/**
 * @brief Serialize topology into a read-only binary representation.
 *
 * The serialized representation can be read using GraphTopologyView
 */
std::vector<std::byte> serialize(const DirectedGraphTopology &);
//DirectedGraphTopology deserialize(std::vector<std::byte> data);

/**
 * @brief Serialize labels into a read-only binary representation.
 *
 * The serialized representation can be read using GraphLabelView
 */
template <Serializable T>
std::vector<std::byte> serialize(const LabelStorage<T> &obj) {

  size_t n_objects = 0;
  if (obj.cbegin() == obj.cend())
    n_objects = 0;
  else{
    auto max_item = std::max_element(obj.cbegin(), obj.cend(),
                                 [](const auto &a, const auto &b) {
                                   return a.first < b.first;
                                 });
    n_objects = (*max_item).first + 1;
    }

  offset_t header_size = paddToMultipleOf(
      sizeof(typename LabelStorageView::LabelStorageViewHeader), 64);
  offset_t id_to_offset_map_size =
      paddToMultipleOf((n_objects + 1) * sizeof(offset_t), 64);

  if (n_objects == 0) { // return just header with all field set to zero
    return std::vector<std::byte>(header_size, std::byte{0});
  }

  // Header
  typename LabelStorageView::LabelStorageViewHeader header{};
  header.n_objects = n_objects;
  header.id_to_offset_map_offset = header_size;
  header.data_offset = header_size + id_to_offset_map_size;

  // Offsets
  std::vector<offset_t> id_to_offset_map(header.n_objects + 1, 0);

  // Data
  std::vector<std::byte> buffer; // TODO: 64byte alligment

  // Determine offsets and serialize data
  for (IdType id = 0; id < header.n_objects; id++) {
    auto data = obj.getConstData(id);
    if (data) {
      if constexpr (std::is_trivially_copyable_v<T>) {
        T value = *data;
        std::byte *bytes = reinterpret_cast<std::byte *>(&value);

        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));

        // TODO: Padd so that next inserted element has correct alignment
        // IS it even required ??
      } else {
        const T *value = data;
        offset_t size = Serializer<T>::serialize(*value, buffer);
        offset_t aligned_end_offset = paddToMultipleOf(buffer.size(), 8);
        if (aligned_end_offset - buffer.size() != 0) {
          buffer.insert(buffer.end(), aligned_end_offset - buffer.size(),
                        std::byte{0});
        }
      }
    }

    id_to_offset_map[id + 1] = buffer.size();
  }

  // Merge
  std::vector<std::byte> serialized_labels(
      header_size + id_to_offset_map_size + buffer.size(), std::byte{0});
  std::memcpy(serialized_labels.data(), &header, sizeof(header));
  std::memcpy(serialized_labels.data() + header_size, id_to_offset_map.data(),
              id_to_offset_map.size() * sizeof(offset_t));
  std::memcpy(serialized_labels.data() + header_size + id_to_offset_map_size,
              buffer.data(), buffer.size());

  return serialized_labels;
}
//template <Serializable T>
//LabelStorage<T> deserialize(std::vector<std::byte> data);


/**
 * @brief Serialize graph into a read-only binary representation.
 *
 * The serialized representation can be read using GraphView
 */
template <Serializable T>
std::pair<std::vector<std::byte>, std::vector<std::byte>> serialize(const Graph<T>& obj){
    std::vector<std::byte> topo = serialize(static_cast<const DirectedGraphTopology&>(obj));
    std::vector<std::byte> labels = serialize(static_cast<const LabelStorage<T>&>(obj));
    return {topo, labels};
}


template <GraphType T>
[[nodiscard]] std::string convertToDot(const T &obj) noexcept {
  std::ostringstream dotStream;

  dotStream << "digraph G {" << std::endl;

  dotStream << "\t// Node definitions " << std::endl;
  for (const auto &node_id : obj.getVertices()) {
    dotStream << "\t" << node_id << ";" << std::endl;
  }

  dotStream << "\n";
  dotStream << "\t// Edge definitions " << std::endl;
  for (const auto &node_id : obj.getVertices()) {
    for (const auto &edge : getOutgoingEdges(node_id)) {
      dotStream << "\t" << node_id << " -> " << edge.to << ";" << std::endl;
    }
  }
  dotStream << "}\n" << std::endl;
  return dotStream.str();
}
} // namespace graphlib

#endif